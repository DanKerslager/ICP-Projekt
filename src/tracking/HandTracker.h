#pragma once

#include <array>
#include <string>
#include <vector>

#include <opencv2/dnn.hpp>

#include "Types.h"

// 21 hand landmarks from MediaPipe hand model (opencv_zoo ONNX). No palm detector:
//  - hand found in previous frame -> crop from its landmarks (tracking, follows wrist bends)
//  - otherwise                    -> crop from body pose (elbow -> wrist)
// Keeps tracking state between calls: one instance per thread, frames in capture order.
// No threads inside. Not thread-safe (cv::dnn::Net).
class HandTracker {
public:
    // [0] = person's left hand, [1] = person's right hand
    std::array<HandPose, 2> detect(const cv::Mat& frame_bgr, const BodyPose& body);

private:
    // crop as transform frame pixels -> model input pixels, empty = no crop possible
    cv::Mat crop_from_arm(const BodyPose& body, int hand) const;
    cv::Mat crop_from_landmarks(const BodyPose& body, int hand) const;
    HandPose run_model(const cv::Mat& frame_bgr, const cv::Mat& crop);

    cv::dnn::Net net = cv::dnn::readNet("resources/models/handpose_estimation_mediapipe_2023feb.onnx"); // throws if missing
    std::vector<std::string> output_names = net.getUnconnectedOutLayersNames(); // asked once, used every frame

    // tracking state: hands from the previous frame
    std::array<HandPose, 2> last_hands;
};
