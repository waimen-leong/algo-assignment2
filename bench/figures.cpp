// figures.cpp — makes the image figures for section 5(c).
//
//   seams_<algo>.png      the original image with that algorithm's first N seams in red
//   seams_all.png         those overlays side by side, one per algorithm
//   quality.png           same target size: scaled | cropped | each algorithm, side by side
//   carved_<algo>.png, scaled.png, cropped.png   the individual results
//
// Run it on 2 or 3 images (one with a clear subject), e.g.
//   ./build/bench/seam_figures --image beach.jpg --width 300 --outdir figs/beach
//
// Graph cut is slow on big images. --resize-width shrinks the input first
// (e.g. --resize-width 200), and --skip-graphcut leaves it out entirely.
#include "seam_api.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Draw the first nSeams seams on the ORIGINAL image. Each seam is found on the
// already-shrunk image, so we keep a map from current pixel -> original column
// and shrink it alongside the image.
static cv::Mat3b overlaySeams(const cv::Mat3b& original, ISeamFinder& f, int nSeams) {
    cv::Mat3b cur = original.clone();
    cv::Mat1i origCol(original.rows, original.cols);
    for (int r = 0; r < original.rows; ++r)
        for (int c = 0; c < original.cols; ++c) origCol(r, c) = c;

    cv::Mat3b out = original.clone();
    for (int k = 0; k < nSeams && cur.cols > 1; ++k) {
        Seam s = f.find(computeEnergy(cur));
        cv::Mat1i nextCol(cur.rows, cur.cols - 1);
        for (int r = 0; r < cur.rows; ++r) {
            out(r, origCol(r, s[r])) = cv::Vec3b(0, 0, 255);  // red (OpenCV is BGR)
            for (int c = 0, j = 0; c < cur.cols; ++c)
                if (c != s[r]) nextCol(r, j++) = origCol(r, c);
        }
        origCol = nextCol;
        cur = removeVerticalSeam(cur, s);
    }
    return out;
}

// Puts a white label strip above the image.
static cv::Mat3b labelled(const cv::Mat3b& img, const std::string& text) {
    const int strip = 24;
    cv::Mat3b out(img.rows + strip, img.cols, cv::Vec3b(255, 255, 255));
    img.copyTo(out(cv::Rect(0, strip, img.cols, img.rows)));
    cv::putText(out, text, cv::Point(4, 17), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1,
                cv::LINE_AA);
    return out;
}

// Joins images left to right with a small white gap; pads shorter ones at the bottom.
static cv::Mat3b sideBySide(const std::vector<cv::Mat3b>& imgs) {
    int h = 0, w = 0;
    const int gap = 8;
    for (auto& m : imgs) { h = std::max(h, m.rows); w += m.cols + gap; }
    cv::Mat3b out(h, w - gap, cv::Vec3b(255, 255, 255));
    int x = 0;
    for (auto& m : imgs) {
        m.copyTo(out(cv::Rect(x, 0, m.cols, m.rows)));
        x += m.cols + gap;
    }
    return out;
}

static cv::Mat3b centreCrop(const cv::Mat3b& img, int w, int h) {
    return img(cv::Rect((img.cols - w) / 2, (img.rows - h) / 2, w, h)).clone();
}

int main(int argc, char** argv) {
    std::string imagePath, outdir = "figures";
    int targetW = -1, targetH = -1, nSeams = 20, resizeWidth = -1;
    bool skipGraphCut = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (a == "--image") imagePath = next();
        else if (a == "--width") targetW = std::stoi(next());
        else if (a == "--height") targetH = std::stoi(next());
        else if (a == "--seams") nSeams = std::stoi(next());
        else if (a == "--resize-width") resizeWidth = std::stoi(next());
        else if (a == "--outdir") outdir = next();
        else if (a == "--skip-graphcut") skipGraphCut = true;
        else {
            std::cout << "Usage: seam_figures --image path [--width W] [--height H] [--seams 20]\n"
                         "                    [--resize-width W] [--skip-graphcut] [--outdir dir]\n";
            return 1;
        }
    }
    if (imagePath.empty()) { std::cerr << "--image is required\n"; return 1; }

    cv::Mat3b img = cv::imread(imagePath, cv::IMREAD_COLOR);
    if (img.empty()) { std::cerr << "Could not read " << imagePath << "\n"; return 1; }
    if (resizeWidth > 0 && resizeWidth < img.cols) {
        int h = static_cast<int>(img.rows * (double)resizeWidth / img.cols + 0.5);
        cv::resize(img, img, cv::Size(resizeWidth, h), 0, 0, cv::INTER_AREA);
    }
    if (targetW <= 0) targetW = img.cols * 3 / 4;  // default: 25% narrower
    if (targetH <= 0) targetH = img.rows;
    if (targetW > img.cols || targetH > img.rows) {
        std::cerr << "Target must not be larger than the image (" << img.cols << "x" << img.rows << ")\n";
        return 1;
    }

    fs::create_directories(outdir);
    auto save = [&](const std::string& name, const cv::Mat3b& m) {
        std::string p = (fs::path(outdir) / name).string();
        cv::imwrite(p, m);
        std::cout << "wrote " << p << "\n";
    };

    std::vector<std::unique_ptr<ISeamFinder>> finders;
#ifdef SEAM_HAVE_DP
    finders.push_back(std::make_unique<DPSeamFinder>());
#endif
#ifdef SEAM_HAVE_GREEDY
    finders.push_back(std::make_unique<GreedySeamFinder>());
#endif
#ifdef SEAM_HAVE_GRAPHCUT
    if (!skipGraphCut) finders.push_back(std::make_unique<GraphCutSeamFinder>());
#endif

    // Reference results: plain scaling and centre cropping.
    cv::Mat3b scaled, cropped = centreCrop(img, targetW, targetH);
    cv::resize(img, scaled, cv::Size(targetW, targetH), 0, 0, cv::INTER_AREA);
    save("original.png", img);
    save("scaled.png", scaled);
    save("cropped.png", cropped);

    std::vector<cv::Mat3b> overlays, quality = {labelled(scaled, "scaled"), labelled(cropped, "cropped")};
    for (auto& f : finders) {
        std::string name = f->name();
        std::cout << "running " << name << "...\n";
        cv::Mat3b ov = overlaySeams(img, *f, nSeams);
        save("seams_" + name + ".png", ov);
        overlays.push_back(labelled(ov, name + ": first " + std::to_string(nSeams) + " seams"));

        cv::Mat3b carved = carve(img, targetW, targetH, *f);
        save("carved_" + name + ".png", carved);
        quality.push_back(labelled(carved, name));
    }
    if (!overlays.empty()) save("seams_all.png", sideBySide(overlays));
    save("quality.png", sideBySide(quality));
    return 0;
}
