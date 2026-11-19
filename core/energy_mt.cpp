#include "core/energy_mt.h"

#include <algorithm>
#include <cstdlib>
#include <thread>
#include <vector>

namespace {

// Sobel energy for rows [r0, r1) of the interior. Reads only gray and writes
// only rows r0..r1-1 of out, so threads with separate bands need no locks.
// Also writes the 0 border pixels of those rows itself: out is NOT zero-filled
// up front, because on big images that serial fill costs more than the Sobel
// work (42 of 45 ms at 4000x3000) and hides any speedup from threading.
void SobelRows(const cv::Mat1b& gray, cv::Mat1f& out, int r0, int r1)
{
    const int cols = gray.cols;

    for (int r = r0; r < r1; ++r)
    {
        const uchar* up = gray.ptr<uchar>(r - 1);
        const uchar* mid = gray.ptr<uchar>(r);
        const uchar* down = gray.ptr<uchar>(r + 1);
        float* o = out.ptr<float>(r);
        o[0] = 0.0f;
        o[cols - 1] = 0.0f;

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

// Too small for a 3x3 kernel: everything is border, so all zeros.
// Otherwise allocate without filling and zero only the top and bottom rows;
// SobelRows writes every other pixel.
static bool AllocOutput(const cv::Mat1b& gray, cv::Mat1f& out)
{
    if (gray.cols < 3 || gray.rows < 3)
    {
        out = cv::Mat1f(gray.size(), 0.0f);
        return false;
    }
    out.create(gray.size());
    out.row(0).setTo(0.0f);
    out.row(gray.rows - 1).setTo(0.0f);
    return true;
}

cv::Mat1f ComputeEnergyST(const cv::Mat1b& gray)
{
    cv::Mat1f out;
    if (!AllocOutput(gray, out)) return out;

    SobelRows(gray, out, 1, gray.rows - 1);
    return out;
}

cv::Mat1f ComputeEnergyMT(const cv::Mat1b& gray, int nThreads)
{
    cv::Mat1f out;
    if (!AllocOutput(gray, out)) return out;

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
