#ifndef DVBS2_LINK_UHD
#error "UHD is required for loopback operation. Build with -DDVBS2_LINK_UHD=ON"
#endif

#include <vector>
#include <string>
#include <chrono>
#include <numeric>
#include <iostream>
#include <algorithm>

#include <aff3ct.hpp>
#include <streampu.hpp>

#include "Factory/DVBS2/DVBS2.hpp"
#include "Tools/Reporter/Reporter_throughput_DVBS2.hpp"
#include "Tools/Reporter/Reporter_noise_DVBS2.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

int main(int argc, char** argv)
{
	// setup signal handlers
	spu::tools::Signal_handler::init();

	// get the parameter to configure the tools and modules
	auto params = factory::DVBS2(argc, argv);

	std::cout << "[trace]" << std::endl;
	std::map<std::string,tools::header_list> headers;
	std::vector<factory::Factory*> param_vec;
	param_vec.push_back(&params);
	tools::Header::print_parameters(param_vec, false, std::cout);

	// construct tools
	tools::Constellation_user<float> cstl(params.constellation_file);
	tools::BCH_polynomial_generator<> poly_gen(params.N_bch_unshortened, 12, params.bch_prim_poly);
	std::unique_ptr<tools::Interleaver_core<>> itl_core(factory::DVBS2::build_itl_core<>(params));
	tools::Sigma<> noise_ref;

	// aliases
	using uptr_source      = std::unique_ptr<spu::module::Source<>>;
	using uptr_sink        = std::unique_ptr<spu::module::Sink<>>;

	// construct TX chain modules
	uptr_source                                        source        (factory::DVBS2::build_source          <>(params             ));
	std::unique_ptr<Scrambler<>                 > bb_scrambler  (factory::DVBS2::build_bb_scrambler    <>(params             ));
	std::unique_ptr<Encoder<>                   > BCH_encoder   (factory::DVBS2::build_bch_encoder     <>(params, poly_gen   ));
	std::unique_ptr<tools::Codec_SIHO<>         > LDPC_cdc      (factory::DVBS2::build_ldpc_cdc        <>(params             ));
	std::unique_ptr<Interleaver<>               > itl_tx        (factory::DVBS2::build_itl             <>(params, *itl_core  ));
	std::unique_ptr<Modem<>                     > modem         (factory::DVBS2::build_modem           <>(params, &cstl      ));
	std::unique_ptr<Framer<>                    > framer        (factory::DVBS2::build_framer          <>(params             ));
	std::unique_ptr<Scrambler<float>            > pl_scrambler  (factory::DVBS2::build_pl_scrambler    <>(params             ));
	std::unique_ptr<Filter_UPRRC_ccr_naive<>    > shaping_flt   (factory::DVBS2::build_uprrc_filter    <>(params             ));
	auto* LDPC_encoder = &LDPC_cdc->get_encoder();

	// construct RX chain modules (no simulated channel modules)
	std::unique_ptr<Filter_RRC_ccr_naive<>      > matched_flt   (factory::DVBS2::build_matched_filter           <>(params             ));
	std::unique_ptr<Synchronizer_timing<>       > sync_timing   (factory::DVBS2::build_synchronizer_timing      <>(params             ));
	std::unique_ptr<Multiplier_AGC_cc_naive<>   > mult_agc      (factory::DVBS2::build_agc_shift                <>(params             ));
	std::unique_ptr<Synchronizer_frame<>        > sync_frame    (factory::DVBS2::build_synchronizer_frame       <>(params             ));
	std::unique_ptr<Feedbacker<>                > feedbr        (factory::DVBS2::build_feedbacker               <>(params             ));
	std::unique_ptr<Estimator<>                 > estimator     (factory::DVBS2::build_estimator                <>(params, &noise_ref ));
	std::unique_ptr<Synchronizer_freq_coarse<>  > sync_coarse_f (factory::DVBS2::build_synchronizer_freq_coarse <>(params             ));
	std::unique_ptr<Synchronizer_freq_fine<>    > sync_fine_pf  (factory::DVBS2::build_synchronizer_freq_phase  <>(params             ));
	std::unique_ptr<Synchronizer_freq_fine<>    > sync_fine_lr  (factory::DVBS2::build_synchronizer_lr          <>(params             ));
	std::unique_ptr<Synchronizer_step_mf_cc<>   > sync_step_mf  (factory::DVBS2::build_synchronizer_step_mf_cc  <>(params,
	                                                                                                    sync_coarse_f.get(),
	                                                                                                    matched_flt  .get(),
	                                                                                                    sync_timing  .get()));
	std::unique_ptr<Interleaver<float,uint32_t> > itl_rx        (factory::DVBS2::build_itl<float,uint32_t>        (params, *itl_core  ));
	std::unique_ptr<Decoder_HIHO<>              > BCH_decoder   (factory::DVBS2::build_bch_decoder              <>(params, poly_gen   ));
	std::unique_ptr<Scrambler<>                 > bb_descrambler(factory::DVBS2::build_bb_scrambler             <>(params             ));
	auto* LDPC_decoder = &LDPC_cdc->get_decoder_siho();

	// construct Radio_USRP with full-duplex (both TX and RX enabled via CLI --rad-tx-rate and --rad-rx-rate)
	std::unique_ptr<module::Radio<>             > radio         (params.p_rad.build<float>());

	// construct monitor, delay, and sink
	std::unique_ptr<Filter_buffered_delay<>     > delay         (factory::DVBS2::build_txrx_delay               <>(params             ));
	std::unique_ptr<Monitor_BFER<>              > monitor       (factory::DVBS2::build_monitor                  <>(params             ));
	uptr_sink                                     sink          (factory::DVBS2::build_sink                     <>(params             ));

	// disable auto-termination so the loopback runs continuously
	monitor->disable_is_done(true);

	// set custom names for clarity
	delay        ->set_custom_name("TX/RX Delay");
	LDPC_encoder ->set_custom_name("LDPC Encoder");
	LDPC_decoder ->set_custom_name("LDPC Decoder");
	BCH_encoder  ->set_custom_name("BCH Encoder" );
	BCH_decoder  ->set_custom_name("BCH Decoder" );
	sync_fine_lr ->set_custom_name("L&R F Syn"   );
	sync_fine_pf ->set_custom_name("Fine P/F Syn");
	sync_timing  ->set_custom_name("Timing Syn"  );
	sync_frame   ->set_custom_name("Frame Syn"   );
	matched_flt  ->set_custom_name("Matched Flt" );
	shaping_flt  ->set_custom_name("Shaping Flt" );
	sync_coarse_f->set_custom_name("Coarse_Synch");
	sync_step_mf ->set_custom_name("MF_Synch"    );
	mult_agc     ->set_custom_name("Mult AGC"    );

	// allocate reporters for terminal display
	// Note: no simulated noise in loopback, so set noise_ref to a placeholder value
	noise_ref.set_values(1.0, 0.0, 0.0); // sigma=1, ebn0=0, esn0=0 (placeholder)
	tools::Reporter_noise<>      rep_noise( noise_ref, true);
	tools::Reporter_BFER<>       rep_BFER (*monitor        );
	tools::Reporter_throughput<> rep_thr  (*monitor        );

	// allocate terminal
	spu::tools::Terminal_std terminal({ &rep_noise, &rep_BFER, &rep_thr });

	// display the legend
	terminal.legend();

	int delay_tx_rx = params.overall_delay;
	delay->set_delay(delay_tx_rx);

	// ========================================================================================================
	// SOCKET BINDINGS
	// ========================================================================================================

	// --- TX chain: source → ... → shaping_flt → radio(send) ---
	(*bb_scrambler  )[             scr::sck::scramble     ::X_N1   ] = (*source        )[spu::module::src::sck::generate     ::out_data];
	(*BCH_encoder   )[             enc::sck::encode       ::U_K    ] = (*bb_scrambler  )[             scr::sck::scramble     ::X_N2    ];
	(*LDPC_encoder  )[             enc::sck::encode       ::U_K    ] = (*BCH_encoder   )[             enc::sck::encode       ::X_N     ];
	(*itl_tx        )[             itl::sck::interleave   ::nat    ] = (*LDPC_encoder  )[             enc::sck::encode       ::X_N     ];
	(*modem         )[             mdm::sck::modulate     ::X_N1   ] = (*itl_tx        )[             itl::sck::interleave   ::itl     ];
	(*framer        )[             frm::sck::generate     ::Y_N1   ] = (*modem         )[             mdm::sck::modulate     ::X_N2    ];
	(*pl_scrambler  )[             scr::sck::scramble     ::X_N1   ] = (*framer        )[             frm::sck::generate     ::Y_N2    ];
	(*shaping_flt   )[             flt::sck::filter       ::X_N1   ] = (*pl_scrambler  )[             scr::sck::scramble     ::X_N2    ];
	(*radio         )[             rad::sck::send         ::X_N1   ] = (*shaping_flt   )[             flt::sck::filter       ::Y_N2    ];

	// --- RX chain: radio(receive) → sync → decode ---
	(*sync_coarse_f )[             sfc::sck::synchronize  ::X_N1   ] = (*radio         )[             rad::sck::receive      ::Y_N1    ];
	(*matched_flt   )[             flt::sck::filter       ::X_N1   ] = (*sync_coarse_f )[             sfc::sck::synchronize  ::Y_N2    ];
	(*sync_timing   )[             stm::sck::synchronize  ::X_N1   ] = (*matched_flt   )[             flt::sck::filter       ::Y_N2    ];
	(*sync_timing   )[             stm::sck::extract      ::B_N1   ] = (*sync_timing   )[             stm::sck::synchronize  ::B_N1    ];
	(*sync_timing   )[             stm::sck::extract      ::Y_N1   ] = (*sync_timing   )[             stm::sck::synchronize  ::Y_N1    ];
	(*mult_agc      )[             mlt::sck::imultiply    ::X_N    ] = (*sync_timing   )[             stm::sck::extract      ::Y_N2    ];
	(*sync_frame    )[             sfm::sck::synchronize  ::X_N1   ] = (*mult_agc      )[             mlt::sck::imultiply    ::Z_N     ];
	(*pl_scrambler  )[             scr::sck::descramble   ::Y_N1   ] = (*sync_frame    )[             sfm::sck::synchronize  ::Y_N2    ];
	(*sync_fine_lr  )[             sff::sck::synchronize  ::X_N1   ] = (*pl_scrambler  )[             scr::sck::descramble   ::Y_N2    ];
	(*sync_fine_pf  )[             sff::sck::synchronize  ::X_N1   ] = (*sync_fine_lr  )[             sff::sck::synchronize  ::Y_N2    ];
	(*framer        )[             frm::sck::remove_plh   ::Y_N1   ] = (*sync_fine_pf  )[             sff::sck::synchronize  ::Y_N2    ];
	(*estimator     )[             est::sck::estimate     ::X_N    ] = (*framer        )[             frm::sck::remove_plh   ::Y_N2    ];
	(*modem         )[             mdm::sck::demodulate   ::CP     ] = (*estimator     )[             est::sck::estimate     ::SIG     ];
	(*modem         )[             mdm::sck::demodulate   ::Y_N1   ] = (*framer        )[             frm::sck::remove_plh   ::Y_N2    ];
	(*itl_rx        )[             itl::sck::deinterleave ::itl    ] = (*modem         )[             mdm::sck::demodulate   ::Y_N2    ];
	(*LDPC_decoder  )[             dec::sck::decode_siho  ::Y_N    ] = (*itl_rx        )[             itl::sck::deinterleave ::nat     ];
	(*BCH_decoder   )[             dec::sck::decode_hiho  ::Y_N    ] = (*LDPC_decoder  )[             dec::sck::decode_siho  ::V_K     ];
	(*bb_descrambler)[             scr::sck::descramble   ::Y_N1   ] = (*BCH_decoder   )[             dec::sck::decode_hiho  ::V_K     ];

	// --- Monitor path: source → delay → monitor, bb_descrambler → monitor ---
	(*delay         )[             flt::sck::filter       ::X_N1   ] = (*source        )[spu::module::src::sck::generate     ::out_data];
	(*monitor       )[             mnt::sck::check_errors2::U      ] = (*delay         )[             flt::sck::filter       ::Y_N2    ];
	(*monitor       )[             mnt::sck::check_errors2::V      ] = (*bb_descrambler)[             scr::sck::descramble   ::Y_N2    ];
	(*sink          )[spu::module::snk::sck::send         ::in_data] = (*bb_descrambler)[             scr::sck::descramble   ::Y_N2    ];

	// ========================================================================================================
	// WAITING & LEARNING PHASES
	// ========================================================================================================
	if (!params.perfect_sync)
	{
		// --- Waiting Phase: rebind to use sync_step_mf ---
		// partial unbinding
		(*sync_coarse_f)[             sfc::sck::synchronize::X_N1].unbind((*radio        )[rad::sck::receive     ::Y_N1]);
		(*sync_timing  )[             stm::sck::extract    ::B_N1].unbind((*sync_timing  )[stm::sck::synchronize::B_N1]);
		(*sync_timing  )[             stm::sck::extract    ::Y_N1].unbind((*sync_timing  )[stm::sck::synchronize::Y_N1]);

		// partial binding for waiting/learning phases 1&2
		(*sync_step_mf)[             smf::sck::synchronize::X_N1] = (*radio       )[rad::sck::receive     ::Y_N1];
		(*feedbr      )[             fbr::sck::memorize   ::X_N ] = (*sync_frame  )[sfm::sck::synchronize::DEL ];
		(*sync_step_mf)[             smf::sck::synchronize::DEL ] = (*feedbr      )[fbr::sck::produce    ::Y_N ];
		(*sync_timing )[             stm::sck::extract    ::B_N1] = (*sync_step_mf)[smf::sck::synchronize::B_N1];
		(*sync_timing )[             stm::sck::extract    ::Y_N1] = (*sync_step_mf)[smf::sck::synchronize::Y_N1];

		std::vector<spu::runtime::Task*> firsts_wl12 = { &(*source)[spu::module::src::tsk::generate],
		                                                 &(*radio )[             rad::tsk::receive ],
		                                                 &(*feedbr)[             fbr::tsk::produce ] };

		std::vector<spu::runtime::Task*> exclude_wl12 = { &(*pl_scrambler)[scr::tsk::descramble] };

		spu::runtime::Sequence sequence_waiting_and_learning_1_2(firsts_wl12, {}, exclude_wl12);

		// Waiting phase: wide PLL bandwidth for acquisition
		sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 1e-4);
		std::cout << "# Waiting for frame synchronization..." << std::endl;
		unsigned int m_wait = 0;
		sequence_waiting_and_learning_1_2.exec([&](const std::vector<const int*>& statuses)
		{
			m_wait++;
			if (statuses.back() == nullptr)
				delay_tx_rx += params.n_frames;
			return sync_frame->get_packet_flag();
		});
		std::cout << "# Frame lock acquired after " << m_wait << " iterations." << std::endl;

		// Learning Phase 1 (150 frames)
		std::cout << "# Learning phase 1..." << std::endl;
		sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 1e-4);
		unsigned int m_learn = 0;
		int limit = 150;
		sequence_waiting_and_learning_1_2.exec([&](const std::vector<const int*>& statuses)
		{
			m_learn++;
			if (statuses.back() == nullptr)
				delay_tx_rx += params.n_frames;

			// Transition to Learning Phase 2 at 150 frames
			if (limit == 150 && (int)m_learn >= 150)
			{
				std::cout << "# Learning phase 2..." << std::endl;
				limit = m_learn + 150;
				sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 5e-5);
			}
			return (int)m_learn >= limit;
		});

		// Learning Phase 3: rebind standard sync chain
		(*sync_step_mf)[             smf::sck::synchronize::X_N1].unbind((*radio       )[rad::sck::receive     ::Y_N1]);
		(*feedbr      )[             fbr::sck::memorize   ::X_N ].unbind((*sync_frame  )[sfm::sck::synchronize::DEL ]);
		(*sync_timing )[             stm::sck::extract    ::B_N1].unbind((*sync_step_mf)[smf::sck::synchronize::B_N1]);
		(*sync_timing )[             stm::sck::extract    ::Y_N1].unbind((*sync_step_mf)[smf::sck::synchronize::Y_N1]);

		(*sync_coarse_f)[             sfc::sck::synchronize::X_N1] = (*radio        )[rad::sck::receive     ::Y_N1];
		(*sync_timing  )[             stm::sck::extract    ::B_N1] = (*sync_timing  )[stm::sck::synchronize::B_N1];
		(*sync_timing  )[             stm::sck::extract    ::Y_N1] = (*sync_timing  )[stm::sck::synchronize::Y_N1];

		std::vector<spu::runtime::Task*> firsts_l3 = { &(*source)[spu::module::src::tsk::generate],
		                                               &(*radio )[             rad::tsk::receive ] };
		std::vector<spu::runtime::Task*> lasts_l3  = { &(*sync_fine_pf)[sff::tsk::synchronize] };

		spu::runtime::Sequence sequence_learning_3(firsts_l3, lasts_l3);

		std::cout << "# Learning phase 3..." << std::endl;
		unsigned int m_l3 = 0;
		sequence_learning_3.exec([&](const std::vector<const int*>& statuses)
		{
			m_l3++;
			if (statuses.back() == nullptr)
				delay_tx_rx += params.n_frames;
			return m_l3 >= 200;
		});
		std::cout << "# Learning complete." << std::endl;
	}

	// ========================================================================================================
	// STEADY-STATE EXECUTION
	// ========================================================================================================
	monitor->reset();
	delay->set_delay(delay_tx_rx);
	sync_timing->set_act(true);

	const std::vector<spu::runtime::Task*> firsts_t = { &(*source)[spu::module::src::tsk::generate],
	                                                     &(*radio )[             rad::tsk::receive ] };
	spu::runtime::Sequence sequence_transmission(firsts_t);

	if (params.ter_freq != std::chrono::nanoseconds(0))
		terminal.start_temp_report(params.ter_freq);

	std::cout << "# Steady-state: decoding frames..." << std::endl;
	terminal.legend();

	int d = 0;
	sequence_transmission.exec([&](const std::vector<const int*>& statuses)
	{
		if (statuses.back() != nullptr)
			d += params.n_frames;
		else
			delay_tx_rx += params.n_frames;

		if (d < delay_tx_rx + params.n_frames)
			monitor->reset();

		return false; // run continuously until Ctrl+C
	});

	terminal.final_report();
	std::cout << "#" << std::endl;
	std::cout << "# End of the simulation" << std::endl;

	return EXIT_SUCCESS;
}
