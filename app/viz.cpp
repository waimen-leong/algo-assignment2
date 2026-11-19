#include "app/viz.h"

#include <opencv2/imgcodecs.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <stdexcept>

namespace {
const cv::Vec3b kRed(0, 0, 255);  // BGR
}

cv::Mat3b drawSeam(const cv::Mat3b& img, const Seam& s, bool vertical) {
    cv::Mat3b out = img.clone();
    if (vertical) {
        // One entry per row; seam[r] is the column to mark.
        const int rows = std::min<int>(out.rows, static_cast<int>(s.size()));
        for (int r = 0; r < rows; ++r) {
            int c = s[r];
            if (c >= 0 && c < out.cols) out(r, c) = kRed;
        }
    } else {
        // One entry per column; seam[c] is the row to mark.
        const int cols = std::min<int>(out.cols, static_cast<int>(s.size()));
        for (int c = 0; c < cols; ++c) {
            int r = s[c];
            if (r >= 0 && r < out.rows) out(r, c) = kRed;
        }
    }
    return out;
}

cv::Mat3b onCanvas(const cv::Mat3b& img, int canvasW, int canvasH) {
    cv::Mat3b canvas(canvasH, canvasW, cv::Vec3b(0, 0, 0));
    const int w = std::min(img.cols, canvasW);
    const int h = std::min(img.rows, canvasH);
    if (w > 0 && h > 0)
        img(cv::Rect(0, 0, w, h)).copyTo(canvas(cv::Rect(0, 0, w, h)));
    return canvas;
}

FrameWriter::FrameWriter(const std::string& dir, int originalW, int originalH)
    : dir_(dir), canvasW_(originalW), canvasH_(originalH) {
    std::error_code ec;
    std::filesystem::create_directories(dir_, ec);
    if (ec)
        throw std::runtime_error("cannot create frames directory '" + dir_ +
                                 "': " + ec.message());
}

void FrameWriter::write(const cv::Mat3b& img, const Seam& s, bool vertical) {
    cv::Mat3b marked = drawSeam(img, s, vertical);
    cv::Mat3b frame = onCanvas(marked, canvasW_, canvasH_);

    char name[32];
    std::snprintf(name, sizeof(name), "frame_%04d.png", ++frame_);
    std::filesystem::path path = std::filesystem::path(dir_) / name;
    if (!cv::imwrite(path.string(), frame))
        throw std::runtime_error("cannot write frame " + path.string());
}
