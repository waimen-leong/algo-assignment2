// test_algorithms.cpp — tests for the three seam finders.
//
// Each block only compiles if that algorithm has been merged (SeamDetect.cmake
// sets SEAM_HAVE_DP etc.), so main always builds even mid-week.
//
//   test_dp          B's example + DP equals brute force on 100 random 5x5 grids
//   test_dp_lowmem   low-memory DP gives the identical seam to normal DP
//   test_greedy      C's counter-example costs 19 + greedy is never cheaper than optimal
//   test_graphcut    graph cut cost equals the optimum on 100 random 8x8 grids
//   test_carve_all   every finder carves 20x15 down to 12x10
//
// About the two hand-made grids below: they were built to have exactly the
// answers the plan asks for (DP: seam {0,1,1}, cost 5; greedy: cost 19), with no
// ties, so the tie rule doesn't matter. If B or C use different example grids in
// the report, paste theirs in instead so the report and tests match.
#include "doctest.h"
#include "test_helpers.h"

#include <memory>
#include <string>
#include <vector>

namespace {

// 3 rows x 4 columns. Cheapest path: 1 (col 0) -> 2 (col 1) -> 2 (col 1) = 5.
[[maybe_unused]] cv::Mat1f dpExample() {
    return th::makeGrid({{1, 5, 9, 9},
                         {9, 2, 9, 9},
                         {9, 2, 9, 9}});
}

// 3 x 3 grid where greedy is fooled. Greedy starts at the cheapest pixel in
// row 0 (the 1), then the 9s look better than the 50s, so it takes 1+9+9 = 19.
// The real best seam is 2 -> 1 -> 1 = 4, which starts at a slightly worse pixel.
[[maybe_unused]] cv::Mat1f greedyCounterExample() {
    return th::makeGrid({{1, 2, 50},
                         {9, 50, 1},
                         {9, 50, 1}});
}

}  // namespace

// --------------------------------------------------------------------- test_dp
#ifdef SEAM_HAVE_DP
TEST_CASE("test_dp: hand-worked 3x4 example returns {0,1,1} with cost 5") {
    DPSeamFinder dp;
    cv::Mat1f e = dpExample();
    Seam s = dp.find(e);
    CHECK(s == Seam{0, 1, 1});
    CHECK(seamCost(e, s) == doctest::Approx(5.0f));
}

TEST_CASE("test_dp: cost equals brute force on 100 random 5x5 grids") {
    std::mt19937 rng(th::kSeed);
    DPSeamFinder dp;
    for (int i = 0; i < 100; ++i) {
        CAPTURE(i);  // if this fails, the message says which grid number
        cv::Mat1f e = th::randomIntGrid(rng, 5, 5);
        Seam s = dp.find(e);
        REQUIRE(isValidSeam(s, e.rows, e.cols));
        CHECK(th::seamSum(e, s) == th::bruteForceMinCost(e));
    }
}

TEST_CASE("test_dp: works on non-square grids and single-column grids") {
    std::mt19937 rng(th::kSeed + 2);
    DPSeamFinder dp;
    for (auto [rows, cols] : std::vector<std::pair<int, int>>{{6, 3}, {3, 7}, {5, 1}, {1, 5}}) {
        CAPTURE(rows);
        CAPTURE(cols);
        cv::Mat1f e = th::randomIntGrid(rng, rows, cols);
        Seam s = dp.find(e);
        REQUIRE(isValidSeam(s, rows, cols));
        CHECK(th::seamSum(e, s) == th::bruteForceMinCost(e));
    }
}
#endif

// -------------------------------------------------------------- test_dp_lowmem
#if defined(SEAM_HAVE_DP) && defined(SEAM_HAVE_DP_LOWMEM)
TEST_CASE("test_dp_lowmem: identical seam to normal DP") {
    DPSeamFinder dp;
    DPSeamFinderLowMem low;
    CHECK(low.find(dpExample()) == dp.find(dpExample()));

    // Decimal energies, so two different seams with the same cost (a tie)
    // practically never happens and "identical seam" is a fair demand.
    std::mt19937 rng(th::kSeed + 3);
    for (int i = 0; i < 100; ++i) {
        CAPTURE(i);
        cv::Mat1f e = th::randomFloatGrid(rng, 12, 10);
        CHECK(low.find(e) == dp.find(e));
    }
}
#endif

// ----------------------------------------------------------------- test_greedy
#ifdef SEAM_HAVE_GREEDY
TEST_CASE("test_greedy: counter-example — greedy pays 19, best seam costs 4") {
    GreedySeamFinder greedy;
    cv::Mat1f e = greedyCounterExample();
    Seam s = greedy.find(e);
    REQUIRE(isValidSeam(s, e.rows, e.cols));
    CHECK(s == Seam{0, 0, 0});
    CHECK(seamCost(e, s) == doctest::Approx(19.0f));
    CHECK(th::bruteForceMinCost(e) == doctest::Approx(4.0f));
}

TEST_CASE("test_greedy: valid seam, never cheaper than the optimum") {
    std::mt19937 rng(th::kSeed + 4);
    GreedySeamFinder greedy;
    for (int i = 0; i < 100; ++i) {
        CAPTURE(i);
        cv::Mat1f e = th::randomIntGrid(rng, 5, 5);
        Seam s = greedy.find(e);
        REQUIRE(isValidSeam(s, e.rows, e.cols));
        CHECK(th::seamSum(e, s) >= th::bruteForceMinCost(e));
    }
}
#endif

// --------------------------------------------------------------- test_graphcut
#ifdef SEAM_HAVE_GRAPHCUT
TEST_CASE("test_graphcut: hand-worked 3x4 example costs 5") {
    GraphCutSeamFinder gc;
    cv::Mat1f e = dpExample();
    Seam s = gc.find(e);
    REQUIRE(isValidSeam(s, e.rows, e.cols));
    CHECK(seamCost(e, s) == doctest::Approx(5.0f));
}

// The key test: DP and graph cut are both optimal, so their costs must match,
// even if they choose different seams of equal cost.
TEST_CASE("test_graphcut: cost equals DP / brute force on 100 random 8x8 grids") {
    std::mt19937 rng(th::kSeed + 5);
    GraphCutSeamFinder gc;
#ifdef SEAM_HAVE_DP
    DPSeamFinder dp;
#endif
    for (int i = 0; i < 100; ++i) {
        CAPTURE(i);
        cv::Mat1f e = th::randomIntGrid(rng, 8, 8);
        Seam s = gc.find(e);
        REQUIRE(isValidSeam(s, e.rows, e.cols));
        const float gcCost = th::seamSum(e, s);
        CHECK(gcCost == th::bruteForceMinCost(e));
#ifdef SEAM_HAVE_DP
        CHECK(gcCost == th::seamSum(e, dp.find(e)));
#endif
    }
}
#endif

// -------------------------------------------------------------- test_carve_all
namespace {
std::vector<std::unique_ptr<ISeamFinder>> allFinders() {
    std::vector<std::unique_ptr<ISeamFinder>> v;
#ifdef SEAM_HAVE_DP
    v.push_back(std::make_unique<DPSeamFinder>());
#endif
#ifdef SEAM_HAVE_DP_LOWMEM
    v.push_back(std::make_unique<DPSeamFinderLowMem>());
#endif
#ifdef SEAM_HAVE_GREEDY
    v.push_back(std::make_unique<GreedySeamFinder>());
#endif
#ifdef SEAM_HAVE_GRAPHCUT
    v.push_back(std::make_unique<GraphCutSeamFinder>());
#endif
    return v;
}
}  // namespace

TEST_CASE("test_carve_all: every finder carves 20x15 down to 12x10") {
    auto finders = allFinders();
    if (finders.empty()) {
        MESSAGE("no seam finders merged yet — skipping");
        return;
    }
    std::mt19937 rng(th::kSeed + 6);
    cv::Mat3b img = th::randomImage(rng, 15, 20);

    for (auto& f : finders) {
        CAPTURE(std::string(f->name()));
        int calls = 0;
        int badSeams = 0;
        cv::Mat3b out = carve(img, 12, 10, *f, [&](const cv::Mat3b& cur, const Seam& s, bool vertical) {
            ++calls;
            if (vertical && !isValidSeam(s, cur.rows, cur.cols)) ++badSeams;
        });
        CHECK(out.cols == 12);
        CHECK(out.rows == 10);
        CHECK(calls == 13);
        CHECK(badSeams == 0);
    }
}
