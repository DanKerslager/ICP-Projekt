#pragma once

#include <string>

// Live view of the tracking pipeline on the main thread (cv::imshow).
// source: empty = camera 0, otherwise video file. Esc quits.
// real_time = false: video file is read as fast as possible, shows max pipeline FPS.
int run_tracking_viewer(const std::string& source, bool real_time = true);
