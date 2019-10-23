#pragma once

#include <chrono>

using namespace std::chrono_literals;

static constexpr auto BROADCAST_ORDERBOOK_PERIOD = 1min;

static constexpr auto DEFAULT_BUYING_POWER = 1.e6; // 1,000,000.00

static constexpr auto FIX_SESSION_DURATION = 12 * 60 * 60; // 12 hours

static constexpr unsigned int NUM_SECONDS_PER_CANDLESTICK = 5;
