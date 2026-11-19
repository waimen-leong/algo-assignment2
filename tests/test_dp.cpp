// Sanity checks for the dynamic programming seam finder (DPSeamFinder and
// DPSeamFinderLowMem). each CHECK prints on failure, exit code 1 if any failed.
#include "algos/dp_seam.h"
#include "core/carver.h"

#include <algorithm>
#include <iostream>
#include <limits>

static int failures = 0;
#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << __FILE__ << ":" << __LINE__ << "  FAILED: " #cond \
                      << "\n";                                             \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

// Ground truth: try every seam starting from (r, c), return the cheapest.
// Exponential, so only used on tiny grids.
static float bruteFrom(const cv::Mat1f& e, int r, int c)
{
    if (r == e.rows - 1) return e(r, c);
    float best = std::numeric_limits<float>::infinity();
    for (int dc = -1; dc <= 1; ++dc) {
        int nc = c + dc;
        if (nc >= 0 && nc < e.cols) best = std::min(best, bruteFrom(e, r + 1, nc));
    }
    return e(r, c) + best;
}

static float bruteForceMinCost(const cv::Mat1f& e)
{
    float best = std::numeric_limits<float>::infinity();
    for (int c = 0; c < e.cols; ++c) best = std::min(best, bruteFrom(e, 0, c));
    return best;
}

// hand-worked 3x4 example: cheapest path 1 -> 2 -> 2 = 5.
static cv::Mat1f dpExample()
{
    return (cv::Mat1f(3, 4) << 1, 5, 9, 9,
                               9, 2, 9, 9,
                               9, 2, 9, 9);
}

static void TestHandExample()
{
    cv::Mat1f e = dpExample();
    DPSeamFinder dp;
    Seam s = dp.find(e);
    CHECK((s == Seam{0, 1, 1}));
    CHECK((seamCost(e, s) == 5.0f));
}

// Every entry of M must be e(i,j) + min of the (up to) three cells above it.
static void TestCostTable()
{
    cv::Mat1f M = buildCostTable(dpExample());
    cv::Mat1f expected = (cv::Mat1f(3, 4) <<  1,  5,  9,  9,
                                             10,  3, 14, 18,
                                             12,  5, 12, 23);
    CHECK(M.size() == expected.size());
    CHECK(cv::norm(M, expected, cv::NORM_INF) == 0.0);
}

// Tie rule: on equal costs the LEFTMOST column wins, at the bottom row and
// at every step of the backtrack.
static void TestTiesLeftMost()
{
    cv::Mat1f flat(4, 5, 1.0f);
    DPSeamFinder dp;
    CHECK((dp.find(flat) == Seam{0, 0, 0, 0}));

    // Two equally cheap seams in columns 1 and 3: column 1 must win.
    cv::Mat1f e = (cv::Mat1f(3, 5) << 9, 1, 9, 1, 9,
                                      9, 1, 9, 1, 9,
                                      9, 1, 9, 1, 9);
    CHECK((dp.find(e) == Seam{1, 1, 1}));
}

static void TestEdges()
{
    DPSeamFinder dp;

    // Cheap column on the far right: must reach it without going out of range.
    cv::Mat1f right = (cv::Mat1f(5, 4) << 1, 1, 1, 0,
                                          1, 1, 1, 0,
                                          1, 1, 1, 0,
                                          1, 1, 1, 0,
                                          1, 1, 1, 0);
    CHECK((dp.find(right) == Seam{3, 3, 3, 3, 3}));

    // Single column and single row.
    cv::Mat1f col = (cv::Mat1f(4, 1) << 3, 1, 4, 1);
    CHECK((dp.find(col) == Seam{0, 0, 0, 0}));
    cv::Mat1f row = (cv::Mat1f(1, 5) << 4, 2, 7, 1, 3);
    CHECK((dp.find(row) == Seam{3}));
}

// On random whole-number grids the DP cost must equal brute force
// (whole numbers add up exactly, so == is safe).
static void TestRandomVsBruteForce()
{
    cv::RNG rng(20261119);  // fixed seed -> same grids every run
    DPSeamFinder dp;
    for (int t = 0; t < 100; ++t) {
        int rows = rng.uniform(1, 7);  // 1..6
        int cols = rng.uniform(1, 7);
        cv::Mat1f e(rows, cols);
        for (int i = 0; i < rows; ++i)
            for (int j = 0; j < cols; ++j)
                e(i, j) = static_cast<float>(rng.uniform(0, 10));
        Seam s = dp.find(e);
        CHECK(isValidSeam(s, rows, cols));
        CHECK(seamCost(e, s) == bruteForceMinCost(e));
    }
}

// The low-memory version must return exactly the same seam, ties included.
static void TestLowMemMatches()
{
    cv::RNG rng(4242);
    DPSeamFinder dp;
    DPSeamFinderLowMem low;
    CHECK(low.find(dpExample()) == dp.find(dpExample()));

    cv::Mat1f flat(6, 7, 2.0f);
    CHECK(low.find(flat) == dp.find(flat));

    for (int t = 0; t < 100; ++t) {
        int rows = rng.uniform(1, 40);
        int cols = rng.uniform(1, 40);
        cv::Mat1f e(rows, cols);
        // Small integer range on purpose: lots of ties, which is where the
        // two versions would disagree if their tie rules differed.
        for (int i = 0; i < rows; ++i)
            for (int j = 0; j < cols; ++j)
                e(i, j) = static_cast<float>(rng.uniform(0, 4));
        CHECK(low.find(e) == dp.find(e));
    }
}

static void TestCarve()
{
    cv::RNG rng(777);
    cv::Mat3b img(15, 20);  // 20 wide x 15 tall
    rng.fill(img, cv::RNG::UNIFORM, 0, 256);

    DPSeamFinder dp;
    cv::Mat3b out = carve(img, 12, 10, dp);
    CHECK(out.cols == 12 && out.rows == 10);

    DPSeamFinderLowMem low;
    cv::Mat3b outLow = carve(img, 12, 10, low);
    CHECK(outLow.size() == out.size());
    CHECK(cv::norm(outLow, out, cv::NORM_INF) == 0.0);
}

int main()
{
    TestHandExample();
    TestCostTable();
    TestTiesLeftMost();
    TestEdges();
    TestRandomVsBruteForce();
    TestLowMemMatches();
    TestCarve();

    if (failures) { std::cerr << failures << " check(s) failed\n"; return 1; }
    std::cout << "test_dp: all checks passed\n";
    return 0;
}
