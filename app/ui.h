#pragma once
#include <opencv2/core.hpp>

// Launch the interactive window: Width/Height sliders, algorithm switching
// with keys 1/2/3, Space to carve, r to reset, s to save, q/Esc to quit.
// Always carves from `original`, never from the previous result, so dragging
// a slider back up restores detail naturally. Returns when the user quits.
void runInteractive(const cv::Mat3b& original);
