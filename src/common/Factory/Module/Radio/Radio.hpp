#ifndef FACTORY_RADIO_HPP_
#define FACTORY_RADIO_HPP_

#include <string>
#include <memory>
#include <aff3ct.hpp>

#include "Module/Radio/Radio.hpp"

namespace aff3ct
{
namespace factory
{
extern const std::string Radio_name;
extern const std::string Radio_prefix;
struct Radio : Factory
{
public:
	// ------------------------------------------------------------------------------------------------- PARAMETERS
	// required parameters

	// optional parameters
	int N                      = 0;
	bool threaded              = false;
	uint64_t fifo_size         = uint64_t(10000000000);
	int n_frames               = 1;
	std::string type           = "USRP";
	std::string usrp_addr      = ""; // 192.168.20.2
	std::string usrp_type      = ""; // b200
	double clk_rate            = 0; // 125e6

	bool rx_enabled            = false;
	double rx_rate             = 0; // if rx_rate is not overriden, rx is disabled
	std::string rx_subdev_spec = ""; // A:0
	std::string rx_antenna     = "RX2";
	double rx_freq             = 1090e6;
	double rx_gain             = 10;
	std::string rx_filepath    = "";
	std::string tx_filepath    = "";
	bool   rx_no_loop          = false;

	bool tx_enabled            = false;
	double tx_rate             = 0; // if tx_rate is not overriden, tx is disabled
	std::string tx_subdev_spec = ""; // A:0
	std::string tx_antenna     = "TX/RX";
	double tx_freq             = 1090e6;
	double tx_gain             = 10;

	int rx_pin_core            = 1;
	int tx_pin_core            = 3;
	std::string clock_source   = "internal"; // "internal", "gpsdo", "external"

	// deduced parameters

	// ---------------------------------------------------------------------------------------------------- METHODS
	explicit Radio(const std::string &p = Radio_prefix);
	virtual ~Radio() = default;
	Radio* clone() const;

	// parameters construction
	virtual void get_description(cli::Argument_map_info &args) const;
	virtual void store          (const cli::Argument_map_value &vals);
	virtual void get_headers    (std::map<std::string,tools::header_list>& headers, const bool full = true) const;

	// B200-mini parameter validation (callable independently of store())
	void validate() const;

	// Build the UHD device string from current parameters (testable without hardware)
	std::string build_device_string() const;

	// Get effective subdev specs after B200 defaulting (testable without hardware)
	std::string get_effective_rx_subdev_spec() const;
	std::string get_effective_tx_subdev_spec() const;

	// Returns true if the time source should also be set to "gpsdo" (testable without hardware)
	bool should_set_time_source_gpsdo() const { return clock_source == "gpsdo"; }

	// Returns true if the given pin_core value is valid (non-negative and less than available cores)
	bool is_pin_core_valid(int pin_core) const;

	template <typename R = float>
	module::Radio<R>* build() const;
};
}
}

#endif /* FACTORY_RADIO_HPP_ */

