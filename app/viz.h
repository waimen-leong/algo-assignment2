#pragma once
#include "common/types.h"

#include <opencv2/core.hpp>

#include <string>

// Return a copy of img with the seam painted red.
//   vertical == true : seam[row] = column, paint pixel (row, seam[row])
//   vertical == false: seam[col] = row,    paint pixel (seam[col], col)
// Red is Vec3b(0, 0, 255) because OpenCV stores pixels as BGR.
cv::Mat3b drawSeam(const cv::Mat3b& img, const Seam& s, bool vertical);

// Paste img into the top-left of a black canvas of size (canvasW x canvasH).
// Used so every exported frame has the same size even as the image shrinks.
cv::Mat3b onCanvas(const cv::Mat3b& img, int canvasW, int canvasH);

// Dumps one PNG per seam into a directory, each the size of the original so
// the sequence can be turned straight into a video.
class FrameWriter {
public:
    // dir is created if needed. originalW/H set the canvas size.
    FrameWriter(const std::string& dir, int originalW, int originalH);

    // Draw the seam on img, paste onto the canvas, save as frame_NNNN.png.
    void write(const cv::Mat3b& img, const Seam& s, bool vertical);

    int count() const { return frame_; }

private:
    std::string dir_;
    int canvasW_;
    int canvasH_;
    int frame_ = 0;
};
