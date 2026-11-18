#pragma once

#include "common/types.h"

class GreedySeamFinder : public ISeamFinder
{
    public:

    Seam find(const cv::Mat1f& energy) override;

    const char* name() const override;
};