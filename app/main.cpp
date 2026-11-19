// Kai Yang: the seam-carving application.
//   - CLI parsing and the carve/report flow                (parseArgs, makeFinder)
//   - bonus (a) step-by-step / animated red-seam visualisation (viz.*)
//   - bonus (b) interactive slider + algorithm-switch window    (ui.*)
#include "app/args.h"
#include "app/ui.h"
#include "app/viz.h"
#include "core/carver.h"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>

int main(int argc, char** argv) {
    Args args;
    try {
        args = parseArgs(argc, argv);
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n" << usage() << "\n";
        return 2;
    }
    if (args.showHelp) {
        std::cout << usage() << "\n";
        return 0;
    }
    if (args.in.empty()) {
        std::cerr << "error: --in is required\n" << usage() << "\n";
        return 2;
    }

    cv::Mat3b img = cv::imread(args.in, cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "error: cannot read image '" << args.in << "'\n";
        return 1;
    }

    // Interactive mode ignores --width/--height/--out and drives everything
    // from the window instead.
    if (args.interactive) {
        runInteractive(img);
        return 0;
    }

    // Fill in the "keep original size" defaults now that we know the image.
    const int targetW = (args.width  == -1) ? img.cols : args.width;
    const int targetH = (args.height == -1) ? img.rows : args.height;

    auto finder = makeFinder(args.algo);
    const std::string requested = args.algo;
    const std::string actual = finder->name();
    if (actual != requested)
        std::cerr << "note: algorithm '" << requested
                  << "' is not available yet, using '" << actual << "'\n";

    // Build the step callback: animate on screen with --step (the window shows
    // each red seam), and/or dump frames with --frames. Either, both, neither.
    const bool animate = args.step;  // show a window while carving
    std::unique_ptr<FrameWriter> writer;
    try {
        if (!args.framesDir.empty())
            writer = std::make_unique<FrameWriter>(args.framesDir, img.cols,
                                                   img.rows);
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }

    if (animate)
        cv::namedWindow("Seam carving", cv::WINDOW_AUTOSIZE);

    StepCallback cb;
    if (animate || writer) {
        cb = [&](const cv::Mat3b& im, const Seam& s, bool vertical) {
            if (animate) {
                cv::imshow("Seam carving", drawSeam(im, s, vertical));
                cv::waitKey(args.step ? 0 : 15);  // --step waits for a key
            }
            if (writer) writer->write(im, s, vertical);
        };
    }

    try {
        auto t0 = std::chrono::steady_clock::now();
        cv::Mat3b result = carve(img, targetW, targetH, *finder, cb);
        double ms = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - t0).count();

        if (!cv::imwrite(args.out, result)) {
            std::cerr << "error: cannot write '" << args.out << "'\n";
            return 1;
        }

        const int seamsRemoved = (img.cols - result.cols) + (img.rows - result.rows);
        std::cout << "algorithm:    " << actual << "\n"
                  << "input:        " << img.cols << "x" << img.rows << "\n"
                  << "output:       " << result.cols << "x" << result.rows << "\n"
                  << "seams removed: " << seamsRemoved << "\n"
                  << "time:         " << ms << " ms\n"
                  << "saved to:     " << args.out << "\n";
        if (writer)
            std::cout << "frames:       " << writer->count() << " in "
                      << args.framesDir << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }

    if (animate) {
        std::cout << "press any key in the window to close\n";
        cv::waitKey(0);
        cv::destroyAllWindows();
    }
    return 0;
}
