# Design Document

## Overview

This design describes the changes needed to support the Ettus B200-mini USB SDR in the existing DVB-S2 transceiver codebase. The architecture already supports parameterized USRP configuration via `factory::Radio` and CLI arguments. The migration requires: (1) B200-mini-aware device string construction, (2) parameter validation for the B200-mini's hardware constraints, (3) configurable thread pinning, (4) clock/time source management, (5) USB error handling, and (6) updated documentation.

The design preserves backward compatibility — all changes are additive or conditional on `usrp_type == "b200"`. No new CMake options or link libraries are required.

## Architecture

### Component Interaction

```
CLI Arguments
     │
     ▼
┌─────────────────────────┐
│   factory::Radio        │  ← Validation layer (new B200 constraints)
│   (Radio.hpp/cpp)       │
└────────────┬────────────┘
             │ build()
             ▼
┌─────────────────────────┐
│   Radio_USRP            │  ← Device string construction, clock source,
│   (Radio_USRP.cpp)      │     thread pinning (parameterized)
└────────────┬────────────┘
             │ multi_usrp::make()
             ▼
┌─────────────────────────┐
│   UHD Library           │  ← Discovers B200 over USB or Network USRP over IP
└─────────────────────────┘
```

### Key Design Decisions

1. **Single binary, runtime branching**: All B200-specific behavior is gated by `params.usrp_type == "b200"` checks at runtime, not compile-time. This avoids a separate build target.

2. **Validation in Factory**: Parameter validation (clock rate, sample rate) happens in `Radio::store()` after parsing, before `build()` is called. This fails early with a clear message.

3. **Device string logic in Radio_USRP constructor**: The existing device string construction already handles optional fields. We extend it to map `usrp_addr` → `serial=` for B200 and omit `addr=` entirely.

4. **Thread pinning parameterized via factory**: New `rx_pin_core` and `tx_pin_core` fields in the factory struct replace hardcoded values in Radio_USRP.cpp.

## File Changes

### 1. `src/common/Factory/Module/Radio/Radio.hpp`

**Changes:**
- Add new fields: `rx_pin_core` (int, default 1), `tx_pin_core` (int, default 3), `clock_source` (string, default "internal").
- These are stored alongside existing parameters.

```cpp
// New fields in struct Radio:
int rx_pin_core            = 1;
int tx_pin_core            = 3;
std::string clock_source   = "internal"; // "internal", "gpsdo", "external"
```

### 2. `src/common/Factory/Module/Radio/Radio.cpp`

**Changes in `get_description()`:**
- Register `--rad-rx-pin-core`, `--rad-tx-pin-core`, `--rad-clock-source`.

**Changes in `store()`:**
- Parse the new CLI parameters.
- Add a validation block after all parameters are parsed:
  - If `usrp_type == "b200"`:
    - Reject `clk_rate > 61.44e6`.
    - Reject `rx_rate > 56e6` or `tx_rate > 56e6`.
    - Warn if `rx_rate + tx_rate > 56e6` (full-duplex throughput limit).
    - Warn if `clock_source == "external"` (not supported).
    - Warn if user-specified antenna not in {"TX/RX", "RX2"}.

**Changes in `get_headers()`:**
- Print clock source, pin cores, and any B200-specific info when `usrp_type` contains "b200".

### 3. `src/common/Module/Radio/Radio_USRP/Radio_USRP.cpp`

**Changes in constructor:**

- **Device string construction**: When `usrp_type` contains "b200":
  - If `usrp_addr` is non-empty, use `serial=<usrp_addr>` instead of `addr=<usrp_addr>`.
  - If `usrp_addr` is empty, rely on UHD auto-discovery (no addr/serial in string).

- **Subdevice spec defaulting**: After `multi_usrp::make()`:
  - If `usrp_type == "b200"` and `rx_subdev_spec` is empty → use "A:A".
  - If `usrp_type == "b200"` and `tx_subdev_spec` is empty → use "A:A".

- **Clock source configuration**: After `multi_usrp::make()`:
  - Call `usrp->set_clock_source(params.clock_source)`.
  - If `clock_source == "gpsdo"`, also call `usrp->set_time_source("gpsdo")`.

**Changes in `thread_function_send()` and `thread_function_receive()`:**
- Replace hardcoded `pin(3)` and `pin(1)` with `pin(params.tx_pin_core)` and `pin(params.rx_pin_core)`.
- This requires storing pin core values as member variables (set from constructor params).

**Changes in error handling (`receive_usrp`):**
- Add handling for `ERROR_CODE_BAD_PACKET` and unknown transport errors: log a message suggesting USB disconnection, then set `stop_threads = true` for graceful shutdown.

### 4. `src/common/Module/Radio/Radio_USRP/Radio_USRP.hpp`

**Changes:**
- Add member variables: `int rx_pin_core`, `int tx_pin_core`.

### 5. `README.md`

**Changes:**
- Add a "B200-mini Setup" section after the existing "Ethernet configuration" section.
- Include: USB udev rules, UHD image download for B200, example benchmark command.
- Add B200-mini TX/RX example command lines using `--rad-usrp-type b200` with sample rates ≤ 30.72 MHz and `--rad-rx-subdev-spec "A:A"`.
- Document master clock / sample rate relationship.

### 6. `CMakeLists.txt`

**No changes required.** The existing UHD linkage and `DVBS2_LINK_UHD` guard already support B200-mini since UHD handles both device families. No additional libraries are needed.

## Correctness Properties

### Property 1: Device String Construction (Req 1)

**Property**: For all combinations of `usrp_type` and `usrp_addr`, the constructed Device_String satisfies:
- If `usrp_type == "b200"` and `usrp_addr` is empty → string contains "type=b200" and does NOT contain "addr=" or "serial=".
- If `usrp_type == "b200"` and `usrp_addr` is non-empty → string contains "type=b200" and "serial=<addr>" and does NOT contain "addr=".
- If `usrp_type != "b200"` and `usrp_addr` is non-empty → string contains "addr=<addr>" (existing behavior preserved).

### Property 2: Clock Rate Validation Boundary (Req 2)

**Property**: For all `clk_rate` values when `usrp_type == "b200"`:
- `clk_rate > 61.44e6` → validation error raised.
- `0 < clk_rate <= 61.44e6` → no validation error, value passed through.
- `clk_rate == 0` → no validation error, master_clock_rate omitted from device string.

### Property 3: Sample Rate Validation Boundary (Req 3)

**Property**: For all `rx_rate` and `tx_rate` values when `usrp_type == "b200"`:
- `rx_rate > 56e6` → validation error.
- `tx_rate > 56e6` → validation error.
- `rx_rate <= 56e6` and `tx_rate <= 56e6` → no error (warning possible if combined > 56e6).

### Property 4: Subdevice Spec Defaulting Idempotence (Req 4)

**Property**: For all configurations where `usrp_type == "b200"`:
- If user provides `rx_subdev_spec = X` (non-empty), the effective value is X (user override preserved).
- If user provides empty `rx_subdev_spec`, the effective value is "A:A".
- Applying the defaulting logic twice produces the same result (idempotent).

### Property 5: Thread Pin Core Parameterization (Req 6)

**Property**: For all valid `rx_pin_core` and `tx_pin_core` values:
- The RX thread is pinned to `rx_pin_core` (not hardcoded 1).
- The TX thread is pinned to `tx_pin_core` (not hardcoded 3).
- If core ID >= number of available cores → thread is not pinned (graceful fallback).

### Property 6: Backward Compatibility (Req 10)

**Property**: For all configurations where `usrp_type != "b200"`:
- The Device_String construction produces identical output to the current implementation.
- No new validation errors are raised.
- Thread pinning defaults remain cores 1 and 3.
- Subdevice spec is not modified when empty (existing network USRP behavior uses UHD defaults or user-specified values).

## Error Handling

| Scenario | Detection | Response |
|----------|-----------|----------|
| clk_rate > 61.44 MHz (B200) | `store()` validation | Throw `runtime_error` with message |
| rx/tx_rate > 56 MHz (B200) | `store()` validation | Throw `runtime_error` with message |
| External clock on B200 | `store()` validation | Throw `runtime_error` with message |
| Invalid antenna name (B200) | `store()` validation | Log warning, continue |
| Combined rate > 56 MHz | `store()` validation | Log warning, continue |
| USB disconnection | `receive_usrp()`/`send_usrp()` UHD error | Log error, set `stop_threads=true` |
| Pin core exceeds CPU count | `thread_function_*()` | Log warning, skip pinning |
| No B200 device found | UHD `multi_usrp::make()` exception | Re-throw with descriptive message |

## Testing Strategy

- **Unit tests for device string construction**: Extract device string building into a testable helper function. Verify all combinations of usrp_type/addr/clk_rate produce correct strings.
- **Unit tests for parameter validation**: Call `store()` with out-of-range values for B200 and verify exceptions.
- **Integration test with hardware**: Run `benchmark_rate` with B200-mini at 30.72 MHz to verify streaming works end-to-end.
- **Backward compatibility test**: Run existing command lines for network USRP (from README) against the updated binary — behavior must be unchanged.
