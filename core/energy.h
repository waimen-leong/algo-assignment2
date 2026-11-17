#pragma once
#include <opencv2/core.hpp>

// Energy map: one value per pixel, high where removing it would be noticed
// (edges), low in flat regions. e = |dI/dx| + |dI/dy| using 3x3 Sobel on
// the grayscale image. Output has the same size as img.
cv::Mat1f computeEnergy(const cv::Mat3b& img);
