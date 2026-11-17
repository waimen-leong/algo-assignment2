#pragma once
#include "common/types.h"

// Returns img with one pixel removed from every row (width - 1).
// Requires s.size() == img.rows and every s[r] in [0, img.cols).
cv::Mat3b removeVerticalSeam(const cv::Mat3b& img, const Seam& s);

// True if s is a valid vertical seam for a rows x cols image:
// one entry per row, every column in range, and neighbouring rows
// differ by at most 1 column (connected vertically or diagonally).
bool isValidSeam(const Seam& s, int rows, int cols);

// Total energy along the seam: sum over r of e(r, s[r]).
float seamCost(const cv::Mat1f& e, const Seam& s);

// Main loop. Shrinks img to exactly targetW x targetH (shrink only) by
// repeatedly computing energy, asking finder for a seam, and removing it.
// Vertical seams first (width), then horizontal (height) via transpose.
// Throws std::invalid_argument on a bad target, std::runtime_error if the
// finder returns an invalid seam.
cv::Mat3b carve(const cv::Mat3b& img, int targetW, int targetH,
                ISeamFinder& finder, StepCallback cb = {});
