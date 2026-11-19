#include <chrono>
#include <iostream>
#include <stdexcept>

#include <opencv2/core/utility.hpp>

#include "Pipeline.h"

Pipeline::Pipeline(const std::string& source)
{
    // open here (main thread), so a failure is an exception for the caller, not a dead thread
    bool is_video_file = !source.empty();
    if (is_video_file)
        capture.open(source);
    else
        capture.open(0);
    if (!capture.isOpened())
        throw std::runtime_error("Can not open video source: " + (is_video_file ? source : std::string("camera 0")));

    camera_thread = std::jthread([this, is_video_file](std::stop_token st) { camera_loop(st, is_video_file); });
    body_thread = std::jthread([this](std::stop_token st) { body_loop(st); });
    hand_thread = std::jthread([this](std::stop_token st) { hand_loop(st); });
}

void Pipeline::camera_loop(std::stop_token st, bool is_video_file)
{
    using clock = std::chrono::steady_clock;
    double fps = capture.get(cv::CAP_PROP_FPS);
    const auto frame_time = std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(1.0 / (fps > 0.0 ? fps : 30.0)));
    auto next_frame = clock::now();
    std::uint64_t id = 0;

    while (!st.stop_requested()) {
        // new Mat every frame: the previous one may still be read by other threads
        cv::Mat image;
        capture >> image;
        if (image.empty()) {
            std::cout << "Video source ended.\n";
            break;
        }

        // camera blocks until next frame by itself; video file would be read as fast as possible
        if (is_video_file) {
            next_frame += frame_time;
            std::this_thread::sleep_until(next_frame);
        }

        auto frame = std::make_shared<Frame>();
        frame->id = ++id;
        frame->time = clock::now();
        frame->image = image;
        frames.put(std::move(frame));
    }
    stopped = true;
}

void Pipeline::body_loop(std::stop_token st)
{
    FramePtr frame;
    cv::TickMeter timer;
    while (frames.take(frame, st)) {
        TrackingResult result;
        result.frame = frame;
        timer.reset();
        timer.start();
        result.body = body_tracker.detect(frame->image);
        timer.stop();
        result.body_ms = timer.getTimeMilli();
        body_results.put(std::move(result));
    }
}

void Pipeline::hand_loop(std::stop_token st)
{
    TrackingResult result;
    cv::TickMeter timer;
    std::uint64_t id = 0;
    while (body_results.take(result, st)) {
        result.id = ++id;
        timer.reset();
        timer.start();
        result.hands = hand_tracker.detect(result.frame->image, result.body);
        timer.stop();
        result.hand_ms = timer.getTimeMilli();
        results.put(std::move(result));
    }
}
