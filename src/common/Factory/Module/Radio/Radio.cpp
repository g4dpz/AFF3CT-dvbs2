#include <iostream>
#include <sstream>
#include <thread>

#include "Factory/Module/Radio/Radio.hpp"
#include "Module/Radio/Radio_NO/Radio_NO.hpp"
#include "Module/Radio/Radio_user/Radio_user_binary.hpp"
#ifdef DVBS2_LINK_UHD
	#include "Module/Radio/Radio_USRP/Radio_USRP.hpp"
#endif

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Radio_name   = "Radio";
const std::string aff3ct::factory::Radio_prefix = "rad";

Radio
::Radio(const std::string &prefix)
: Factory(Radio_name, Radio_name, prefix)
{
}

Radio* Radio
::clone() const
{
	return new Radio(*this);
}

void Radio
::get_description(cli::Argument_map_info &args) const
{
	auto p = this->get_prefix();
	args.add({p+"-fra-size",  "N"}, cli::Integer(cli::Positive(), cli::Non_zero())          , "");
	args.add({p+"-type"          }, cli::Text(cli::Including_set("USRP", "USER_BIN", "NO")) , "");
	args.add({p+"-threaded"      }, cli::None()                                             , "");
	args.add({p+"-fifo-size"     }, cli::Integer<uint64_t>(cli::Positive(), cli::Non_zero()), "");
	args.add({p+"-fra",       "F"}, cli::Integer(cli::Positive(), cli::Non_zero())          , "");
	args.add({p+"-clk-rate"      }, cli::Real(cli::Positive(), cli::Non_zero())             , "");
	args.add({p+"-rx-subdev-spec"}, cli::Text()                                             , "");
	args.add({p+"-rx-ant"        }, cli::Text()                                             , "");
	args.add({p+"-rx-rate"       }, cli::Real(cli::Positive(), cli::Non_zero())             , "");
	args.add({p+"-rx-freq"       }, cli::Real(cli::Positive(), cli::Non_zero())             , "");
	args.add({p+"-rx-gain"       }, cli::Real(cli::Positive(), cli::Non_zero())             , "");
	args.add({p+"-rx-file-path"  }, cli::Text()                                             , "");
	args.add({p+"-tx-file-path"  }, cli::Text()                                             , "");
	args.add({p+"-tx-subdev-spec"}, cli::Text()                                             , "");
	args.add({p+"-tx-ant"        }, cli::Text()                                             , "");
	args.add({p+"-tx-rate"       }, cli::Real(cli::Positive(), cli::Non_zero())             , "");
	args.add({p+"-tx-freq"       }, cli::Real(cli::Positive(), cli::Non_zero())             , "");
	args.add({p+"-tx-gain"       }, cli::Real(cli::Positive(), cli::Non_zero())             , "");
	args.add({p+"-serial"        }, cli::Text()                                             , "");
	args.add({p+"-rx-no-loop"    }, cli::None()                                             , "");
	args.add({p+"-rx-pin-core"   }, cli::Integer(cli::Positive())                           , "");
	args.add({p+"-tx-pin-core"   }, cli::Integer(cli::Positive())                           , "");
	args.add({p+"-clock-source"  }, cli::Text(cli::Including_set("internal", "gpsdo"))      , "");
}

void Radio
::store(const cli::Argument_map_value &vals)
{
	auto p = this->get_prefix();
	if (vals.exist({p+"-fra-size",  "N"})) this->N              = vals.to_int   ({p+"-fra-size",  "N"});
	if (vals.exist({p+"-type"          })) this->type           = vals.at       ({p+"-type"          });
	if (vals.exist({p+"-threaded"      })) this->threaded       = true                                 ;
	if (vals.exist({p+"-fifo-size"     })) this->fifo_size      = vals.to_uint64({p+"-fifo-size"     });
	if (vals.exist({p+"-fra",       "F"})) this->n_frames       = vals.to_int   ({p+"-fra",       "F"});
	if (vals.exist({p+"-clk-rate"      })) this->clk_rate       = vals.to_float ({p+"-clk-rate"      });
	if (vals.exist({p+"-rx-subdev-spec"})) this->rx_subdev_spec = vals.at       ({p+"-rx-subdev-spec"});
	if (vals.exist({p+"-rx-ant"        })) this->rx_antenna     = vals.at       ({p+"-rx-ant"        });
	if (vals.exist({p+"-rx-rate"       })) this->rx_enabled     = true                                 ;
	if (vals.exist({p+"-rx-rate"       })) this->rx_rate        = vals.to_float ({p+"-rx-rate"       });
	if (vals.exist({p+"-rx-freq"       })) this->rx_freq        = vals.to_float ({p+"-rx-freq"       });
	if (vals.exist({p+"-rx-gain"       })) this->rx_gain        = vals.to_float ({p+"-rx-gain"       });
	if (vals.exist({p+"-rx-file-path"  })) this->rx_filepath    = vals.at       ({p+"-rx-file-path"  });
	if (vals.exist({p+"-tx-file-path"  })) this->tx_filepath    = vals.at       ({p+"-tx-file-path"  });
	if (vals.exist({p+"-tx-subdev-spec"})) this->tx_subdev_spec = vals.at       ({p+"-tx-subdev-spec"});
	if (vals.exist({p+"-tx-ant"        })) this->tx_antenna     = vals.at       ({p+"-tx-ant"        });
	if (vals.exist({p+"-tx-rate"       })) this->tx_enabled     = true                                 ;
	if (vals.exist({p+"-tx-rate"       })) this->tx_rate        = vals.to_float ({p+"-tx-rate"       });
	if (vals.exist({p+"-tx-freq"       })) this->tx_freq        = vals.to_float ({p+"-tx-freq"       });
	if (vals.exist({p+"-tx-gain"       })) this->tx_gain        = vals.to_float ({p+"-tx-gain"       });
	if (vals.exist({p+"-serial"        })) this->serial         = vals.at       ({p+"-serial"        });
	if (vals.exist({p+"-rx-no-loop"    })) this->rx_no_loop     = true                                 ;
	if (vals.exist({p+"-rx-pin-core"   })) this->rx_pin_core    = vals.to_int   ({p+"-rx-pin-core"   });
	if (vals.exist({p+"-tx-pin-core"   })) this->tx_pin_core    = vals.to_int   ({p+"-tx-pin-core"   });
	if (vals.exist({p+"-clock-source"  })) this->clock_source   = vals.at       ({p+"-clock-source"  });

	this->validate();
}

void Radio
::validate() const
{
	// B200-mini clock rate limit: 61.44 MHz
	if (this->clk_rate > 61.44e6)
	{
		std::stringstream message;
		message << "B200-mini maximum master clock rate is 61.44 MHz, but clk_rate="
		        << this->clk_rate << " was requested.";
		throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	// B200-mini RX sample rate limit: 56 MHz
	if (this->rx_rate > 56e6)
	{
		std::stringstream message;
		message << "B200-mini maximum RX sample rate is 56 MHz, but rx_rate="
		        << this->rx_rate << " was requested.";
		throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	// B200-mini TX sample rate limit: 56 MHz
	if (this->tx_rate > 56e6)
	{
		std::stringstream message;
		message << "B200-mini maximum TX sample rate is 56 MHz, but tx_rate="
		        << this->tx_rate << " was requested.";
		throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	// Warn if combined RX+TX rate exceeds USB3 bandwidth (~56 Msps aggregate)
	if (this->rx_rate > 0 && this->tx_rate > 0 && (this->rx_rate + this->tx_rate) > 56e6)
	{
		std::cerr << "[WARNING] B200-mini full-duplex: combined rx_rate + tx_rate = "
		          << (this->rx_rate + this->tx_rate) / 1e6 << " MHz exceeds recommended "
		          << "USB3 bandwidth limit of ~56 Msps aggregate. Overflows may occur."
		          << std::endl;
	}

	// B200-mini does not support external clock source
	if (this->clock_source == "external")
	{
		std::stringstream message;
		message << "B200-mini does not support an external 10 MHz reference clock. "
		        << "Use \"internal\" or \"gpsdo\" (if GPSDO module is installed).";
		throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	// Warn if antenna is not one of the B200-mini available ports
	if (!this->rx_antenna.empty() &&
	    this->rx_antenna != "TX/RX" && this->rx_antenna != "RX2")
	{
		std::cerr << "[WARNING] B200-mini antenna port \"" << this->rx_antenna
		          << "\" is not recognized. Available ports: TX/RX, RX2."
		          << std::endl;
	}
	if (!this->tx_antenna.empty() &&
	    this->tx_antenna != "TX/RX" && this->tx_antenna != "RX2")
	{
		std::cerr << "[WARNING] B200-mini antenna port \"" << this->tx_antenna
		          << "\" is not recognized. Available ports: TX/RX, RX2."
		          << std::endl;
	}
}

std::string Radio
::build_device_string() const
{
	std::string device_str = "type=b200";

	if (!this->serial.empty())
		device_str += ",serial=" + this->serial;

	if (this->clk_rate != 0)
		device_str += ",master_clock_rate=" + std::to_string(this->clk_rate);

	return device_str;
}

std::string Radio
::get_effective_rx_subdev_spec() const
{
	if (this->rx_subdev_spec.empty())
		return "A:A";
	return this->rx_subdev_spec;
}

std::string Radio
::get_effective_tx_subdev_spec() const
{
	if (this->tx_subdev_spec.empty())
		return "A:A";
	return this->tx_subdev_spec;
}

bool Radio
::is_pin_core_valid(int pin_core) const
{
	unsigned int n_cores = std::thread::hardware_concurrency();
	return pin_core >= 0 && (n_cores == 0 || static_cast<unsigned int>(pin_core) < n_cores);
}

void Radio
::get_headers(std::map<std::string,tools::header_list>& headers, const bool full) const
{
	auto p = this->get_prefix();

	headers[p].push_back(std::make_pair("N. samples", std::to_string(this->N)));
	headers[p].push_back(std::make_pair("Type      ", this->type             ));
	if (this->type == "USRP")
	{
		headers[p].push_back(std::make_pair("B200 Serial    ", this->serial               ));
		headers[p].push_back(std::make_pair("B200 Clk rate  ", std::to_string(this->clk_rate )));
		headers[p].push_back(std::make_pair("B200 Threaded  ", this->threaded ? "YES" : "NO"  ));
		headers[p].push_back(std::make_pair("B200 Fifo size ", std::to_string(this->fifo_size)));
		headers[p].push_back(std::make_pair("B200 Rx rate   ", std::to_string(this->rx_rate  )));
		headers[p].push_back(std::make_pair("B200 Rx subdev ", get_effective_rx_subdev_spec()  ));
		headers[p].push_back(std::make_pair("B200 Rx antenna", this->rx_antenna               ));
		headers[p].push_back(std::make_pair("B200 Rx freq   ", std::to_string(this->rx_freq  )));
		headers[p].push_back(std::make_pair("B200 Rx gain   ", std::to_string(this->rx_gain  )));
		headers[p].push_back(std::make_pair("B200 Rx File   ", this->rx_filepath              ));
		headers[p].push_back(std::make_pair("B200 Rx no loop", this->rx_no_loop ? "YES" : "NO"));
		headers[p].push_back(std::make_pair("B200 Tx File   ", this->tx_filepath              ));
		headers[p].push_back(std::make_pair("B200 Tx subdev ", get_effective_tx_subdev_spec()  ));
		headers[p].push_back(std::make_pair("B200 Tx antenna", this->tx_antenna               ));
		headers[p].push_back(std::make_pair("B200 Tx rate   ", std::to_string(this->tx_rate  )));
		headers[p].push_back(std::make_pair("B200 Tx freq   ", std::to_string(this->tx_freq  )));
		headers[p].push_back(std::make_pair("B200 Tx gain   ", std::to_string(this->tx_gain  )));
		headers[p].push_back(std::make_pair("B200 Clock src ", this->clock_source             ));
		headers[p].push_back(std::make_pair("B200 Rx pin    ", std::to_string(this->rx_pin_core)));
		headers[p].push_back(std::make_pair("B200 Tx pin    ", std::to_string(this->tx_pin_core)));
	}
}

template <typename R>
module::Radio<R>* Radio
::build() const
{
	if (this->type == "NO")
		return new module::Radio_NO<R>(this->N, this->n_frames);
	else if (this->type == "USER_BIN")
		return new module::Radio_user_binary<R>(this->N, this->rx_filepath, this->tx_filepath, !this->rx_no_loop,
			                                    this->n_frames);
	#ifdef DVBS2_LINK_UHD
	else if (this->type == "USRP")
		return new module::Radio_USRP<R>(*this);
	#endif

	throw spu::tools::cannot_allocate(__FILE__, __LINE__, __func__);
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
template aff3ct::module::Radio<double>*      aff3ct::factory::Radio::build<double     >() const;
template aff3ct::module::Radio<float>*       aff3ct::factory::Radio::build<float      >() const;
template aff3ct::module::Radio<short>*       aff3ct::factory::Radio::build<short      >() const;
template aff3ct::module::Radio<signed char>* aff3ct::factory::Radio::build<signed char>() const;
// ==================================================================================== explicit template instantiation
