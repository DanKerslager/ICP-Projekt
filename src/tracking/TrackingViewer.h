#pragma once

#include <string>

// Live view of the tracking pipeline on the main thread (cv::imshow).
// source: empty = camera 0, otherwise video file. Esc quits.
int run_tracking_viewer(const std::string& source);
