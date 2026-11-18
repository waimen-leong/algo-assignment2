// Hand-written Sobel energy, single- and multi-threaded.
//
// Used only to measure whether multithreading speeds up the energy step.
// It is not used by the app: the pipeline uses computeEnergy() in energy.h,
// which calls cv::Sobel. We cannot benchmark threading against cv::Sobel
// because OpenCV already optimises it internally, so both functions here run
// the same plain loops and differ only in how many threads run them.
//
// Output: e = |Gx| + |Gy| (3x3 Sobel), same size as gray, with the 1-pixel
// border set to 0. ComputeEnergyMT gives exactly the same result as
// ComputeEnergyST for any thread count.

#pragma once

#include <opencv2/core.hpp>

cv::Mat1f ComputeEnergyST(const cv::Mat1b& gray);

// Splits the interior rows into nThreads bands, one std::thread per band.
// nThreads is clamped to [1, number of interior rows].
cv::Mat1f ComputeEnergyMT(const cv::Mat1b& gray, int nThreads);
