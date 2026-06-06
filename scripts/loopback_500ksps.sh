#!/bin/bash
# 500 ksps loopback test (narrowband DATV style)
# 1 MHz sample rate / 2 oversampling = 500 ksps symbol rate
# Requires: B200-mini with attenuator between TX/RX and RX2

SERIAL="${B200_SERIAL:-327BBD5}"
CLK_RATE=30.72e6
SAMPLE_RATE=1e6       # 1 MHz sample rate / 2 oversampling = 500 ksps symbol rate
FREQ=2360e6
TX_GAIN=${TX_GAIN:-50}
RX_GAIN=${RX_GAIN:-40}

cd "$(dirname "$0")/../build" || exit 1

./bin/dvbs2_loopback \
    --rad-clk-rate $CLK_RATE --rad-serial $SERIAL \
    --rad-tx-rate $SAMPLE_RATE --rad-tx-freq $FREQ --rad-tx-gain $TX_GAIN \
    --rad-rx-rate $SAMPLE_RATE --rad-rx-freq $FREQ --rad-rx-gain $RX_GAIN \
    --rad-threaded -F 8 \
    --src-type USER --src-path ../conf/src/K_14232.src \
    --mod-cod QPSK-S_8/9 --dec-implem NMS --dec-ite 10 --dec-simd INTER
