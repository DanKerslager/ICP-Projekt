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
