#pragma once
#include <opencv2/core.hpp>

#include "common/types.h"

// M[i][j] = cheapest seam cost from row 0 ending at pixel (i, j).
cv::Mat1f buildCostTable(const cv::Mat1f& e);

// Walk up from the minimum of the last row.
// Tie rule: when values are equal, the LEFTMOST column wins.
Seam backtrack(const cv::Mat1f& M);

// Full table: O(m*n) time, O(m*n) floats of space.
class DPSeamFinder : public ISeamFinder
{
    public:
        Seam find(const cv::Mat1f& energy) override
        {
            return backtrack(buildCostTable(energy));
        }
        const char* name() const override { return "dp"; }
};

// Two rows of costs + 1 byte back-pointer per pixel.
// Same seam and same time as DPSeamFinder, but 1 byte/pixel instead of 4.
class DPSeamFinderLowMem : public ISeamFinder
{
    public:
        Seam find(const cv::Mat1f& energy) override;
        const char* name() const override { return "dp-lowmem"; }
};
