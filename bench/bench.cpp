// bench.cpp — times each seam finder and records seam cost (section 5(c)).
//
// For every algorithm and every image size it removes 20 vertical seams,
// 3 times, and averages. It writes two files:
//
//   results.csv          algo,width,height,seams,ms_per_seam,total_seam_cost
//   timing_breakdown.csv algo,width,height,energy_ms_per_seam,find_ms_per_seam,remove_ms_per_seam
//
// The second file splits the time into "computing the energy map" versus
// "finding the seam" (and removing it), which shows where the time really goes.
//
// Build in Release (the CI and README commands already do this), then run:
//   ./build/bench/seam_bench                       # synthetic test image
//   ./build/bench/seam_bench --image photo.jpg     # your own image, resized to each size
//   ./build/bench/seam_bench --graphcut-max-width 400   # if Jun Siang says 400x300 is feasible
#include "seam_api.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#ifdef __GLIBC__
#include <malloc.h>
#endif

struct BenchResult {
    std::string algo;
    int w, h, seams;
    double msPerSeam;   // energy + find + remove, per seam
    double totalCost;   // sum of the costs of all seams removed
    // Extra: the split of msPerSeam, for timing_breakdown.csv
    double energyMsPerSeam = 0, findMsPerSeam = 0, removeMsPerSeam = 0;
};

using Clock = std::chrono::steady_clock;
static double msSince(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

// One run: remove nSeams vertical seams from img with finder f.
BenchResult runBench(const cv::Mat3b& img, ISeamFinder& f, int nSeams) {
    BenchResult res{f.name(), img.cols, img.rows, nSeams, 0.0, 0.0};
    cv::Mat3b cur = img.clone();
    double energyMs = 0, findMs = 0, removeMs = 0;

    for (int k = 0; k < nSeams; ++k) {
        auto t0 = Clock::now();
        cv::Mat1f energy = computeEnergy(cur);
        energyMs += msSince(t0);

        auto t1 = Clock::now();
        Seam s = f.find(energy);
        findMs += msSince(t1);

        res.totalCost += seamCost(energy, s);  // not timed

        auto t2 = Clock::now();
        cur = removeVerticalSeam(cur, s);
        removeMs += msSince(t2);
    }
    res.energyMsPerSeam = energyMs / nSeams;
    res.findMsPerSeam = findMs / nSeams;
    res.removeMsPerSeam = removeMs / nSeams;
    res.msPerSeam = res.energyMsPerSeam + res.findMsPerSeam + res.removeMsPerSeam;
    return res;
}

// A test image with smooth areas, sharp edges and texture, so the seams have
// something to avoid. Same every run (fixed seed).
static cv::Mat3b syntheticImage(int w, int h) {
    cv::Mat3b img(h, w);
    for (int r = 0; r < h; ++r)
        for (int c = 0; c < w; ++c)
            img(r, c) = cv::Vec3b(static_cast<uchar>(120 + 100 * r / h),   // sky-ish gradient
                                  static_cast<uchar>(150 + 60 * c / w),
                                  static_cast<uchar>(200 - 80 * r / h));
    cv::rectangle(img, cv::Rect(w / 8, h / 2, w / 5, h / 3), cv::Scalar(40, 60, 160), cv::FILLED);
    cv::circle(img, cv::Point(w * 2 / 3, h * 2 / 5), std::min(w, h) / 6, cv::Scalar(30, 200, 240), cv::FILLED);
    cv::rectangle(img, cv::Rect(0, h * 5 / 6, w, h / 6), cv::Scalar(60, 120, 60), cv::FILLED);  // ground
    // Add a little grain (+/- noise) so flat areas aren't perfectly flat.
    cv::Mat noise(img.size(), CV_16SC3), wide;
    cv::RNG rng(12345);
    rng.fill(noise, cv::RNG::NORMAL, 0, 12);
    img.convertTo(wide, CV_16SC3);
    wide += noise;
    wide.convertTo(img, CV_8UC3);  // clamps back into 0..255
    return img;
}

static void usage() {
    std::cout << "Usage: seam_bench [--image path] [--seams 20] [--runs 3]\n"
                 "                  [--graphcut-max-width 200] [--out results.csv]\n"
                 "                  [--breakdown timing_breakdown.csv]\n";
}

int main(int argc, char** argv) {
#ifdef __GLIBC__
    // Every seam allocates fresh multi-MB buffers (energy map, Sobel temporaries,
    // DP table). By default glibc hands those back to the OS after each call and
    // gets new pages next time, and page faults (slow on WSL) then swamp the
    // timing: energy at 800x600 measured 8.5 ms instead of 0.57 ms. Keep freed
    // memory for reuse so we time the algorithms, not the OS. Benchmark only.
    mallopt(M_MMAP_THRESHOLD, 32 * 1024 * 1024);
    mallopt(M_TRIM_THRESHOLD, 256 * 1024 * 1024);
#endif
    std::string imagePath, outPath = "results.csv", breakdownPath = "timing_breakdown.csv";
    int nSeams = 20, nRuns = 3, graphcutMaxWidth = 200;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) { usage(); std::exit(1); }
            return argv[++i];
        };
        if (a == "--image") imagePath = next();
        else if (a == "--seams") nSeams = std::stoi(next());
        else if (a == "--runs") nRuns = std::stoi(next());
        else if (a == "--graphcut-max-width") graphcutMaxWidth = std::stoi(next());
        else if (a == "--out") outPath = next();
        else if (a == "--breakdown") breakdownPath = next();
        else { usage(); return a == "--help" ? 0 : 1; }
    }

    cv::Mat3b source;
    if (!imagePath.empty()) {
        source = cv::imread(imagePath, cv::IMREAD_COLOR);
        if (source.empty()) { std::cerr << "Could not read " << imagePath << "\n"; return 1; }
    }

    const std::vector<cv::Size> sizes = {{100, 75}, {200, 150}, {400, 300}, {800, 600}};

    struct Entry { std::unique_ptr<ISeamFinder> finder; bool isGraphCut; };
    std::vector<Entry> finders;
#ifdef SEAM_HAVE_DP
    finders.push_back({std::make_unique<DPSeamFinder>(), false});
#endif
#ifdef SEAM_HAVE_DP_LOWMEM
    finders.push_back({std::make_unique<DPSeamFinderLowMem>(), false});
#endif
#ifdef SEAM_HAVE_GREEDY
    finders.push_back({std::make_unique<GreedySeamFinder>(), false});
#endif
#ifdef SEAM_HAVE_GRAPHCUT
    finders.push_back({std::make_unique<GraphCutSeamFinder>(), true});
#endif
    if (finders.empty()) { std::cerr << "No seam finders found to benchmark.\n"; return 1; }

    std::ofstream out(outPath), brk(breakdownPath);
    out << "algo,width,height,seams,ms_per_seam,total_seam_cost\n";
    brk << "algo,width,height,energy_ms_per_seam,find_ms_per_seam,remove_ms_per_seam\n";
    out << std::fixed << std::setprecision(4);
    brk << std::fixed << std::setprecision(4);

    for (const cv::Size& sz : sizes) {
        cv::Mat3b img;
        if (source.empty()) img = syntheticImage(sz.width, sz.height);
        else cv::resize(source, img, sz, 0, 0, cv::INTER_AREA);

        for (auto& e : finders) {
            if (e.isGraphCut && sz.width > graphcutMaxWidth) {
                std::cout << "skip  " << e.finder->name() << " at " << sz.width << "x" << sz.height
                          << " (above --graphcut-max-width " << graphcutMaxWidth << ")\n";
                continue;
            }
            BenchResult avg{e.finder->name(), sz.width, sz.height, nSeams, 0, 0};
            for (int run = 0; run < nRuns; ++run) {
                BenchResult r = runBench(img, *e.finder, nSeams);
                avg.msPerSeam += r.msPerSeam / nRuns;
                avg.energyMsPerSeam += r.energyMsPerSeam / nRuns;
                avg.findMsPerSeam += r.findMsPerSeam / nRuns;
                avg.removeMsPerSeam += r.removeMsPerSeam / nRuns;
                avg.totalCost = r.totalCost;  // same every run: the algorithms are deterministic
            }
            out << avg.algo << ',' << avg.w << ',' << avg.h << ',' << avg.seams << ','
                << avg.msPerSeam << ',' << avg.totalCost << '\n';
            brk << avg.algo << ',' << avg.w << ',' << avg.h << ',' << avg.energyMsPerSeam << ','
                << avg.findMsPerSeam << ',' << avg.removeMsPerSeam << '\n';
            std::cout << std::left << std::setw(10) << avg.algo << std::right << std::setw(4) << sz.width
                      << "x" << std::setw(3) << sz.height << "  " << std::setw(10) << std::setprecision(3)
                      << avg.msPerSeam << " ms/seam   cost " << std::setprecision(1) << avg.totalCost
                      << std::setprecision(4) << "\n";
            out.flush();
            brk.flush();
        }
    }
    std::cout << "Wrote " << outPath << " and " << breakdownPath << "\n";
    return 0;
}
