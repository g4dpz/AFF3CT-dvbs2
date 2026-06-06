# Requirements Document

## Introduction

This document specifies the requirements for a new `dvbs2_loopback` binary that performs full-duplex DVB-S2 transmission and reception on a single Ettus B200-mini USB SDR. The binary enables hardware cable-loopback testing by transmitting DVB-S2 encoded IQ samples from the TX/RX port and simultaneously receiving them on the RX2 port through a physical attenuator, then decoding and measuring BER/FER performance. Unlike the existing `dvbs2_tx_rx` binary which uses a simulated internal channel, this binary exercises the full RF path including D/A conversion, analog filtering, cable propagation, and A/D conversion.

## Glossary

- **Loopback_Binary**: The `dvbs2_loopback` executable that performs simultaneous TX and RX on a single B200-mini device
- **Radio_USRP**: The existing module that interfaces with the Ettus B200-mini SDR via UHD, supporting simultaneous TX and RX streams on one `multi_usrp` instance
- **TX_Chain**: The DVB-S2 encoding pipeline consisting of source generation, BB scrambling, BCH encoding, LDPC encoding, interleaving, modulation, framing, PL scrambling, and pulse shaping
- **RX_Chain**: The DVB-S2 decoding pipeline consisting of AGC, coarse frequency synchronization, matched filtering, timing synchronization, frame synchronization, PL descrambling, fine frequency synchronization, PL header removal, noise estimation, demodulation, deinterleaving, LDPC decoding, BCH decoding, and BB descrambling
- **Waiting_Phase**: The initial phase where the RX synchronizer searches for frame lock on the received signal before error counting begins
- **Learning_Phase**: The phase after frame lock where synchronizer PLL coefficients are progressively tightened before steady-state operation
- **BER**: Bit Error Rate, the ratio of errored bits to total bits received
- **FER**: Frame Error Rate, the ratio of errored frames to total frames received
- **DVBS2_Factory**: The existing `factory::DVBS2` class that parses CLI arguments and constructs all TX/RX modules
- **StreamPU_Sequence**: The StreamPU runtime execution model that connects module tasks via socket bindings and executes them sequentially or in a pipeline

## Requirements

### Requirement 1: Single Radio Instance with Full-Duplex Operation

**User Story:** As an SDR developer, I want the loopback binary to open a single B200-mini with both TX and RX enabled simultaneously, so that I can test DVB-S2 over a real RF cable path without needing two separate devices.

#### Acceptance Criteria

1. THE Loopback_Binary SHALL instantiate exactly one Radio_USRP module with both `rx_enabled=true` and `tx_enabled=true`
2. WHEN the Radio_USRP module is instantiated, THE Loopback_Binary SHALL configure the TX antenna to "TX/RX" and the RX antenna to "RX2"
3. WHEN `--rad-threaded` is specified, THE Radio_USRP module SHALL use separate FIFO-based threads for TX streaming and RX streaming
4. WHEN `--rad-serial` is specified, THE Loopback_Binary SHALL target the B200-mini device with the given serial number

### Requirement 2: Complete TX Encoding Chain

**User Story:** As an SDR developer, I want the loopback binary to run the full DVB-S2 TX encoding chain, so that I can transmit standards-compliant waveforms for over-the-air testing.

#### Acceptance Criteria

1. THE TX_Chain SHALL process data through the following stages in order: source generation, BB scrambling, BCH encoding, LDPC encoding, interleaving, modulation, framing, PL scrambling, and shaping filter
2. THE TX_Chain SHALL use the same module construction methods from DVBS2_Factory as the existing `dvbs2_tx` binary
3. WHEN `--mod-cod` is specified, THE TX_Chain SHALL use the corresponding modulation and coding parameters for encoding
4. THE TX_Chain SHALL feed shaped IQ samples to the Radio_USRP module `send` task for transmission out the TX/RX port

### Requirement 3: Complete RX Decoding Chain

**User Story:** As an SDR developer, I want the loopback binary to run the full DVB-S2 RX decoding chain on received samples, so that I can verify end-to-end decode performance through real hardware.

#### Acceptance Criteria

1. THE RX_Chain SHALL process received IQ samples through the following stages in order: AGC, coarse frequency synchronization, matched filtering, timing synchronization, frame synchronization, PL descrambling, fine frequency synchronization (L&R and phase/frequency), PL header removal, noise estimation, demodulation, deinterleaving, LDPC decoding, BCH decoding, and BB descrambling
2. THE RX_Chain SHALL use the same module construction methods from DVBS2_Factory as the existing `dvbs2_rx` binary
3. THE RX_Chain SHALL receive IQ samples from the Radio_USRP module `receive` task reading from the RX2 port
4. WHEN `--dec-implem` and `--dec-ite` are specified, THE RX_Chain SHALL use the corresponding LDPC decoder implementation and iteration count

### Requirement 4: Synchronizer Waiting and Learning Phases

**User Story:** As an SDR developer, I want the loopback binary to execute waiting and learning phases before counting errors, so that the synchronizers can acquire lock on the real received signal before performance measurement begins.

#### Acceptance Criteria

1. WHEN the Loopback_Binary starts, THE RX_Chain SHALL enter a Waiting_Phase where the frame synchronizer searches for frame lock on the received signal
2. WHILE in the Waiting_Phase, THE TX_Chain SHALL continue transmitting frames to maintain a continuous signal for the receiver to lock onto
3. WHEN the frame synchronizer reports `packet_flag=true`, THE RX_Chain SHALL transition from Waiting_Phase to Learning_Phase
4. WHILE in the Learning_Phase, THE RX_Chain SHALL progressively tighten PLL coefficients for the coarse frequency synchronizer over two sub-phases (Learning Phase 1 and Learning Phase 2)
5. WHEN the Learning_Phase completes, THE RX_Chain SHALL transition to steady-state decoding mode where BER/FER errors are counted

### Requirement 5: BER and FER Reporting

**User Story:** As an SDR developer, I want the loopback binary to report BER and FER statistics comparing decoded data to the original source, so that I can measure the hardware link quality.

#### Acceptance Criteria

1. THE Loopback_Binary SHALL instantiate a Monitor_BFER module that compares decoded output bits to the original source bits
2. THE Loopback_Binary SHALL use a delay buffer to align the original source bits with the corresponding decoded bits accounting for the synchronizer pipeline delay
3. THE Loopback_Binary SHALL display cumulative BER, FER, bit error count, and frame error count via a terminal reporter updated periodically
4. THE Loopback_Binary SHALL display throughput in Mbps and elapsed time via a terminal reporter

### Requirement 6: Continuous Operation Until Interrupted

**User Story:** As an SDR developer, I want the loopback binary to run continuously until I press Ctrl+C, so that I can collect long-duration statistics.

#### Acceptance Criteria

1. THE Loopback_Binary SHALL register signal handlers for SIGINT and SIGTERM using the existing `spu::tools::Signal_handler::init()` mechanism
2. WHEN a SIGINT or SIGTERM signal is received, THE Loopback_Binary SHALL initiate graceful shutdown by stopping the StreamPU execution sequence
3. WHEN graceful shutdown is initiated, THE Radio_USRP module SHALL call `cancel_waiting()` to unblock any pending FIFO operations
4. THE Loopback_Binary SHALL disable the Monitor_BFER `is_done()` interface so the sequence does not terminate automatically

### Requirement 7: No Simulated Channel

**User Story:** As an SDR developer, I want the loopback binary to use only the physical cable path as the channel, so that test results reflect real hardware performance without synthetic noise.

#### Acceptance Criteria

1. THE Loopback_Binary SHALL NOT instantiate a simulated Channel module (no AWGN noise addition)
2. THE Loopback_Binary SHALL NOT instantiate simulated frequency shift, fading, or fractional/integer delay channel impairment modules
3. THE RX_Chain SHALL connect the Radio_USRP `receive` output directly to the front-end AGC input of the synchronization chain

### Requirement 8: CLI Argument Compatibility

**User Story:** As an SDR developer, I want the loopback binary to accept the same CLI arguments as the existing TX and RX binaries, so that I can reuse my existing scripts and parameter sets.

#### Acceptance Criteria

1. THE Loopback_Binary SHALL accept all radio arguments: `--rad-clk-rate`, `--rad-serial`, `--rad-tx-rate`, `--rad-rx-rate`, `--rad-tx-freq`, `--rad-rx-freq`, `--rad-tx-gain`, `--rad-rx-gain`, `--rad-threaded`, `--rad-tx-ant`, `--rad-rx-ant`
2. THE Loopback_Binary SHALL accept source arguments: `--src-type`, `--src-path`, `-F` (n_frames)
3. THE Loopback_Binary SHALL accept coding arguments: `--mod-cod`, `--dec-implem`, `--dec-ite`, `--dec-simd`
4. THE Loopback_Binary SHALL parse all arguments through the existing `factory::DVBS2` constructor
5. WHEN `--rad-tx-rate` and `--rad-rx-rate` are both specified, THE Loopback_Binary SHALL enable both TX and RX paths on the Radio_USRP module

### Requirement 9: Build System Integration

**User Story:** As a developer, I want the loopback binary to be built alongside the other executables via CMake, so that it is part of the standard build workflow.

#### Acceptance Criteria

1. THE CMakeLists.txt SHALL define a new executable target named `dvbs2_loopback` with its source file at `src/mains/LOOPBACK/main.cpp`
2. THE `dvbs2_loopback` target SHALL link against `dvbs2_common` and `aff3ct-static-lib`
3. THE `dvbs2_loopback` target SHALL include the `src/common` directory for header resolution
4. WHEN `DVBS2_LINK_UHD` is OFF, THE `dvbs2_loopback` binary SHALL fail to compile with a clear preprocessor error indicating UHD is required for loopback operation

### Requirement 10: Parallel TX and RX Execution

**User Story:** As an SDR developer, I want the TX and RX chains to run in parallel so that transmission is never starved while the receiver is processing.

#### Acceptance Criteria

1. THE Loopback_Binary SHALL execute the TX_Chain and RX_Chain as a single StreamPU_Sequence where the TX source and radio send tasks run as independent first-stage tasks alongside the RX radio receive task
2. WHEN `--rad-threaded` is specified, THE Radio_USRP module SHALL handle TX/RX streaming in dedicated background threads decoupled from the StreamPU sequence execution via FIFOs
3. THE Loopback_Binary SHALL ensure the TX_Chain produces frames continuously regardless of the RX_Chain decoding latency
