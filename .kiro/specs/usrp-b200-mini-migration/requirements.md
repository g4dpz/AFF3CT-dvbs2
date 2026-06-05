# Requirements Document

## Introduction

This feature migrates the AFF3CT DVB-S2 software transceiver from a network-connected Ettus USRP (N210/N310/X310) to an Ettus B200-mini USB-connected SDR. The B200-mini has different hardware constraints: USB3 transport (no IP address), lower maximum master clock rate (61.44 MHz), a single channel with subdevice spec "A:A", limited bandwidth (~56 MHz max sample rate), a single TX/RX and RX2 antenna port, and no external 10 MHz reference or PPS by default. The migration requires updating default parameters, adding parameter validation, adjusting thread pinning for lower-core-count systems typical of portable B200-mini setups, and updating documentation.

## Glossary

- **Radio_USRP**: The AFF3CT module class that interfaces with Ettus USRP hardware via the UHD library for transmitting and receiving IQ samples.
- **Radio_Factory**: The factory class (`factory::Radio`) responsible for constructing Radio module instances from CLI parameters.
- **UHD**: USRP Hardware Driver, the Ettus-provided library for controlling USRP devices.
- **B200_mini**: The Ettus B200-mini, a USB3-connected single-channel SDR with AD9364 RF frontend, max 61.44 MHz master clock, and ~56 MHz instantaneous bandwidth.
- **Network_USRP**: A network-connected Ettus USRP (N210, N310, X310) that uses Ethernet and IP-based addressing.
- **Master_Clock_Rate**: The FPGA master clock frequency of the USRP device, which constrains achievable sample rates to integer divisors of this value.
- **Subdevice_Spec**: A UHD string specifying which daughterboard and channel to use (e.g., "A:0" for network USRPs, "A:A" for B200-series).
- **Thread_Pinning**: The mechanism in StreamPU that pins processing threads to specific CPU cores for real-time performance.
- **Device_String**: The UHD device argument string passed to `multi_usrp::make()` containing type, address, and clock rate parameters.

## Requirements

### Requirement 1: USB Device Discovery Without IP Address

**User Story:** As a developer, I want the Radio_USRP module to discover and connect to a B200-mini over USB without requiring an IP address, so that I can use the device with its native USB3 transport.

#### Acceptance Criteria

1. WHEN the usrp_type parameter is set to "b200" and the usrp_addr parameter is empty, THE Radio_USRP SHALL construct a Device_String without an "addr=" field and discover the device over USB.
2. WHEN the usrp_type parameter is set to "b200" and usrp_addr is non-empty, THE Radio_USRP SHALL include the "serial=" key with the usrp_addr value in the Device_String to allow selection of a specific B200-mini by serial number.
3. WHEN no B200-mini device is found during USB discovery, THE Radio_USRP SHALL throw a descriptive runtime error indicating that no B200-mini device was detected on USB.

### Requirement 2: Master Clock Rate Validation for B200-mini

**User Story:** As a developer, I want the system to validate that the requested master clock rate is within the B200-mini's supported range, so that I avoid silent misconfiguration or UHD errors at runtime.

#### Acceptance Criteria

1. WHEN the usrp_type is "b200" and the clk_rate parameter exceeds 61.44 MHz, THE Radio_Factory SHALL reject the configuration and report an error stating the maximum supported master clock rate.
2. WHEN the usrp_type is "b200" and the clk_rate parameter is zero, THE Radio_USRP SHALL omit the master_clock_rate from the Device_String, allowing UHD to use the B200-mini default clock rate.
3. WHEN the usrp_type is "b200" and the clk_rate parameter is within the range (0, 61.44e6], THE Radio_USRP SHALL set the master_clock_rate to the specified value.

### Requirement 3: Sample Rate Validation for B200-mini

**User Story:** As a developer, I want the system to validate that the requested TX and RX sample rates are achievable on the B200-mini, so that I avoid overflows caused by exceeding USB3 bandwidth.

#### Acceptance Criteria

1. WHEN the usrp_type is "b200" and the rx_rate exceeds 56e6, THE Radio_Factory SHALL reject the configuration and report an error stating the maximum supported sample rate for the B200-mini.
2. WHEN the usrp_type is "b200" and the tx_rate exceeds 56e6, THE Radio_Factory SHALL reject the configuration and report an error stating the maximum supported sample rate for the B200-mini.
3. WHEN both rx_rate and tx_rate are enabled simultaneously on the B200-mini, THE Radio_Factory SHALL validate that the combined sample rate does not exceed the USB3 throughput limit of approximately 56 Msps aggregate and report a warning if it is close to the limit.

### Requirement 4: Subdevice Specification Defaults for B200-mini

**User Story:** As a developer, I want the default subdevice spec to be automatically set to "A:A" when using a B200-mini, so that I do not need to manually specify the correct subdevice string.

#### Acceptance Criteria

1. WHEN the usrp_type is "b200" and rx_subdev_spec is empty, THE Radio_USRP SHALL use "A:A" as the RX subdevice specification.
2. WHEN the usrp_type is "b200" and tx_subdev_spec is empty, THE Radio_USRP SHALL use "A:A" as the TX subdevice specification.
3. WHEN the usrp_type is "b200" and the user explicitly provides a subdev_spec value, THE Radio_USRP SHALL use the user-provided value without override.

### Requirement 5: Antenna Port Configuration for B200-mini

**User Story:** As a developer, I want the antenna port defaults to match B200-mini hardware, so that the transmitter and receiver connect to the correct RF ports.

#### Acceptance Criteria

1. THE Radio_Factory SHALL default the rx_antenna to "RX2" for B200-mini configurations.
2. THE Radio_Factory SHALL default the tx_antenna to "TX/RX" for B200-mini configurations.
3. WHEN a user specifies an antenna name that is not "TX/RX" or "RX2" for a B200-mini, THE Radio_Factory SHALL report a warning indicating the available antenna ports on the B200-mini.

### Requirement 6: Thread Pinning Configuration for B200-mini Deployments

**User Story:** As a developer, I want configurable thread pinning core IDs, so that I can run the system on machines with fewer CPU cores than the current hardcoded assumptions.

#### Acceptance Criteria

1. THE Radio_Factory SHALL expose CLI parameters `--rad-rx-pin-core` and `--rad-tx-pin-core` to allow the user to specify which CPU cores the RX and TX threads are pinned to.
2. WHEN `--rad-rx-pin-core` is not specified, THE Radio_USRP SHALL default the RX thread pin to core 1.
3. WHEN `--rad-tx-pin-core` is not specified, THE Radio_USRP SHALL default the TX thread pin to core 3.
4. WHEN a specified pin core ID exceeds the number of available CPU cores, THE Radio_USRP SHALL report a warning and fall back to not pinning that thread.

### Requirement 7: Clock Reference and Time Source Configuration

**User Story:** As a developer, I want to configure the clock and time source references for the B200-mini, so that I can use internal references or an optional GPSDO when available.

#### Acceptance Criteria

1. WHEN the usrp_type is "b200", THE Radio_USRP SHALL set the clock source to "internal" by default.
2. WHEN the usrp_type is "b200" and a `--rad-clock-source` parameter is set to "gpsdo", THE Radio_USRP SHALL configure the B200-mini to use its GPSDO module as the clock and time reference.
3. WHEN the usrp_type is "b200" and the user requests an "external" clock source, THE Radio_USRP SHALL report an error indicating that the B200-mini does not support an external 10 MHz reference without a GPSDO.

### Requirement 8: Updated Documentation and Example Commands

**User Story:** As a developer, I want the README and inline documentation to include B200-mini-specific usage examples, so that new users can quickly set up and run the system.

#### Acceptance Criteria

1. THE README SHALL include a "B200-mini Setup" section with USB connection instructions and udev rule installation steps.
2. THE README SHALL include example TX and RX command lines using B200-mini parameters with sample rates at or below 30.72 MHz and subdev spec "A:A".
3. THE README SHALL document the relationship between master clock rate and achievable sample rates on the B200-mini (sample_rate = master_clock / integer_divisor).

### Requirement 9: Graceful Handling of USB Disconnection

**User Story:** As a developer, I want the system to handle unexpected USB disconnection gracefully, so that the application terminates cleanly instead of crashing.

#### Acceptance Criteria

1. IF the B200-mini is disconnected during streaming, THEN THE Radio_USRP SHALL detect the connection loss via UHD error codes and signal a clean shutdown.
2. IF a UHD transport error occurs during send or receive on a B200-mini, THEN THE Radio_USRP SHALL log the error with a descriptive message indicating a likely USB disconnection before stopping the streaming threads.

### Requirement 10: Build System Compatibility

**User Story:** As a developer, I want the build system to continue supporting both network USRPs and the B200-mini with no additional CMake options required, so that a single binary supports all USRP types.

#### Acceptance Criteria

1. THE CMake build system SHALL produce a single binary that supports both Network_USRP and B200_mini hardware when DVBS2_LINK_UHD is ON.
2. THE CMake build system SHALL require no additional CMake options or link libraries beyond UHD and Boost to support the B200-mini.
3. WHEN DVBS2_LINK_UHD is OFF, THE build system SHALL compile without any B200-mini-specific code paths.
