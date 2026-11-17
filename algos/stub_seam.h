#pragma once
#include "common/types.h"

// Deliberately WRONG finder: always returns the middle column.
// Lets the whole pipeline (carve, app, tests, CI) run before any real
// algorithm exists.
class StubSeamFinder : public ISeamFinder {
public:
    Seam find(const cv::Mat1f& energy) override {
        return Seam(energy.rows, energy.cols / 2);
    }
    const char* name() const override { return "stub"; }
};
