// test_core_units.cpp — doctest tests for Javier's core functions (Javier's own test_core.cpp is kept as a separate test).
// so they work from the skeleton onwards.
//
//   test_energy   computeEnergy
//   test_remove   removeVerticalSeam
//   test_valid    isValidSeam
//   test_cost     seamCost
//   test_carve    carve (using a dumb straight-line finder)
#include "doctest.h"
#include "test_helpers.h"

#include <vector>

// ---------------------------------------------------------------- test_energy
TEST_CASE("test_energy: a single-colour image has zero energy everywhere") {
    cv::Mat3b img(15, 20, cv::Vec3b(80, 120, 200));
    cv::Mat1f e = computeEnergy(img);

    REQUIRE(e.rows == 15);
    REQUIRE(e.cols == 20);
    double minV = 0, maxV = 0;
    cv::minMaxLoc(e, &minV, &maxV);
    CHECK(maxV < 1e-6);  // nothing changes colour, so nothing is "important"
}

TEST_CASE("test_energy: left-black / right-white image peaks at the boundary") {
    const int rows = 10, cols = 20, edge = 10;  // columns 0..9 black, 10..19 white
    cv::Mat3b img(rows, cols, cv::Vec3b(0, 0, 0));
    for (int r = 0; r < rows; ++r)
        for (int c = edge; c < cols; ++c) img(r, c) = cv::Vec3b(255, 255, 255);

    cv::Mat1f e = computeEnergy(img);
    REQUIRE(e.rows == rows);
    REQUIRE(e.cols == cols);

    for (int r = 0; r < rows; ++r) {
        CAPTURE(r);
        int maxCol = 0;
        for (int c = 1; c < cols; ++c)
            if (e(r, c) > e(r, maxCol)) maxCol = c;
        // The strongest edge must be on one of the two columns touching the boundary.
        CHECK((maxCol == edge - 1 || maxCol == edge));
        CHECK(e(r, edge - 1) > 0.0f);
        CHECK(e(r, edge) > 0.0f);
        // Far from the boundary the colour is flat, so energy is zero.
        for (int c : {0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15, 16, 17, 18, 19}) {
            CAPTURE(c);
            CHECK(e(r, c) == doctest::Approx(0.0f));
        }
    }
}

// ---------------------------------------------------------------- test_remove
// Pixel (r, c) stores its own column number in channel 0 and row in channel 1,
// so after removal we can see exactly which pixel disappeared.
static cv::Mat3b labelledImage(int rows, int cols) {
    cv::Mat3b img(rows, cols);
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            img(r, c) = cv::Vec3b(static_cast<uchar>(c), static_cast<uchar>(r), 7);
    return img;
}

static void checkRemoved(const cv::Mat3b& in, const Seam& s) {
    cv::Mat3b out = removeVerticalSeam(in, s);
    REQUIRE(out.rows == in.rows);
    REQUIRE(out.cols == in.cols - 1);  // exactly one column narrower

    for (int r = 0; r < in.rows; ++r) {
        CAPTURE(r);
        CAPTURE(s[r]);
        int k = 0;
        for (int c = 0; c < in.cols; ++c) {
            if (c == s[r]) continue;  // this one should be gone
            CHECK(out(r, k)[0] == c);  // every other pixel stays, in order
            CHECK(out(r, k)[1] == r);
            ++k;
        }
    }
}

TEST_CASE("test_remove: output is one column narrower and loses the seam pixel") {
    cv::Mat3b img = labelledImage(4, 6);
    SUBCASE("wiggly seam")     { checkRemoved(img, Seam{1, 2, 2, 3}); }
    SUBCASE("left edge")       { checkRemoved(img, Seam{0, 0, 0, 0}); }
    SUBCASE("right edge")      { checkRemoved(img, Seam{5, 5, 5, 5}); }
    SUBCASE("does not modify the input") {
        cv::Mat3b before = img.clone();
        (void)removeVerticalSeam(img, Seam{1, 2, 2, 3});
        CHECK(cv::norm(before, img, cv::NORM_INF) == 0.0);
    }
}

// ----------------------------------------------------------------- test_valid
TEST_CASE("test_valid: isValidSeam accepts good seams and rejects bad ones") {
    const int rows = 5, cols = 6;
    CHECK(isValidSeam(Seam{2, 2, 2, 2, 2}, rows, cols));        // straight
    CHECK(isValidSeam(Seam{0, 1, 2, 3, 4}, rows, cols));        // diagonal, steps of 1
    CHECK(isValidSeam(Seam{5, 5, 4, 5, 5}, rows, cols));        // uses the last column
    CHECK_FALSE(isValidSeam(Seam{2, 2, 4, 4, 4}, rows, cols));  // jump of 2
    CHECK_FALSE(isValidSeam(Seam{2, 2, 2, 2}, rows, cols));     // too short
    CHECK_FALSE(isValidSeam(Seam{2, 2, 2, 2, 2, 2}, rows, cols)); // too long
    CHECK_FALSE(isValidSeam(Seam{-1, 0, 0, 0, 0}, rows, cols)); // column below 0
    CHECK_FALSE(isValidSeam(Seam{6, 5, 5, 5, 5}, rows, cols));  // column past the edge
}

// ------------------------------------------------------------------ test_cost
TEST_CASE("test_cost: seamCost adds up energy along the seam") {
    cv::Mat1f e = th::makeGrid({{1, 2, 3},
                                {4, 5, 6},
                                {7, 8, 9}});
    CHECK(seamCost(e, Seam{0, 0, 0}) == doctest::Approx(12.0f));
    CHECK(seamCost(e, Seam{0, 1, 2}) == doctest::Approx(15.0f));
    CHECK(seamCost(e, Seam{2, 1, 1}) == doctest::Approx(16.0f));
}

// ----------------------------------------------------------------- test_carve
TEST_CASE("test_carve: 20x15 image carved to 12x10 is exactly 12x10") {
    std::mt19937 rng(th::kSeed);
    cv::Mat3b img = th::randomImage(rng, 15, 20);  // 15 rows (height), 20 cols (width)
    th::StraightFinder finder;

    int calls = 0;
    cv::Mat3b out = carve(img, 12, 10, finder,
                          [&](const cv::Mat3b&, const Seam&, bool) { ++calls; });

    CHECK(out.cols == 12);
    CHECK(out.rows == 10);
    CHECK(out.type() == img.type());
    // 8 vertical seams (20 -> 12) + 5 horizontal seams (15 -> 10)
    CHECK(calls == 13);
}

TEST_CASE("test_carve: target equal to input size changes nothing") {
    std::mt19937 rng(th::kSeed + 1);
    cv::Mat3b img = th::randomImage(rng, 15, 20);
    th::StraightFinder finder;
    cv::Mat3b out = carve(img, 20, 15, finder);
    REQUIRE(out.size() == img.size());
    CHECK(cv::norm(out, img, cv::NORM_INF) == 0.0);
}
