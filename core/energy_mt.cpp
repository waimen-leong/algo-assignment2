#include "core/energy_mt.h"

#include <algorithm>
#include <cstdlib>
#include <thread>
#include <vector>

namespace {

// Sobel energy for rows [r0, r1) of the interior. Reads only gray and writes
// only rows r0..r1-1 of out, so threads with separate bands need no locks.
void SobelRows(const cv::Mat1b& gray, cv::Mat1f& out, int r0, int r1)
{
    const int cols = gray.cols;

    for (int r = r0; r < r1; ++r)
    {
        const uchar* up = gray.ptr<uchar>(r - 1);
        const uchar* mid = gray.ptr<uchar>(r);
        const uchar* down = gray.ptr<uchar>(r + 1);
        float* o = out.ptr<float>(r);

        for (int c = 1; c < cols - 1; ++c)
        {
            // Right column minus left column (weights 1, 2, 1).
            int gx = (up[c + 1] + 2 * mid[c + 1] + down[c + 1]) -
                     (up[c - 1] + 2 * mid[c - 1] + down[c - 1]);

            // Bottom row minus top row (weights 1, 2, 1).
            int gy = (down[c - 1] + 2 * down[c] + down[c + 1]) -
                     (up[c - 1] + 2 * up[c] + up[c + 1]);

            o[c] = static_cast<float>(std::abs(gx) + std::abs(gy));
        }
    }
}

} // namespace

cv::Mat1f ComputeEnergyST(const cv::Mat1b& gray)
{
    cv::Mat1f out(gray.size(), 0.0f);

    if (gray.cols < 3 || gray.rows < 3) return out;

    SobelRows(gray, out, 1, gray.rows - 1);
    return out;
}

cv::Mat1f ComputeEnergyMT(const cv::Mat1b& gray, int nThreads)
{
    cv::Mat1f out(gray.size(), 0.0f);

    if (gray.cols < 3 || gray.rows < 3) return out;

    const int first = 1;
    const int last = gray.rows - 1;
    const int total = last - first;

    nThreads = std::max(1, std::min(nThreads, total)); // no more threads than rows
    int band = (total + nThreads - 1) / nThreads;

    std::vector<std::thread> threads;
    threads.reserve(nThreads);
    for (int i = 0; i < nThreads; i++)
    {
        const int r0 = first + i * band;            // start
        const int r1 = std::min(r0 + band, last);   // end (start + range or last)

        if (r0 >= r1) break;

        threads.emplace_back(SobelRows, std::cref(gray), std::ref(out), r0, r1);
    }

    for (std::thread& t : threads)
    {
        t.join();
    }

    return out;
}
