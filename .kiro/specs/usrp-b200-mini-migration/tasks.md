# Tasks

## Task 1: Add New Factory Parameters and CLI Arguments

> **Requirements**: Req 6 (Thread Pinning), Req 7 (Clock Source)
> **File**: `src/common/Factory/Module/Radio/Radio.hpp`, `src/common/Factory/Module/Radio/Radio.cpp`

- [x] 1.1 Add `rx_pin_core` (int, default 1), `tx_pin_core` (int, default 3), and `clock_source` (string, default "internal") fields to the `Radio` struct in `Radio.hpp`
- [x] 1.2 Register `--rad-rx-pin-core`, `--rad-tx-pin-core`, and `--rad-clock-source` in `get_description()`
- [x] 1.3 Parse the new parameters in `store()` method
- [x] 1.4 Display the new parameters in `get_headers()` when type is USRP

## Task 2: Add B200-mini Parameter Validation in Factory

> **Requirements**: Req 2 (Clock Rate), Req 3 (Sample Rate), Req 5 (Antenna), Req 7 (Clock Source)
> **File**: `src/common/Factory/Module/Radio/Radio.cpp`

- [x] 2.1 Add validation block at the end of `store()` that checks constraints when `usrp_type == "b200"`
- [x] 2.2 Reject `clk_rate > 61.44e6` with descriptive error message
- [x] 2.3 Reject `rx_rate > 56e6` with descriptive error message
- [x] 2.4 Reject `tx_rate > 56e6` with descriptive error message
- [x] 2.5 Log warning when `rx_rate + tx_rate > 56e6` (full-duplex USB bandwidth concern)
- [x] 2.6 Reject `clock_source == "external"` for B200 with error stating external ref not supported
- [x] 2.7 Log warning when user-specified antenna is not in {"TX/RX", "RX2"} for B200
- [x] 2.8 Add a CMake test target (`dvbs2_test_radio_validation`) using a lightweight test runner (e.g., a standalone `main()` with assertions) in `tests/test_radio_b200_validation.cpp`
- [x] 2.9 Write unit tests verifying: clk_rate > 61.44e6 throws for B200, clk_rate <= 61.44e6 passes, rx_rate > 56e6 throws, tx_rate > 56e6 throws, clock_source "external" throws, and non-B200 type skips all validation
- [x] 2.10 Write unit tests verifying default values from Task 1: rx_pin_core == 1, tx_pin_core == 3, clock_source == "internal"

## Task 3: Update Device String Construction for B200-mini

> **Requirements**: Req 1 (USB Discovery)
> **File**: `src/common/Module/Radio/Radio_USRP/Radio_USRP.cpp`

- [x] 3.1 Modify device string construction: when `usrp_type` is "b200" and `usrp_addr` is non-empty, use `serial=<usrp_addr>` instead of `addr=<usrp_addr>`
- [x] 3.2 When `usrp_type` is "b200" and `usrp_addr` is empty, omit both `addr=` and `serial=` from device string (USB auto-discovery)
- [x] 3.3 Wrap `multi_usrp::make()` in try-catch and re-throw with descriptive message when B200-mini is not found
- [x] 3.4 Extract device string construction into a testable helper method `std::string build_device_string() const;` in Radio_USRP (or as a free function) so it can be unit tested without UHD hardware
- [x] 3.5 Write unit tests in `tests/test_radio_device_string.cpp` verifying: B200 + empty addr → "type=b200" only (no addr/serial), B200 + serial "ABC123" → "type=b200,serial=ABC123" (no addr=), non-B200 + addr "192.168.20.2" → "addr=192.168.20.2" (backward compat), and clk_rate inclusion/omission
- [x] 3.6 Add CMake test target `dvbs2_test_radio_device_string` and register with CTest

## Task 4: Add Subdevice Spec Defaulting for B200-mini

> **Requirements**: Req 4 (Subdevice Spec Defaults)
> **File**: `src/common/Module/Radio/Radio_USRP/Radio_USRP.cpp`

- [x] 4.1 After `multi_usrp::make()`, if `usrp_type == "b200"` and `rx_subdev_spec` is empty, set rx subdev to "A:A"
- [x] 4.2 After `multi_usrp::make()`, if `usrp_type == "b200"` and `tx_subdev_spec` is empty, set tx subdev to "A:A"
- [x] 4.3 Ensure user-provided subdev specs are not overwritten (check before defaulting)

## Task 5: Add Clock and Time Source Configuration

> **Requirements**: Req 7 (Clock Reference)
> **File**: `src/common/Module/Radio/Radio_USRP/Radio_USRP.cpp`

- [x] 5.1 Store `clock_source` from params in Radio_USRP constructor
- [x] 5.2 After `multi_usrp::make()`, call `usrp->set_clock_source(clock_source)`
- [x] 5.3 If `clock_source == "gpsdo"`, also call `usrp->set_time_source("gpsdo")`

## Task 6: Parameterize Thread Pinning

> **Requirements**: Req 6 (Thread Pinning)
> **File**: `src/common/Module/Radio/Radio_USRP/Radio_USRP.hpp`, `src/common/Module/Radio/Radio_USRP/Radio_USRP.cpp`

- [x] 6.1 Add `int rx_pin_core` and `int tx_pin_core` member variables to `Radio_USRP` class
- [x] 6.2 Initialize pin core members from `params.rx_pin_core` and `params.tx_pin_core` in constructor
- [x] 6.3 Replace hardcoded `pin(1)` in `thread_function_receive()` with `pin(rx_pin_core)`
- [x] 6.4 Replace hardcoded `pin(3)` in `thread_function_send()` with `pin(tx_pin_core)`
- [x] 6.5 Add guard: if pin core >= number of available cores, log warning and skip pinning

## Task 7: Add USB Disconnection Error Handling

> **Requirements**: Req 9 (Graceful USB Disconnection)
> **File**: `src/common/Module/Radio/Radio_USRP/Radio_USRP.cpp`

- [x] 7.1 In `receive_usrp()`, add a catch for transport-related UHD errors (e.g., ERROR_CODE_BAD_PACKET or repeated timeouts) and log a descriptive USB disconnection message
- [x] 7.2 On detected USB disconnection, set `stop_threads = true` to trigger graceful shutdown
- [x] 7.3 In `send_usrp()`, wrap `tx_stream->send()` in try-catch for UHD transport exceptions and handle similarly

## Task 8: Update README with B200-mini Documentation

> **Requirements**: Req 8 (Documentation)
> **File**: `README.md`

- [x] 8.1 Add "B200-mini Setup" section after "Ethernet configuration" with USB udev rules and UHD image download instructions
- [x] 8.2 Add B200-mini benchmark command example using `--args "type=b200,master_clock_rate=30.72e6"`
- [x] 8.3 Add B200-mini TX example: `./bin/dvbs2_tx --rad-usrp-type b200 --rad-tx-subdev-spec "A:A" --rad-tx-rate 30.72e6 --rad-tx-freq 2360e6 --rad-tx-gain 30 --rad-threaded ...`
- [x] 8.4 Add B200-mini RX example: `./bin/dvbs2_rx --rad-usrp-type b200 --rad-rx-subdev-spec "A:A" --rad-rx-rate 30.72e6 --rad-rx-freq 2360e6 --rad-rx-gain 20 --rad-threaded ...`
- [x] 8.5 Add note documenting `sample_rate = master_clock_rate / N` relationship and recommended rates for B200-mini (e.g., 30.72e6, 15.36e6, 7.68e6)

## Task 9: Verify Build and Backward Compatibility

> **Requirements**: Req 10 (Build System)

- [x] 9.1 Verify project compiles with `DVBS2_LINK_UHD=ON` — no new warnings or errors from B200-specific code
- [x] 9.2 Verify project compiles with `DVBS2_LINK_UHD=OFF` — B200 code properly excluded by `#ifdef`
- [x] 9.3 Verify existing README network USRP command lines still function with unchanged behavior (no regressions in device string construction for non-B200 types)
- [x] 9.4 Run `ctest` and verify `dvbs2_test_radio_validation` passes (all B200 parameter validation tests)
- [x] 9.5 Run `ctest` and verify `dvbs2_test_radio_device_string` passes (all device string construction tests)
- [x] 9.6 Run `ctest` and verify `dvbs2_test_radio_subdev_spec` passes (all subdevice spec defaulting tests)
- [x] 9.7 Run `ctest` and verify `dvbs2_test_radio_clock_source` passes (all clock source configuration tests)
