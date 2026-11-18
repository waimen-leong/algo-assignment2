#include "core/energy_mt.h"

#include <cstdlib>

cv::Mat1f ComputeEnergyST(const cv::Mat1b& gray)
{
    cv::Mat1f out(gray.rows, gray.cols);

    if (gray.cols < 3|| gray.rows < 3) return out;

    SobelRows(gray, out, 1, gray.rows - 1);
    return out;
}

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
            float gx = (up[c + 1] + 2 * mid[c] + down[c + 1]) -
                       (up[c - 1] + 2 * mid[c] + down[c - 1]);

            float gy = (down[c - 1] + 2 * down[c] + down[c + 1]) - 
                       (up[c - 1] + 2 * up[c] + up[c + 1]);

            o[c] = abs(gx) + abs(gy);
        }
    }
}