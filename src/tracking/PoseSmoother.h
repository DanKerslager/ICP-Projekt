#pragma once

#include <array>
#include <chrono>

#include "OneEuroFilter.h"
#include "Types.h"

// Smooths all body and hand points of consecutive results (removes jitter).
// Keeps filter state between frames, so one instance per thread, results in capture order.
class PoseSmoother {
public:
    void apply(TrackingResult& result);

private:
    std::array<OneEuroFilter, BODY_POINTS> body;
    std::array<std::array<OneEuroFilter, HAND_POINTS>, 2> hands;
    std::chrono::steady_clock::time_point last_time; // default = no previous frame
};
