# Implementation Plan: DVB-S2 Loopback Binary

## Overview

This plan implements the `dvbs2_loopback` binary — a full-duplex DVB-S2 hardware-in-the-loop test executable that transmits and receives on a single Ettus B200-mini SDR. The implementation reuses all existing `factory::DVBS2` module construction and `Radio_USRP` full-duplex capability, removing simulated channel impairments and replacing them with real RF path traversal. The binary follows the same waiting/learning phase pattern as `dvbs2_tx_rx` for synchronizer acquisition.

## Tasks

- [x] 1. Create directory structure and build system integration
  - [x] 1.1 Create `src/mains/LOOPBACK/main.cpp` with UHD compile guard and skeleton
    - Create the directory `src/mains/LOOPBACK/`
    - Write a minimal `main.cpp` with `#ifndef DVBS2_LINK_UHD` / `#error "UHD is required for loopback operation"` / `#endif` guard
    - Include headers: `<aff3ct.hpp>`, `<streampu.hpp>`, `"Factory/DVBS2/DVBS2.hpp"`, reporter headers
    - Add the `main()` function skeleton that returns `EXIT_SUCCESS`
    - _Requirements: 9.1, 9.4_

  - [x] 1.2 Add CMakeLists.txt entries for `dvbs2_loopback` target
    - Add `set(SRC_FILES_LOOPBACK ${CMAKE_CURRENT_SOURCE_DIR}/src/mains/LOOPBACK/main.cpp)` source file variable
    - Add `add_executable(dvbs2_loopback ${SRC_FILES_LOOPBACK})` target
    - Add `target_link_libraries(dvbs2_loopback PRIVATE dvbs2_common)` and `target_link_libraries(dvbs2_loopback PRIVATE aff3ct-static-lib)`
    - Add `target_include_directories(dvbs2_loopback PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)`
    - _Requirements: 9.1, 9.2, 9.3_

- [x] 2. Implement signal handling, CLI parsing, and tool/module construction
  - [x] 2.1 Implement signal handler registration and CLI parsing
    - Call `spu::tools::Signal_handler::init()` at the start of `main()`
    - Construct `factory::DVBS2(argc, argv)` params object
    - Print parameter headers using `tools::Header::print_parameters()`
    - _Requirements: 6.1, 8.4_

  - [x] 2.2 Implement tool construction (constellation, BCH poly gen, interleaver core)
    - Construct `tools::Constellation_user<float>` from `params.constellation_file`
    - Construct `tools::BCH_polynomial_generator<>` from `params.N_bch_unshortened`, 12, `params.bch_prim_poly`
    - Construct `tools::Interleaver_core<>` via `factory::DVBS2::build_itl_core<>(params)`
    - Construct `tools::Sigma<> noise_ref` for noise estimation reference
    - _Requirements: 2.2, 3.2_

  - [x] 2.3 Implement TX chain module construction
    - Construct source, bb_scrambler, BCH_encoder, LDPC_cdc (encoder), itl_tx, modem, framer, pl_scrambler, shaping_flt using `factory::DVBS2::build_*()` methods
    - All modules constructed identically to `dvbs2_tx_rx`
    - _Requirements: 2.1, 2.2, 2.3_

  - [x] 2.4 Implement RX chain module construction
    - Construct sync_coarse_f, matched_flt, sync_timing, mult_agc, sync_frame, feedbr, pl_scrambler (descramble), sync_fine_lr, sync_fine_pf, framer (remove_plh), estimator, modem (demodulate), itl_rx, LDPC_decoder, BCH_decoder, bb_descrambler using `factory::DVBS2::build_*()` methods
    - Construct `sync_step_mf` using the combined synchronizer constructor with `sync_coarse_f`, `matched_flt`, `sync_timing`
    - Construct front-end AGC module (`Multiplier_AGC_cc_naive<>`) via `factory::DVBS2::build_front_agc<>(params)` for real RF sample amplitude normalization
    - Do NOT construct any simulated channel modules (Channel, freq_shift, fad_mlt, chn_frac_del, chn_int_del, chn_frm_del)
    - _Requirements: 3.1, 3.2, 3.4, 7.1, 7.2_

  - [x] 2.5 Implement Radio_USRP construction with full-duplex configuration
    - Construct a single `Radio_USRP` instance via `factory::DVBS2::build_radio<>(params)` with both `rx_enabled=true` and `tx_enabled=true` (enabled by specifying both `--rad-tx-rate` and `--rad-rx-rate`)
    - TX antenna set to `"TX/RX"`, RX antenna set to `"RX2"` (factory defaults)
    - Threaded mode controlled by `--rad-threaded` flag
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 10.2_

  - [x] 2.6 Implement monitor, delay, and reporter construction
    - Construct `Monitor_BFER<>` via `factory::DVBS2::build_monitor<>(params)`
    - Construct delay buffer via `factory::DVBS2::build_txrx_delay<>(params)`
    - Call `monitor->disable_is_done(true)` to prevent auto-termination
    - Construct `Reporter_BFER`, `Reporter_throughput`, and `Terminal_std` for BER/FER/throughput display
    - Construct probe reporters (frame sync, timing sync, freq sync, decode status, noise, BFER, throughput) following the `dvbs2_tx_rx` pattern
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 6.4_

- [x] 3. Checkpoint - Ensure the binary compiles and links
  - Ensure all tests pass, ask the user if questions arise.

- [x] 4. Implement socket bindings for TX, RX, and monitor paths
  - [x] 4.1 Implement TX chain socket bindings
    - Bind: source → bb_scrambler → BCH_encoder → LDPC_encoder → itl_tx → modem(modulate) → framer(generate) → pl_scrambler(scramble) → shaping_flt → radio(send)
    - Connect shaping filter output directly to radio `send` task (no simulated channel in between)
    - _Requirements: 2.1, 2.4, 7.1, 7.2_

  - [x] 4.2 Implement RX chain socket bindings
    - Bind: radio(receive) → front_agc → sync_coarse_f → matched_flt → sync_timing → sync_timing(extract) → mult_agc → sync_frame → pl_scrambler(descramble) → sync_fine_lr → sync_fine_pf → framer(remove_plh) → estimator → modem(demodulate) → itl_rx → LDPC_decoder → BCH_decoder → bb_descrambler
    - Connect radio `receive` output directly to `front_agc` input (no simulated channel)
    - _Requirements: 3.1, 3.3, 7.3_

  - [x] 4.3 Implement monitor path socket bindings and probe bindings
    - Bind: source → delay → monitor(check_errors2::U)
    - Bind: bb_descrambler → monitor(check_errors2::V)
    - Bind all probe reporters to their respective module outputs following the `dvbs2_tx_rx` pattern
    - _Requirements: 5.1, 5.2_

- [x] 5. Implement waiting and learning phases
  - [x] 5.1 Implement waiting phase with sync_step_mf rebinding
    - Unbind steady-state RX front (sync_coarse_f ← radio receive)
    - Rebind to use `sync_step_mf` combined synchronizer: radio(receive) → front_agc → sync_step_mf
    - Bind `feedbr` (feedbacker): sync_frame(DEL) → feedbr(memorize), feedbr(produce) → sync_step_mf(DEL)
    - Construct `firsts_wl12` vector with source (TX continues), feedbr(produce), and relevant probes
    - Set PLL coefficients: `sync_coarse_f->set_PLL_coeffs(1, 1/sqrt(2.0), 1e-4)` for wide acquisition
    - Execute sequence until `sync_frame->get_packet_flag()` returns true
    - On abort, increment `delay_tx_rx += params.n_frames`
    - _Requirements: 4.1, 4.2, 4.3_

  - [x] 5.2 Implement learning phases 1, 2, and 3
    - Learning Phase 1 (150 frames): same sequence as waiting, PLL coeffs `(1, 1/sqrt(2), 1e-4)`
    - Learning Phase 2 (150 frames): tighten PLL coeffs to `(1, 1/sqrt(2), 5e-5)`
    - Learning Phase 3 (200 frames): unbind sync_step_mf, rebind standard sync_coarse_f → matched_flt → sync_timing chain, execute up to sync_fine_pf without BER counting
    - _Requirements: 4.4, 4.5_

- [x] 6. Implement steady-state sequence execution and shutdown
  - [x] 6.1 Implement steady-state sequence with BER/FER counting
    - Call `monitor->reset()` to clear any learning-phase counts
    - Set `delay->set_delay(delay_tx_rx)` with accumulated pipeline delay
    - Set `sync_timing->set_act(true)` to enable timing update tracking
    - Construct `firsts_t` vector with `source(generate)` and `radio(receive)` as independent first-stage tasks
    - Create `spu::runtime::Sequence` with `firsts_t`
    - Execute sequence in a loop, displaying periodic terminal stats via `terminal_stats.temp_report()`
    - First frame handling: reset monitor until `d >= delay_tx_rx + params.n_frames`
    - _Requirements: 5.3, 5.4, 10.1, 10.3_

  - [x] 6.2 Implement graceful shutdown via signal handling
    - Sequence loop terminates when signal handler flag is set
    - Call `radio_usrp->cancel_waiting()` to unblock pending FIFO operations
    - Call `terminal.final_report()` to display final BER/FER/throughput statistics
    - _Requirements: 6.1, 6.2, 6.3_

- [x] 7. Final checkpoint - Ensure the binary compiles, links, and CLI args are accepted
  - Ensure all tests pass, ask the user if questions arise.

- [x]* 8. Write build verification and integration tests
  - [x]* 8.1 Write compile-and-link test verifying the binary builds without errors
    - Add a CMake test that verifies `dvbs2_loopback` target compiles and links
    - _Requirements: 9.1, 9.2, 9.3_

  - [x]* 8.2 Write UHD-required guard test
    - Verify that building with `DVBS2_LINK_UHD=OFF` produces a compile error from the `#error` directive
    - _Requirements: 9.4_

  - [x]* 8.3 Write CLI argument acceptance test
    - Test that `dvbs2_loopback --help` or dummy invocation with all documented CLI args (`--rad-clk-rate`, `--rad-serial`, `--rad-tx-rate`, `--rad-rx-rate`, `--rad-tx-freq`, `--rad-rx-freq`, `--rad-tx-gain`, `--rad-rx-gain`, `--rad-threaded`, `--mod-cod`, `--dec-implem`, `--dec-ite`, `--src-type`) exits cleanly or is parsed without error
    - _Requirements: 8.1, 8.2, 8.3, 8.4, 8.5_

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation
- The design explicitly states property-based testing does not apply (infrastructure wiring feature)
- The primary validation is a manual hardware cable-loopback test (connect TX/RX → attenuator → RX2)
- The implementation language is C++ (matching the existing codebase)
- Unit tests and integration tests target build system correctness and CLI parsing, not module logic (modules are independently tested)

## Task Dependency Graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["1.1", "1.2"] },
    { "id": 1, "tasks": ["2.1"] },
    { "id": 2, "tasks": ["2.2", "2.3", "2.4", "2.5"] },
    { "id": 3, "tasks": ["2.6"] },
    { "id": 4, "tasks": ["4.1", "4.2", "4.3"] },
    { "id": 5, "tasks": ["5.1"] },
    { "id": 6, "tasks": ["5.2"] },
    { "id": 7, "tasks": ["6.1", "6.2"] },
    { "id": 8, "tasks": ["8.1", "8.2", "8.3"] }
  ]
}
```
