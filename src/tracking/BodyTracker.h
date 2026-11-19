#pragma once

#include <opencv2/dnn.hpp>

#include "Types.h"

// Single-person body pose from YOLO-pose ONNX model.
// No threads inside. Not thread-safe (cv::dnn::Net), one instance per thread.
class BodyTracker {
public:
    BodyPose detect(const cv::Mat& frame_bgr);

private:
    static constexpr int INPUT_SIZE = 320; // must match export: yolo export ... imgsz=320
    cv::dnn::Net net = cv::dnn::readNet("resources/models/yolo11n-pose.onnx"); // throws if missing
};
