#pragma once

#include <atomic>
#include <string>
#include <thread>

#include <opencv2/videoio.hpp>

#include "BodyTracker.h"
#include "HandTracker.h"
#include "Mailbox.h"
#include "PoseSmoother.h"
#include "Types.h"

// Owns the tracking threads and the mailboxes between them.
//   camera thread --FramePtr--> body thread --TrackingResult--> hand thread --TrackingResult--> consumers
// Each stage works on the newest data and skips what it was too slow for.
// Stages overlap: body works on frame N while hands work on frame N-1.
// Threads start in the constructor and stop + join in the destructor.
class Pipeline {
public:
    // source: empty = camera 0, otherwise video file
    // real_time: video file plays at its own FPS; false = as fast as possible (benchmark)
    explicit Pipeline(const std::string& source, bool real_time = true);

    TrackingResult latest_result() { return results.latest(); }
    bool finished() const { return stopped; } // video ended or camera closed

private:
    void camera_loop(std::stop_token st, bool throttle);
    void body_loop(std::stop_token st);
    void hand_loop(std::stop_token st); // hands + smoothing

    // created on main thread (a missing model file throws to the caller), then used only by its own thread
    cv::VideoCapture capture;
    BodyTracker body_tracker;
    HandTracker hand_tracker;
    PoseSmoother smoother;

    std::atomic<bool> stopped{ false };

    Mailbox<FramePtr> frames;              // camera -> body
    Mailbox<TrackingResult> body_results;  // body -> hand
    Mailbox<TrackingResult> results;       // hand -> consumers

    // threads last: destroyed first, so they are joined before the members they use are gone
    std::jthread camera_thread;
    std::jthread body_thread;
    std::jthread hand_thread;
};
