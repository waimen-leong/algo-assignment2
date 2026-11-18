// Sanity checks for Greedy Seam Approach by wai men
// No framework: each CHECK prints on failure; exit code 1 if any failed.
#include "algos/greedy_seam.h"
#include "core/carver.h"
#include "core/energy.h"


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

int main() {

    TestCounterExample();
    TestTiesLeftMost();
    TestRightEdge();
    TestRandomValid();

    if (failures) { std::cerr << failures << " check(s) failed\n"; return 1; }
    std::cout << "test_greedy: all checks passed\n";
    return 0;
}
