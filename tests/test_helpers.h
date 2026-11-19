// test_helpers.h — small tools shared by all the tests.
//
//   bruteForceMinCost  the "answer key": tries every seam, returns the cheapest cost
//   seamSum            adds up energy along a seam (our own, so we don't trust seamCost)
//   makeGrid           write a small energy grid by hand, row by row
//   randomIntGrid      random whole-number grid (whole numbers add up exactly, so == is safe)
//   randomFloatGrid    random decimal grid (ties are practically impossible)
//   randomImage        random colour image
//   StraightFinder     a dumb finder that always picks the middle column (for carve tests)
#pragma once

#include "seam_api.h"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <random>

namespace th {

// Fixed seed: every run of the tests uses the same "random" grids,
// so a failure can be reproduced exactly.
constexpr std::uint32_t kSeed = 20261119u;

inline float seamSum(const cv::Mat1f& e, const Seam& s) {
    float total = 0.0f;
    for (int r = 0; r < e.rows; ++r) total += e(r, s[r]);
    return total;
}

// Cheapest cost of any seam that passes through (r, c) and continues to the bottom.
// Plain recursion, no table — deliberately nothing like DP, so it is an
// independent check. It explores 3 choices per row, so keep grids tiny
// (8 rows x 8 cols is about 17,000 paths, which is still instant).
inline float bruteFrom(const cv::Mat1f& e, int r, int c) {
    float here = e(r, c);
    if (r == e.rows - 1) return here;
    float best = std::numeric_limits<float>::infinity();
    for (int dc = -1; dc <= 1; ++dc) {
        int nc = c + dc;
        if (nc < 0 || nc >= e.cols) continue;
        best = std::min(best, bruteFrom(e, r + 1, nc));
    }
    return here + best;
}

inline float bruteForceMinCost(const cv::Mat1f& e) {
    float best = std::numeric_limits<float>::infinity();
    for (int c = 0; c < e.cols; ++c) best = std::min(best, bruteFrom(e, 0, c));
    return best;
}

inline cv::Mat1f makeGrid(std::initializer_list<std::initializer_list<float>> rows) {
    const int h = static_cast<int>(rows.size());
    const int w = static_cast<int>(rows.begin()->size());
    cv::Mat1f e(h, w);
    int r = 0;
    for (const auto& row : rows) {
        int c = 0;
        for (float v : row) e(r, c++) = v;
        ++r;
    }
    return e;
}

inline cv::Mat1f randomIntGrid(std::mt19937& rng, int rows, int cols, int lo = 0, int hi = 9) {
    std::uniform_int_distribution<int> d(lo, hi);
    cv::Mat1f e(rows, cols);
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) e(r, c) = static_cast<float>(d(rng));
    return e;
}

inline cv::Mat1f randomFloatGrid(std::mt19937& rng, int rows, int cols) {
    std::uniform_real_distribution<float> d(0.0f, 255.0f);
    cv::Mat1f e(rows, cols);
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) e(r, c) = d(rng);
    return e;
}

inline cv::Mat3b randomImage(std::mt19937& rng, int rows, int cols) {
    std::uniform_int_distribution<int> d(0, 255);
    cv::Mat3b img(rows, cols);
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            img(r, c) = cv::Vec3b(static_cast<uchar>(d(rng)), static_cast<uchar>(d(rng)),
                                  static_cast<uchar>(d(rng)));
    return img;
}

// Always removes the middle column. Lets us test carve() without any real algorithm.
struct StraightFinder : ISeamFinder {
    Seam find(const cv::Mat1f& energy) override { return Seam(energy.rows, energy.cols / 2); }
    const char* name() const override { return "straight"; }
};

}  // namespace th
