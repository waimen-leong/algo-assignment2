#include "app/ui.h"

#include "app/args.h"
#include "app/viz.h"
#include "core/carver.h"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>

namespace {
const char* kWindow = "Seam carving - interactive";

// Graph cut is slow; above this many pixels we downscale before carving it
// and tell the user so the window stays responsive.
constexpr int kGraphCutPixelLimit = 400 * 400;

struct State {
    cv::Mat3b original;   // never modified
    cv::Mat3b result;     // what the window currently shows
    std::string algo = "dp";
    double lastMs = 0.0;
    int targetW;          // driven by the Width slider
    int targetH;          // driven by the Height slider
    bool working = false; // true while a carve runs (suppresses re-entry)
};

// Trackbar callbacks only need to update the stored target; the carve itself
// happens on Space so dragging stays cheap.
void onWidth(int pos, void* userdata) {
    auto* st = static_cast<State*>(userdata);
    st->targetW = std::max(1, pos);
}
void onHeight(int pos, void* userdata) {
    auto* st = static_cast<State*>(userdata);
    st->targetH = std::max(1, pos);
}

cv::Mat3b withStatus(const cv::Mat3b& img, const State& st, const char* note) {
    cv::Mat3b canvas = onCanvas(img, st.original.cols, st.original.rows);
    char line[160];
    std::snprintf(line, sizeof(line), "algo=%s  size=%dx%d  last=%.0f ms",
                  st.algo.c_str(), img.cols, img.rows, st.lastMs);

    // Dark strip behind the text so it is readable on any image.
    cv::rectangle(canvas, cv::Rect(0, 0, canvas.cols, 22),
                  cv::Scalar(0, 0, 0), cv::FILLED);
    cv::putText(canvas, line, cv::Point(6, 16), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
    if (note && note[0]) {
        cv::rectangle(canvas, cv::Rect(0, 22, canvas.cols, 22),
                      cv::Scalar(0, 0, 0), cv::FILLED);
        cv::putText(canvas, note, cv::Point(6, 38), cv::FONT_HERSHEY_SIMPLEX,
                    0.5, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
    }
    return canvas;
}

void show(const cv::Mat3b& img, const State& st, const char* note = "") {
    cv::imshow(kWindow, withStatus(img, st, note));
}

// Carve the original down to the slider sizes, animating each seam.
void runCarve(State& st) {
    if (st.working) return;
    st.working = true;

    cv::Mat3b src = st.original;
    bool downscaled = false;
    if (st.algo == "graphcut" &&
        st.original.cols * st.original.rows > kGraphCutPixelLimit) {
        double f = std::sqrt(static_cast<double>(kGraphCutPixelLimit) /
                             (st.original.cols * st.original.rows));
        cv::resize(st.original, src, cv::Size(), f, f, cv::INTER_AREA);
        downscaled = true;
    }

    int tw = std::min(st.targetW, src.cols);
    int th = std::min(st.targetH, src.rows);

    if (downscaled)
        show(src, st, "graph cut: downscaled for speed, working...");
    else if (st.algo == "graphcut")
        show(src, st, "graph cut is slow, working...");

    auto finder = makeFinder(st.algo);
    const bool usingStub = std::string(finder->name()) != st.algo;

    // Animate: redraw the window on every seam the core reports.
    auto cb = [&](const cv::Mat3b& img, const Seam& s, bool vertical) {
        show(drawSeam(img, s, vertical), st,
             usingStub ? "(stub finder - real algo not merged yet)" : "");
        cv::waitKey(15);
    };

    try {
        auto t0 = std::chrono::steady_clock::now();
        st.result = carve(src, tw, th, *finder, cb);
        st.lastMs = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - t0).count();
    } catch (const std::exception& ex) {
        std::cerr << "carve failed: " << ex.what() << "\n";
    }

    show(st.result, st, usingStub ? "(stub finder - real algo not merged yet)"
                                  : "");
    st.working = false;
}

void save(const State& st) {
    char name[64];
    static int n = 0;
    std::snprintf(name, sizeof(name), "interactive_%03d.png", ++n);
    if (cv::imwrite(name, st.result))
        std::cout << "saved " << name << " (" << st.result.cols << "x"
                  << st.result.rows << ")\n";
    else
        std::cerr << "could not save " << name << "\n";
}
}  // namespace

void runInteractive(const cv::Mat3b& original) {
    State st;
    st.original = original;
    st.result = original.clone();
    st.targetW = original.cols;
    st.targetH = original.rows;

    cv::namedWindow(kWindow, cv::WINDOW_AUTOSIZE);
    // Pass nullptr for the value pointer (the deprecated/unsafe form); the
    // onChange callbacks keep st.targetW/H in sync instead.
    cv::createTrackbar("Width", kWindow, nullptr, original.cols, onWidth, &st);
    cv::createTrackbar("Height", kWindow, nullptr, original.rows, onHeight, &st);
    cv::setTrackbarMin("Width", kWindow, 1);
    cv::setTrackbarMin("Height", kWindow, 1);
    cv::setTrackbarPos("Width", kWindow, original.cols);
    cv::setTrackbarPos("Height", kWindow, original.rows);

    std::cout << "Interactive mode. Keys:\n"
                 "  1/2/3  DP / greedy / graph cut\n"
                 "  Space  carve from original to slider sizes (animated)\n"
                 "  r      reset to original\n"
                 "  s      save current result\n"
                 "  q/Esc  quit\n";

    show(st.result, st);

    for (;;) {
        int key = cv::waitKey(30);
        if (key < 0) continue;
        key &= 0xFF;

        if (key == 'q' || key == 27) break;  // 27 == Esc
        switch (key) {
            case '1': st.algo = "dp";       show(st.result, st, "algo: DP"); break;
            case '2': st.algo = "greedy";   show(st.result, st, "algo: greedy"); break;
            case '3': st.algo = "graphcut"; show(st.result, st, "algo: graph cut"); break;
            case ' ': runCarve(st); break;
            case 'r':
                st.result = st.original.clone();
                cv::setTrackbarPos("Width", kWindow, st.original.cols);
                cv::setTrackbarPos("Height", kWindow, st.original.rows);
                show(st.result, st, "reset");
                break;
            case 's': save(st); break;
            default: break;
        }
    }
    cv::destroyWindow(kWindow);
}
