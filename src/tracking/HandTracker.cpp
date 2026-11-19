#include <algorithm>
#include <cmath>

#include <opencv2/geometry.hpp> // getRotationMatrix2D, invertAffineTransform, minAreaRect (moved here in OpenCV 5)
#include <opencv2/imgproc.hpp>

#include "HandTracker.h"

namespace {
    constexpr int INPUT_SIZE = 224;

    // body points (elbow, wrist) for hand [0] = person's left, [1] = person's right
    constexpr int ARMS[2][2] = {
        { body::LEFT_ELBOW,  body::LEFT_WRIST },
        { body::RIGHT_ELBOW, body::RIGHT_WRIST },
    };

    constexpr float MIN_HAND_SCORE = 0.5f;

    // crop from body pose, relative to forearm length (elbow -> wrist)
    constexpr float ARM_CENTER_SHIFT = 0.15f;   // crop center = wrist + shift * forearm
    constexpr float ARM_BOX_SCALE = 1.3f;       // crop side = scale * forearm length
    constexpr float MIN_SHOULDER_SCALE = 0.8f;  // crop side >= scale * shoulder width (forearm pointing at camera looks short)

    // crop from previous landmarks (values from MediaPipe hand tracking)
    constexpr float LANDMARK_BOX_SCALE = 2.0f;  // crop side = scale * hand box
    constexpr float LANDMARK_SHIFT = 0.1f;      // move center towards fingers by shift * hand length
    constexpr float MAX_WRIST_DISTANCE = 0.5f;  // tracked wrist must stay within this * crop side from body wrist

    // Square crop of the frame given to the hand model, as transform frame pixels -> crop pixels:
    // rotate around center so `up` (towards fingers) points up, scale side to INPUT_SIZE,
    // then move center to the middle of the model input.
    cv::Mat crop_transform(cv::Point2f center, float side, cv::Point2f up)
    {
        double angle = 90.0 + std::atan2(up.y, up.x) * 180.0 / CV_PI; // degrees, counter-clockwise
        cv::Mat m = cv::getRotationMatrix2D(center, angle, INPUT_SIZE / side);
        m.at<double>(0, 2) += INPUT_SIZE / 2.0 - center.x;
        m.at<double>(1, 2) += INPUT_SIZE / 2.0 - center.y;
        return m;
    }

    float length(cv::Point2f v) { return static_cast<float>(cv::norm(v)); }
}

std::array<HandPose, 2> HandTracker::detect(const cv::Mat& frame_bgr, const BodyPose& body)
{
    std::array<HandPose, 2> hands;
    for (int i = 0; i < 2; ++i) {
        cv::Mat crop; // empty = no crop
        CropFrom from = CropFrom::TRACKING;
        if (last_hands[i].valid)
            crop = crop_from_landmarks(body, i);
        if (crop.empty()) {
            crop = crop_from_arm(body, i);
            from = CropFrom::ARM;
        }
        if (!crop.empty()) {
            hands[i] = run_model(frame_bgr, crop);
            hands[i].crop_from = from;
        }
    }
    last_hands = hands;
    return hands;
}

cv::Mat HandTracker::crop_from_arm(const BodyPose& body, int hand) const
{
    if (!body.valid)
        return {};
    const Keypoint& elbow = body.points[ARMS[hand][0]];
    const Keypoint& wrist = body.points[ARMS[hand][1]];
    if (!elbow.visible() || !wrist.visible())
        return {};

    cv::Point2f forearm = wrist.pos - elbow.pos;
    float forearm_length = length(forearm);
    if (forearm_length < 1.0f) // no direction
        return {};

    float side = ARM_BOX_SCALE * forearm_length;
    const Keypoint& ls = body.points[body::LEFT_SHOULDER];
    const Keypoint& rs = body.points[body::RIGHT_SHOULDER];
    if (ls.visible() && rs.visible())
        side = std::max(side, MIN_SHOULDER_SCALE * length(ls.pos - rs.pos));

    return crop_transform(wrist.pos + ARM_CENTER_SHIFT * forearm, side, forearm);
}

cv::Mat HandTracker::crop_from_landmarks(const BodyPose& body, int hand) const
{
    const auto& points = last_hands[hand].points;

    // hand direction: wrist (0) -> middle finger base (9)
    cv::Point2f up = points[9] - points[0];
    float up_length = length(up);
    if (up_length < 1.0f)
        return {};

    // smallest rectangle around the hand, crop = bigger square, a bit towards the fingers
    cv::RotatedRect box = cv::minAreaRect(points);
    float hand_size = std::max(box.size.width, box.size.height);
    cv::Point2f center = box.center + up / up_length * (LANDMARK_SHIFT * hand_size);
    float side = LANDMARK_BOX_SCALE * hand_size;

    // safety: tracked hand must stay at the body's wrist (e.g. hands crossing, tracking drifted away)
    if (body.valid) {
        const Keypoint& wrist = body.points[ARMS[hand][1]];
        if (wrist.visible() && length(wrist.pos - points[0]) > MAX_WRIST_DISTANCE * side)
            return {};
    }

    return crop_transform(center, side, up);
}

HandPose HandTracker::run_model(const cv::Mat& frame_bgr, const cv::Mat& transform)
{
    HandPose hand;
    cv::Mat inverse; // crop pixels -> frame pixels
    cv::invertAffineTransform(transform, inverse);

    // crop outline, for drawing
    const float s = static_cast<float>(INPUT_SIZE);
    const std::array<cv::Point2f, 4> corners = { { {0, 0}, {s, 0}, {s, s}, {0, s} } };
    cv::transform(corners, hand.crop_corners, inverse);

    // model input: NHWC, RGB, 0..1
    cv::Mat input;
    cv::warpAffine(frame_bgr, input, transform, cv::Size(INPUT_SIZE, INPUT_SIZE));
    net.setInput(cv::dnn::blobFromImageWithParams(input, cv::dnn::Image2BlobParams(
        cv::Scalar::all(1.0 / 255.0), cv::Size(), cv::Scalar(), true, CV_32F, cv::DATA_LAYOUT_NHWC)));

    // outputs: landmarks [1, 63] (21 x (x, y, z) in crop pixels), score [1, 1], handedness [1, 1], world landmarks [1, 63]
    std::vector<cv::Mat> outs;
    net.forward(outs, output_names);

    hand.score = outs[1].ptr<float>()[0];
    if (hand.score < MIN_HAND_SCORE)
        return hand;

    const float* landmarks = outs[0].ptr<float>();
    std::array<cv::Point2f, HAND_POINTS> crop_points;
    for (int k = 0; k < HAND_POINTS; ++k)
        crop_points[k] = cv::Point2f(landmarks[3 * k], landmarks[3 * k + 1]);
    cv::transform(crop_points, hand.points, inverse);
    hand.valid = true;
    return hand;
}
