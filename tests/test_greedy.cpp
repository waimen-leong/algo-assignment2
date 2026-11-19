// Sanity checks for Greedy Seam Approach by wai men
// No framework: each CHECK prints on failure; exit code 1 if any failed.
#include "algos/greedy_seam.h"
#include "core/carver.h"
#include "core/energy.h"
#include "core/energy_mt.h"

#include <opencv2/imgproc.hpp>


#include <iostream>
#include <stdexcept>

static int failures = 0;
#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << __FILE__ << ":" << __LINE__ << "  FAILED: " #cond \
                      << "\n";                                             \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

static void TestCounterExample()
{
    cv::Mat1f e = (cv::Mat1f(3,3) << 1,2,3,
                                     9,9,1,
                                     9,9,1);

    GreedySeamFinder g;
    Seam s = g.find(e);
    CHECK((s == Seam{0,0,0}));
    CHECK((seamCost(e,s) == 19.0f));
}

static void TestTiesLeftMost()
{
    cv::Mat1f e = (cv::Mat1f(4,5) << 1,1,1,1,1,
                                     1,1,1,1,1,
                                     1,1,1,1,1,
                                     1,1,1,1,1);

    GreedySeamFinder g;
    Seam s = g.find(e);
    CHECK((s == Seam{0,0,0,0}));
}

static void TestRightEdge()
{
    cv::Mat1f e = (cv::Mat1f(5,4) << 1,1,1,0,
                                     1,1,1,0,
                                     1,1,1,0,
                                     1,1,1,0,
                                     1,1,1,0);
    
    GreedySeamFinder g;
    Seam s = g.find(e);
    CHECK((isValidSeam(s, 5, 4)));
    CHECK((s == Seam{3,3,3,3,3}));
}

static void TestRandomValid() 
{
    cv::RNG rng(12345);                       // fixed seed -> same grids every run
    GreedySeamFinder g;
    for (int t = 0; t < 100; ++t) {
        int rows = rng.uniform(1, 30);        // 1..29 (upper bound excluded)
        int cols = rng.uniform(1, 30);
        cv::Mat1f e(rows, cols);
        cv::randu(e, 0.0f, 100.0f);           // fill with random energies
        Seam s = g.find(e);
        CHECK(isValidSeam(s, rows, cols));
    }
}

// ST energy must equal |Sobel x| + |Sobel y| (OpenCV, ksize 3) on the
// interior, and be exactly 0 on the 1-pixel border.
static void TestSingleThread()
{
    cv::RNG rng(777);
    const cv::Size sizes[] = { {53, 37}, {3, 3}, {200, 150}, {4, 101} };   // (cols, rows)

    for (const cv::Size& sz : sizes)
    {
        cv::Mat3b img(sz);
        rng.fill(img, cv::RNG::UNIFORM, 0, 256);
        cv::Mat1b gray;
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

        cv::Mat sx, sy;
        cv::Sobel(gray, sx, CV_32F, 1, 0, 3);
        cv::Sobel(gray, sy, CV_32F, 0, 1, 3);
        cv::Mat1f ref = cv::abs(sx) + cv::abs(sy);

        cv::Mat1f e = ComputeEnergyST(gray);
        CHECK(e.size() == gray.size());

        // Interior: must match OpenCV.
        cv::Rect inner(1, 1, sz.width - 2, sz.height - 2);
        CHECK(cv::norm(e(inner), ref(inner), cv::NORM_INF) < 1e-3);

        // Border: first/last row and first/last column are all 0.
        CHECK(cv::countNonZero(e.row(0)) == 0);
        CHECK(cv::countNonZero(e.row(e.rows - 1)) == 0);
        CHECK(cv::countNonZero(e.col(0)) == 0);
        CHECK(cv::countNonZero(e.col(e.cols - 1)) == 0);
    }
}

// MT must give exactly the same output as ST for every size and thread count,
// including tiny images (< 3 rows/cols) and more threads than rows.
static void TestMultiThread()
{
    cv::RNG rng(4242);
    const cv::Size sizes[] = { {1, 1}, {2, 3}, {3, 2}, {3, 3}, {7, 5},
                               {53, 37}, {200, 150} };                    // (cols, rows)
    const int threadCounts[] = { 0, 1, 2, 3, 4, 8, 64 };                  // 0 must clamp to 1

    for (const cv::Size& sz : sizes)
    {
        cv::Mat1b gray(sz);
        rng.fill(gray, cv::RNG::UNIFORM, 0, 256);
        cv::Mat1f st = ComputeEnergyST(gray);

        for (int t : threadCounts)
        {
            cv::Mat1f mt = ComputeEnergyMT(gray, t);
            CHECK(mt.size() == st.size());
            CHECK(cv::norm(mt, st, cv::NORM_INF) == 0.0);
        }
    }
}

int main() {

    TestCounterExample();
    TestTiesLeftMost();
    TestRightEdge();
    TestRandomValid();
    TestSingleThread();
    TestMultiThread();

    if (failures) { std::cerr << failures << " check(s) failed\n"; return 1; }
    std::cout << "test_greedy: all checks passed\n";
    return 0;
}
