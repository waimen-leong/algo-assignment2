// Sanity checks for core/ (energy + carver), from Javier's checklist.
// No framework: each CHECK prints on failure; exit code 1 if any failed.
#include "algos/stub_seam.h"
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

static void testEnergyFlat() {
    cv::Mat3b img(10, 10, cv::Vec3b(120, 60, 200));
    cv::Mat1f e = computeEnergy(img);
    CHECK(e.size() == img.size());
    CHECK(cv::countNonZero(e) == 0);
}

static void testEnergyEdge() {
    // Black left half, white right half -> energy only near the boundary.
    cv::Mat3b img(8, 10, cv::Vec3b(0, 0, 0));
    img.colRange(5, 10).setTo(cv::Vec3b(255, 255, 255));
    cv::Mat1f e = computeEnergy(img);
    CHECK(e(4, 0) == 0.0f);
    CHECK(e(4, 9) == 0.0f);
    CHECK(e(4, 4) > 0.0f);
    CHECK(e(4, 5) > 0.0f);
}

static void testRemoveSeam() {
    // 1x5 row [a b c d e], remove column 2 -> [a b d e]
    cv::Mat3b img(1, 5);
    for (int c = 0; c < 5; ++c) img(0, c) = cv::Vec3b(c, c, c);
    cv::Mat3b out = removeVerticalSeam(img, Seam{2});
    CHECK(out.rows == 1 && out.cols == 4);
    CHECK(out(0, 0)[0] == 0);
    CHECK(out(0, 1)[0] == 1);
    CHECK(out(0, 2)[0] == 3);
    CHECK(out(0, 3)[0] == 4);
}

static void testIsValidSeam() {
    CHECK(isValidSeam({1, 2, 1, 0}, 4, 3));
    CHECK(!isValidSeam({1, 2, 1}, 4, 3));     // wrong length
    CHECK(!isValidSeam({1, 3, 2, 1}, 4, 3));  // column out of range
    CHECK(!isValidSeam({0, 2, 1, 1}, 4, 3));  // jump of 2
    CHECK(!isValidSeam({-1, 0, 0, 0}, 4, 3)); // negative
}

static void testSeamCost() {
    cv::Mat1f e = (cv::Mat1f(3, 3) << 1, 2, 3,
                                      4, 5, 6,
                                      7, 8, 9);
    CHECK(seamCost(e, {0, 1, 2}) == 1 + 5 + 9);
    CHECK(seamCost(e, {1, 1, 1}) == 2 + 5 + 8);
}

static void testCarveSize() {
    cv::Mat3b img(30, 40);
    cv::randu(img, cv::Scalar::all(0), cv::Scalar::all(255));
    StubSeamFinder stub;

    cv::Mat3b out = carve(img, 25, 20, stub);
    CHECK(out.cols == 25 && out.rows == 20);

    int vertical = 0, horizontal = 0;
    carve(img, 35, 27, stub, [&](const cv::Mat3b& shown, const Seam& s, bool v) {
        // Callback image is always display orientation; seam length matches.
        if (v) { ++vertical;   CHECK(static_cast<int>(s.size()) == shown.rows); }
        else   { ++horizontal; CHECK(static_cast<int>(s.size()) == shown.cols); }
    });
    CHECK(vertical == 5 && horizontal == 3);

    // No-op and invalid targets
    CHECK(carve(img, 40, 30, stub).size() == img.size());
    bool threw = false;
    try { carve(img, 41, 30, stub); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
}

int main() {
    testEnergyFlat();
    testEnergyEdge();
    testRemoveSeam();
    testIsValidSeam();
    testSeamCost();
    testCarveSize();
    if (failures) { std::cerr << failures << " check(s) failed\n"; return 1; }
    std::cout << "test_core: all checks passed\n";
    return 0;
}
