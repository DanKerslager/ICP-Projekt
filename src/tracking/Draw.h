#pragma once

#include <opencv2/core.hpp>

#include "Types.h"

void draw_body(cv::Mat& img, const BodyPose& body);
void draw_hand(cv::Mat& img, const HandPose& hand); // also the crop box the hand model looked at
