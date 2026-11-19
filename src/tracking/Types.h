#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>

#include <opencv2/core.hpp>

// Camera frame shared between threads. Never modified after it is published,
// so many threads can read it at once without locking (enforced by const in FramePtr).
struct Frame {
    std::uint64_t id = 0;                       // 1, 2, 3... in capture order
    std::chrono::steady_clock::time_point time; // capture time, for latency measurement
    cv::Mat image;                              // BGR
};
using FramePtr = std::shared_ptr<const Frame>;

// All positions are in frame pixels.

struct Keypoint {
    cv::Point2f pos{ -1.0f, -1.0f };
    float score = 0.0f; // 0..1, how sure the model is that the point is visible

    bool visible() const { return score > 0.3f; } // one rule for drawing, hand crops, gestures...
};

//
// Body: 17 COCO keypoints (YOLO-pose)
//
constexpr int BODY_POINTS = 17;

namespace body {
    enum Point : int {
        NOSE, LEFT_EYE, RIGHT_EYE, LEFT_EAR, RIGHT_EAR,
        LEFT_SHOULDER, RIGHT_SHOULDER, LEFT_ELBOW, RIGHT_ELBOW, LEFT_WRIST, RIGHT_WRIST,
        LEFT_HIP, RIGHT_HIP, LEFT_KNEE, RIGHT_KNEE, LEFT_ANKLE, RIGHT_ANKLE
    };
}

constexpr std::array<std::pair<int, int>, 16> BODY_BONES{ {
    {0, 1}, {0, 2}, {1, 3}, {2, 4},                 // head
    {5, 6}, {5, 7}, {7, 9}, {6, 8}, {8, 10},        // shoulders, arms
    {5, 11}, {6, 12}, {11, 12},                     // torso
    {11, 13}, {13, 15}, {12, 14}, {14, 16},         // legs
} };

struct BodyPose {
    std::array<Keypoint, BODY_POINTS> points;
    bool valid = false; // person found
};

// Output of the pipeline: poses together with the frame they were found in.
struct TrackingResult {
    std::uint64_t id = 0;          // 1, 2, 3... in order of finishing (frames may be skipped, results not)
    FramePtr frame;
    BodyPose body;
    double body_ms = 0.0;          // body model time
};
