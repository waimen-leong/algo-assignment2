// PLACEHOLDER by Person A so the pipeline runs end to end.
// Person E replaces this with the real app (parseArgs, makeFinder, UI).
//
// Usage: seamcarve --in input.jpg [--out out.png] [--width W] [--height H]
#include "algos/stub_seam.h"
#include "core/carver.h"

#include <opencv2/imgcodecs.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::string in, out = "out.png", algo = "stub";
    int width = -1, height = -1;

    for (int i = 1; i + 1 < argc; i += 2) {
        std::string key = argv[i], val = argv[i + 1];
        if (key == "--in") in = val;
        else if (key == "--out") out = val;
        else if (key == "--width") width = std::atoi(val.c_str());
        else if (key == "--height") height = std::atoi(val.c_str());
        else if (key == "--algo") algo = val;
        else { std::cerr << "unknown option " << key << "\n"; return 2; }
    }
    if (in.empty()) {
        std::cerr << "usage: seamcarve --in input.jpg [--out out.png] "
                     "[--width W] [--height H]\n";
        return 2;
    }

    cv::Mat3b img = cv::imread(in, cv::IMREAD_COLOR);
    if (img.empty()) { std::cerr << "cannot read " << in << "\n"; return 1; }
    if (width < 0) width = img.cols;
    if (height < 0) height = img.rows;

    if (algo != "stub")
        std::cerr << "note: only 'stub' exists in the skeleton, using it\n";
    StubSeamFinder finder;

    try {
        auto t0 = std::chrono::steady_clock::now();
        cv::Mat3b result = carve(img, width, height, finder);
        auto ms = std::chrono::duration<double, std::milli>(
                      std::chrono::steady_clock::now() - t0).count();

        cv::imwrite(out, result);
        std::cout << img.cols << "x" << img.rows << " -> " << result.cols << "x"
                  << result.rows << " using " << finder.name() << " in " << ms
                  << " ms, saved to " << out << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
