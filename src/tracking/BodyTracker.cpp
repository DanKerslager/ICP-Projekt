#include <algorithm>

#include <opencv2/imgproc.hpp>

#include "BodyTracker.h"

namespace {
    constexpr float MIN_PERSON_SCORE = 0.5f;
}

BodyPose BodyTracker::detect(const cv::Mat& frame_bgr)
{
    // shrink to fit INPUT_SIZE, then pad bottom and right to square: a single scale maps results back
    float scale = static_cast<float>(INPUT_SIZE) / std::max(frame_bgr.cols, frame_bgr.rows);
    cv::Mat small, square;
    cv::resize(frame_bgr, small, cv::Size(cvRound(frame_bgr.cols * scale), cvRound(frame_bgr.rows * scale)));
    cv::copyMakeBorder(small, square, 0, INPUT_SIZE - small.rows, 0, INPUT_SIZE - small.cols,
        cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));

    // NCHW, RGB, 0..1
    net.setInput(cv::dnn::blobFromImage(square, 1.0 / 255.0, cv::Size(), cv::Scalar(), true, false));
    cv::Mat out = net.forward();

    // out = [1, 56, N], N = candidates
    // rows: cx, cy, w, h, person score, then 17 x (x, y, visibility), all in network input pixels
    cv::Mat rows(out.size[1], out.size[2], CV_32F, out.ptr<float>());

    // one player -> take the best candidate, no need for NMS
    BodyPose pose;
    double best_score = 0.0;
    cv::Point best_loc;
    cv::minMaxLoc(rows.row(4), nullptr, &best_score, nullptr, &best_loc);
    if (best_score < MIN_PERSON_SCORE)
        return pose;

    int best = best_loc.x;
    for (int k = 0; k < BODY_POINTS; ++k) {
        pose.points[k].pos = cv::Point2f(rows.at<float>(5 + 3 * k, best), rows.at<float>(6 + 3 * k, best)) / scale;
        pose.points[k].score = rows.at<float>(7 + 3 * k, best);
    }
    pose.valid = true;
    return pose;
}
