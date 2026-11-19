#pragma once
#include "algos/flow_graph.h"
#include "common/types.h"

// Seam finding as a minimum s-t cut. Owned by Person D.
//
// Pixel (i, j) of an H x W energy map is node i*W + j; the source is H*W and
// the sink H*W + 1. The S side of the minimum cut is, in every row, an unbroken
// run of pixels from column 0, and the seam is the last pixel of that run.

inline int seamSourceNode(int rows, int cols) { return rows * cols; }
inline int seamSinkNode(int rows, int cols) { return rows * cols + 1; }

// Builds the flow network for energy map e (H rows, W cols):
//   S -> (i, 0)                         INF       column 0 is always on the S side
//   (i, j) -> (i, j+1)                  e(i, j)   cutting it removes pixel (i, j)
//   (i, W-1) -> T                       e(i, W-1) lets the last column be chosen
//   (i, j+1) -> (i, j)                  INF       exactly one cut per row
//   (i, j) -> (i-1, j-1), (i+1, j-1)    INF       adjacent rows differ by <= 1 col
// Energies are rounded to the nearest integer (Sobel on 8-bit input is already
// integral, so nothing is lost there); negative values are clamped to 0.
FlowGraph buildSeamGraph(const cv::Mat1f& e);

// seam[i] = largest j with inS[i*W + j] true (inS from reachableFromSource).
Seam extractSeam(const std::vector<bool>& inS, int rows, int cols);

class GraphCutSeamFinder : public ISeamFinder {
public:
    Seam find(const cv::Mat1f& energy) override;
    const char* name() const override { return "graphcut"; }

    // Max-flow value of the most recent find() = cost of the seam it returned
    // (in rounded energy units). Handy for tests and the report.
    long long lastFlow() const { return lastFlow_; }

private:
    long long lastFlow_ = 0;
};
