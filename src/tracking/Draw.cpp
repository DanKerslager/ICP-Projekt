#include <vector>

#include <opencv2/imgproc.hpp>

#include "Draw.h"

void draw_body(cv::Mat& img, const BodyPose& body)
{
    if (!body.valid)
        return;
    for (auto [a, b] : BODY_BONES)
        if (body.points[a].visible() && body.points[b].visible())
            cv::line(img, body.points[a].pos, body.points[b].pos, CV_RGB(0, 255, 0), 2);
    for (const Keypoint& p : body.points)
        if (p.visible())
            cv::circle(img, p.pos, 4, CV_RGB(255, 0, 0), cv::FILLED);
}

void draw_hand(cv::Mat& img, const HandPose& hand)
{
    // crop box: cyan = tracked from previous hand, orange = from forearm, red = hand rejected
    if (hand.crop_from != CropFrom::NONE) {
        std::vector<cv::Point> box(hand.crop_corners.begin(), hand.crop_corners.end());
        cv::Scalar color = !hand.valid ? CV_RGB(255, 0, 0)
            : hand.crop_from == CropFrom::TRACKING ? CV_RGB(0, 200, 255) : CV_RGB(255, 160, 0);
        cv::polylines(img, box, true, color, 1);
    }

    if (!hand.valid)
        return;
    for (auto [a, b] : HAND_BONES)
        cv::line(img, hand.points[a], hand.points[b], CV_RGB(0, 200, 255), 1);
    for (const cv::Point2f& p : hand.points)
        cv::circle(img, p, 2, CV_RGB(255, 255, 0), cv::FILLED);
}
