// FROZEN CONTRACT - owned by Person A.
// Changes only if the whole team agrees.
#pragma once
#include <opencv2/core.hpp>
#include <functional>
#include <vector>

// A vertical seam: seam[row] = column of the pixel to remove in that row.
using Seam = std::vector<int>;

// Every seam-finding algorithm (DP, greedy, graph cut) implements this.
// It is only ever asked for a VERTICAL seam; carve() handles horizontal
// seams by transposing the image.
struct ISeamFinder {
    virtual ~ISeamFinder() = default;
    virtual Seam find(const cv::Mat1f& energy) = 0;
    virtual const char* name() const = 0;
};

// Called after each seam is found, before it is removed (UI / frame dumps).
// img is always in normal (display) orientation.
//   vertical == true : seam[row] = column
//   vertical == false: seam[col] = row
using StepCallback =
    std::function<void(const cv::Mat3b& img, const Seam& seam, bool vertical)>;
