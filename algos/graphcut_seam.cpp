#include "algos/graphcut_seam.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
long long toCapacity(float energy) {
    return std::max(0LL, std::llround(energy));
}
}  // namespace

FlowGraph buildSeamGraph(const cv::Mat1f& e) {
    const int H = e.rows, W = e.cols;
    const int S = seamSourceNode(H, W), T = seamSinkNode(H, W);
    auto id = [W](int i, int j) { return i * W + j; };

    FlowGraph g(H * W + 2);
    for (int i = 0; i < H; ++i) {
        // Column 0 can never be on the T side.
        g.addEdge(S, id(i, 0), FLOW_INF);

        for (int j = 0; j + 1 < W; ++j) {
            // Forward: the only finite edges inside a row. Cutting the edge out
            // of (i, j) means "pixel (i, j) is the last S-side pixel = seam".
            g.addEdge(id(i, j), id(i, j + 1), toCapacity(e(i, j)));
            // Backward INF: if (i, j+1) is on the S side then (i, j) must be
            // too, so each row's S side is a prefix and is cut exactly once.
            g.addEdge(id(i, j + 1), id(i, j), FLOW_INF);
        }

        // Last column: finite edge carrying its energy, so choosing column W-1
        // costs e(i, W-1). (The brief's INF here would make column W-1
        // unchoosable; see the report's "last-column refinement".)
        g.addEdge(id(i, W - 1), T, toCapacity(e(i, W - 1)));

        // Diagonal INF: if (i, j) is on the S side so are (i-1, j-1) and
        // (i+1, j-1), so seam columns in adjacent rows differ by at most 1.
        for (int j = 1; j < W; ++j) {
            if (i > 0) g.addEdge(id(i, j), id(i - 1, j - 1), FLOW_INF);
            if (i + 1 < H) g.addEdge(id(i, j), id(i + 1, j - 1), FLOW_INF);
        }
    }
    return g;
}

Seam extractSeam(const std::vector<bool>& inS, int rows, int cols) {
    if (static_cast<long long>(inS.size()) < static_cast<long long>(rows) * cols)
        throw std::invalid_argument("extractSeam: inS is smaller than rows * cols");

    Seam seam(rows, 0);
    for (int i = 0; i < rows; ++i) {
        int last = 0;  // column 0 is always on the S side (INF source edge)
        for (int j = 0; j < cols; ++j)
            if (inS[static_cast<size_t>(i) * cols + j]) last = j;
        seam[i] = last;
    }
    return seam;
}

Seam GraphCutSeamFinder::find(const cv::Mat1f& energy) {
    if (energy.empty()) return {};
    const int H = energy.rows, W = energy.cols;
    const int S = seamSourceNode(H, W), T = seamSinkNode(H, W);

    FlowGraph g = buildSeamGraph(energy);
    lastFlow_ = g.maxFlow(S, T);
    return extractSeam(g.reachableFromSource(S), H, W);
}
