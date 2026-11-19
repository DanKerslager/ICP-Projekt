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

//
// Hand: 21 MediaPipe landmarks (0 = wrist, 4 = thumb tip, 8 = index tip, ...)
//
constexpr int HAND_POINTS = 21;

constexpr std::array<std::pair<int, int>, 21> HAND_BONES{ {
    {0, 1}, {1, 2}, {2, 3}, {3, 4},                 // thumb
    {0, 5}, {5, 6}, {6, 7}, {7, 8},                 // index
    {5, 9}, {9, 10}, {10, 11}, {11, 12},            // middle
    {9, 13}, {13, 14}, {14, 15}, {15, 16},          // ring
    {13, 17}, {0, 17}, {17, 18}, {18, 19}, {19, 20} // little + palm
} };

// where the crop given to the hand model came from
enum class CropFrom { NONE, ARM, TRACKING }; // none / elbow -> wrist / previous hand landmarks

struct HandPose {
    std::array<cv::Point2f, HAND_POINTS> points;
    float score = 0.0f; // hand presence score
    bool valid = false;

    // crop the model looked at (also when the hand was rejected), for debug drawing
    CropFrom crop_from = CropFrom::NONE;
    std::array<cv::Point2f, 4> crop_corners;
};

// Output of the pipeline: poses together with the frame they were found in.
struct TrackingResult {
    std::uint64_t id = 0;          // 1, 2, 3... in order of finishing (frames may be skipped, results not)
    FramePtr frame;
    BodyPose body;
    std::array<HandPose, 2> hands; // [0] = person's left, [1] = person's right
    double body_ms = 0.0;          // body model time
    double hand_ms = 0.0;          // hand model time (both hands)
};
