// This file is meant to replace the energy.h 
// This energy calculation approach doesnt rely on the cv::sobel
// And user can switch to Single/Multi threaded

#pragma once

#include <opencv2/core.hpp>

cv::Mat1f ComputeEnergyST(const cv::Mat1b& gray);


void SobelRows(const cv::Mat1b& gray, cv::Mat1f& out, int r0, int r1);