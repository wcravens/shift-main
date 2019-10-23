#pragma once

#include <chrono>

using namespace std::chrono_literals;

static constexpr auto DURATION_PER_DATA_CHUNK = 300s;

static constexpr auto FIX_SESSION_DURATION = 12 * 60 * 60; // 12 hours
