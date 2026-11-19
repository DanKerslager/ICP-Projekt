#include <chrono>
#include <cstdlib>
#include <format>
#include <iostream>

#include <opencv2/opencv.hpp>

#include "Draw.h"
#include "Pipeline.h"
#include "TrackingViewer.h"

namespace {
    using clock = std::chrono::steady_clock;

    double seconds_between(clock::time_point t0, clock::time_point t1)
    {
        return std::chrono::duration<double>(t1 - t0).count();
    }

    // Rate of a growing counter (frames, draws...) per second, refreshed once per second.
    class RateMeter {
    public:
        double update(std::uint64_t count, clock::time_point now)
        {
            double elapsed = seconds_between(last_time, now);
            if (elapsed >= 1.0) {
                rate = (count - last_count) / elapsed;
                last_count = count;
                last_time = now;
            }
            return rate;
        }

    private:
        clock::time_point last_time = clock::now();
        std::uint64_t last_count = 0;
        double rate = 0.0;
    };

    const char* crop_name(CropFrom from)
    {
        switch (from) {
        case CropFrom::ARM:      return "arm";
        case CropFrom::TRACKING: return "track";
        default:                 return "-";
        }
    }

    void draw_overlay(cv::Mat& view, const TrackingResult& result, double age_ms,
        double camera_fps, double pipeline_fps, double view_fps)
    {
        draw_body(view, result.body);
        for (const HandPose& hand : result.hands)
            draw_hand(view, hand);

        int y = 0;
        auto text = [&](const std::string& line) {
            cv::putText(view, line, cv::Point(10, y += 25), cv::FONT_HERSHEY_SIMPLEX, 0.6, CV_RGB(0, 255, 0), 2);
        };
        text(std::format("camera {:.1f}  pipeline {:.1f}  view {:.1f} FPS", camera_fps, pipeline_fps, view_fps));
        text(std::format("body {:.1f} ms  hands {:.1f} ms  age {:.1f} ms", result.body_ms, result.hand_ms, age_ms));
        text(std::format("hand L {:.2f} {}  R {:.2f} {}   [s] save  [Esc] quit",
            result.hands[0].score, crop_name(result.hands[0].crop_from),
            result.hands[1].score, crop_name(result.hands[1].crop_from)));
    }
}

int run_tracking_viewer(const std::string& source)
{
    try {
        Pipeline pipeline(source);

        // The view is not limited by the camera: it redraws the last result when nothing new came,
        // so view FPS = max draw rate while the tracking threads run.
        // camera and pipeline FPS come from the frame / result numbers (frame->id, id)
        RateMeter camera_rate, pipeline_rate, view_rate;
        std::uint64_t drawn = 0;
        auto t_start = clock::now();

        TrackingResult result;
        cv::Mat view; // last drawn image, [s] saves it
        while (!pipeline.finished()) {
            int key = cv::pollKey(); // no waiting (waitKey(1) sleeps up to ~15 ms on Windows)
            if (key == 27) { // Esc
                std::cout << "Quit by Esc.\n";
                break;
            }

            result = pipeline.latest_result();
            if (!result.frame) // camera and models still starting
                continue;

            // measure
            auto now = clock::now();
            ++drawn;
            double camera_fps = camera_rate.update(result.frame->id, now);
            double pipeline_fps = pipeline_rate.update(result.id, now);
            double view_fps = view_rate.update(drawn, now);
            double age_ms = 1000.0 * seconds_between(result.frame->time, now); // capture -> display

            // draw (frame is shared and const - draw on a copy; copyTo reuses view's memory)
            result.frame->image.copyTo(view);
            draw_overlay(view, result, age_ms, camera_fps, pipeline_fps, view_fps);
            cv::imshow("tracking", view);

            if (key == 's') {
                cv::imwrite("debug_result.jpg", view);
                std::cout << "Saved debug_result.jpg\n";
            }
        }

        double total_s = seconds_between(t_start, clock::now());
        std::uint64_t frames = result.frame ? result.frame->id : 0;
        std::cout << std::format("{:.0f} s, average FPS: camera {:.1f}, pipeline {:.1f}, view {:.1f}\n",
            total_s, frames / total_s, result.id / total_s, drawn / total_s);
        cv::destroyAllWindows();
    }
    catch (const std::exception& e) {
        std::cerr << "Viewer failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
