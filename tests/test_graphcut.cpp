// Correctness checks for the graph-cut seam finder (Person D -> Person F).
// No framework: each CHECK prints on failure; exit code 1 if any failed.
//
// The key property: on any energy map, the max-flow value, the cost of the
// graph-cut seam, and the DP optimum are all equal. Seams may differ on ties;
// costs may not.
#include "algos/flow_graph.h"
#include "algos/graphcut_seam.h"
#include "core/carver.h"
#include "core/energy.h"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

static int failures = 0;
#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << __FILE__ << ":" << __LINE__ << "  FAILED: " #cond \
                      << "\n";                                             \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

// Reference optimum: textbook seam DP (cost only). Kept local so this test does
// not depend on B's finder; it can be swapped for DpSeamFinder once merged.
static double dpMinCost(const cv::Mat1f& e) {
    std::vector<double> prev(e.cols), cur(e.cols);
    for (int j = 0; j < e.cols; ++j) prev[j] = e(0, j);
    for (int i = 1; i < e.rows; ++i) {
        for (int j = 0; j < e.cols; ++j) {
            double best = prev[j];
            if (j > 0) best = std::min(best, prev[j - 1]);
            if (j + 1 < e.cols) best = std::min(best, prev[j + 1]);
            cur[j] = best + e(i, j);
        }
        std::swap(prev, cur);
    }
    return *std::min_element(prev.begin(), prev.end());
}

// ---- FlowGraph on its own ---------------------------------------------------

static void testMaxFlowTextbook() {
    // CLRS Figure 26.1: max flow 23, min cut {s, v1, v2, v4} | {v3, t}.
    enum { s, v1, v2, v3, v4, t };
    FlowGraph g(6);
    g.addEdge(s, v1, 16);  g.addEdge(s, v2, 13);  g.addEdge(v2, v1, 4);
    g.addEdge(v1, v3, 12); g.addEdge(v3, v2, 9);  g.addEdge(v2, v4, 14);
    g.addEdge(v4, v3, 7);  g.addEdge(v3, t, 20);  g.addEdge(v4, t, 4);
    CHECK(g.maxFlow(s, t) == 23);

    std::vector<bool> inS = g.reachableFromSource(s);
    CHECK(inS[s] && inS[v1] && inS[v2] && inS[v4]);
    CHECK(!inS[v3] && !inS[t]);
}

static void testMaxFlowNeedsResidual() {
    // Diamond with a cross edge: if the first path goes 0-1-2-3, reaching the
    // optimum of 2 requires pushing flow back along the residual edge 2->1.
    FlowGraph g(4);
    g.addEdge(0, 1, 1); g.addEdge(0, 2, 1); g.addEdge(1, 2, 1);
    g.addEdge(1, 3, 1); g.addEdge(2, 3, 1);
    CHECK(g.maxFlow(0, 3) == 2);
}

static void testMaxFlowEdgeCases() {
    FlowGraph disconnected(3);
    disconnected.addEdge(0, 1, 5);
    CHECK(disconnected.maxFlow(0, 2) == 0);
    CHECK(disconnected.maxFlow(1, 1) == 0);  // s == t

    // Parallel edges add up; a self-loop carries nothing and must not break rev.
    FlowGraph g(2);
    g.addEdge(0, 0, 7);
    g.addEdge(0, 1, 3);
    g.addEdge(0, 1, 4);
    CHECK(g.edgesFrom(0)[0].rev == 1 && g.edgesFrom(0)[1].rev == 0);
    CHECK(g.maxFlow(0, 1) == 7);
}

static void testAddEdgeReverseIndices() {
    // Every edge's partner must point straight back at it.
    FlowGraph g(4);
    g.addEdge(0, 1, 3); g.addEdge(1, 2, 2); g.addEdge(0, 2, 1); g.addEdge(2, 3, 5);
    bool ok = true;
    for (int u = 0; u < g.numNodes(); ++u)
        for (int i = 0; i < static_cast<int>(g.edgesFrom(u).size()); ++i) {
            const Edge& e = g.edgesFrom(u)[i];
            const Edge& back = g.edgesFrom(e.to)[e.rev];
            if (back.to != u || back.rev != i) ok = false;
        }
    CHECK(ok);
}

// ---- Seam graph pieces ------------------------------------------------------

static void testExtractSeam() {
    // seam[i] = last column of row i that is on the S side.
    std::vector<bool> inS = {1, 1, 0, 0,
                             1, 1, 1, 0,
                             1, 1, 0, 0};
    CHECK((extractSeam(inS, 3, 4) == Seam{1, 2, 1}));

    std::vector<bool> fullRow = {1, 1, 1};  // whole row on S side -> last column
    CHECK((extractSeam(fullRow, 1, 3) == Seam{2}));
}

static void testSeamGraphShape() {
    // H*W pixels + S + T; source/sink ids as documented.
    cv::Mat1f e(3, 4, 1.0f);
    FlowGraph g = buildSeamGraph(e);
    CHECK(g.numNodes() == 3 * 4 + 2);
    CHECK(seamSourceNode(3, 4) == 12 && seamSinkNode(3, 4) == 13);
    // S has exactly one outgoing INF edge per row (plus no others).
    int fromS = 0;
    for (const Edge& ed : g.edgesFrom(12))
        if (ed.cap > 0) { ++fromS; CHECK(ed.cap == FLOW_INF); }
    CHECK(fromS == 3);
}

// ---- GraphCutSeamFinder -----------------------------------------------------

static void testSmallHandExample() {
    cv::Mat1f e = (cv::Mat1f(3, 4) << 1, 4, 3, 5,
                                      2, 9, 1, 8,
                                      6, 3, 2, 7);
    GraphCutSeamFinder f;
    Seam s = f.find(e);
    CHECK(isValidSeam(s, 3, 4));
    // Two optimal seams tie at 6: {0, 0, 1} = 1+2+3 and {2, 2, 2} = 3+1+2.
    CHECK(seamCost(e, s) == 6);
    CHECK(f.lastFlow() == 6);
}

static void testLastColumnChoosable() {
    // Only the last column is free. With an INF edge to T (the brief's literal
    // wording) this column could never be cut; the refinement makes it work.
    cv::Mat1f e(4, 3, 9.0f);
    e.col(2).setTo(0.0f);
    GraphCutSeamFinder f;
    CHECK((f.find(e) == Seam{2, 2, 2, 2}));
    CHECK(f.lastFlow() == 0);
}

static void testDegenerateSizes() {
    GraphCutSeamFinder f;

    cv::Mat1f column(5, 1, 3.0f);  // W == 1: only one possible seam
    CHECK((f.find(column) == Seam(5, 0)));
    CHECK(f.lastFlow() == 15);

    cv::Mat1f row = (cv::Mat1f(1, 6) << 6, 5, 4, 3, 2, 1);  // H == 1: pick the min
    CHECK((f.find(row) == Seam{5}));
    CHECK(f.lastFlow() == 1);
}

static void testRandomGridsMatchDp() {
    // The key test: 1000 random integer grids, 1x1 .. 9x9.
    // Every third grid uses energies 0..3 to force lots of ties.
    std::mt19937 rng(2026);
    int bad = 0;
    for (int it = 0; it < 1000; ++it) {
        const int rows = 1 + static_cast<int>(rng() % 9);
        const int cols = 1 + static_cast<int>(rng() % 9);
        const unsigned maxE = (it % 3 == 0) ? 3 : 50;
        cv::Mat1f e(rows, cols);
        for (int i = 0; i < rows; ++i)
            for (int j = 0; j < cols; ++j)
                e(i, j) = static_cast<float>(rng() % (maxE + 1));

        GraphCutSeamFinder f;
        Seam s = f.find(e);
        const bool ok = isValidSeam(s, rows, cols) &&
                        static_cast<double>(f.lastFlow()) == seamCost(e, s) &&
                        seamCost(e, s) == dpMinCost(e);
        if (!ok && ++bad <= 3)
            std::cerr << "  grid " << it << " (" << rows << "x" << cols
                      << "): flow=" << f.lastFlow() << " graphcut=" << seamCost(e, s)
                      << " dp=" << dpMinCost(e) << "\n";
    }
    CHECK(bad == 0);
}

static void testSobelEnergy() {
    // Real energy from computeEnergy: values are whole numbers (so llround is
    // lossless) and the graph-cut seam is still optimal.
    cv::Mat3b img(40, 60);
    cv::randu(img, cv::Scalar::all(0), cv::Scalar::all(255));
    cv::GaussianBlur(img, img, cv::Size(5, 5), 0);  // photo-like smoothness
    cv::Mat1f e = computeEnergy(img);

    bool integral = true;
    for (int i = 0; i < e.rows; ++i)
        for (int j = 0; j < e.cols; ++j)
            if (e(i, j) != std::round(e(i, j))) integral = false;
    CHECK(integral);

    GraphCutSeamFinder f;
    Seam s = f.find(e);
    CHECK(isValidSeam(s, e.rows, e.cols));
    CHECK(static_cast<double>(f.lastFlow()) == dpMinCost(e));
}

static void testCarveWithGraphCut() {
    cv::Mat3b img(30, 40);
    cv::randu(img, cv::Scalar::all(0), cv::Scalar::all(255));
    GraphCutSeamFinder f;
    cv::Mat3b out = carve(img, 34, 26, f);  // 6 vertical + 4 horizontal seams
    CHECK(out.cols == 34 && out.rows == 26);
    CHECK(std::string(f.name()) == "graphcut");
}

int main() {
    testMaxFlowTextbook();
    testMaxFlowNeedsResidual();
    testMaxFlowEdgeCases();
    testAddEdgeReverseIndices();
    testExtractSeam();
    testSeamGraphShape();
    testSmallHandExample();
    testLastColumnChoosable();
    testDegenerateSizes();
    testRandomGridsMatchDp();
    testSobelEnergy();
    testCarveWithGraphCut();
    if (failures) { std::cerr << failures << " check(s) failed\n"; return 1; }
    std::cout << "test_graphcut: all checks passed\n";
    return 0;
}
