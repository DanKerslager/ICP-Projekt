#pragma once

#include <numbers>

#include <opencv2/core.hpp>

// One Euro filter (Casiez et al. 2012) for a 2D point: low-pass filter whose cutoff frequency grows with speed.
// Slow movement -> low cutoff -> jitter removed. Fast movement -> high cutoff -> little lag.
// A default-constructed filter starts fresh: reset with `filter = {}`.
class OneEuroFilter {
public:
    // tuned on camera, positions in pixels of a 640x480 frame
    static constexpr float MIN_CUTOFF = 0.7f; // [Hz] smoothing when still (lower = smoother, more lag)
    static constexpr float BETA = 0.008f;     // how fast cutoff grows with speed [per px/s] (higher = less lag in fast moves)
    static constexpr float D_CUTOFF = 1.0f;   // [Hz] cutoff for the speed estimate

    cv::Point2f filter(cv::Point2f p, float dt)
    {
        if (!initialized) {
            initialized = true;
            prev = p;
            return p;
        }
        speed += alpha(D_CUTOFF, dt) * ((p - prev) / dt - speed); // smoothed speed [px/s]
        float cutoff = MIN_CUTOFF + BETA * static_cast<float>(cv::norm(speed));
        prev += alpha(cutoff, dt) * (p - prev);
        return prev;
    }

private:
    // how much of the new value to take (0..1) for a given cutoff frequency
    static float alpha(float cutoff, float dt)
    {
        float tau = 1.0f / (2.0f * std::numbers::pi_v<float> * cutoff);
        return 1.0f / (1.0f + tau / dt);
    }

    bool initialized = false;
    cv::Point2f prev;
    cv::Point2f speed;
};
