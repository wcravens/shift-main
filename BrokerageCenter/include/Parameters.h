#pragma once

#include <chrono>

using namespace std::chrono_literals;

static const auto BROADCAST_ORDERBOOK_PERIOD = 1min;

static constexpr auto DEFAULT_BUYING_POWER = 1.e6;

static constexpr unsigned int NUM_SECONDS_PER_CANDLESTICK = 5;
