#include "core/carver.h"
#include "core/energy.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <string>

cv::Mat3b removeVerticalSeam(const cv::Mat3b& img, const Seam& s) {
    if (img.cols < 2)
        throw std::invalid_argument("removeVerticalSeam: image is too narrow");
    if (static_cast<int>(s.size()) != img.rows)
        throw std::invalid_argument("removeVerticalSeam: seam length != rows");

    cv::Mat3b out(img.rows, img.cols - 1);
    for (int r = 0; r < img.rows; ++r) {
        const int c = s[r];
        if (c < 0 || c >= img.cols)
            throw std::invalid_argument("removeVerticalSeam: column out of range");

        const cv::Vec3b* src = img.ptr<cv::Vec3b>(r);
        cv::Vec3b* dst = out.ptr<cv::Vec3b>(r);
        // Row r of the output = [0, c) followed by (c, cols). Pixel c is skipped.
        std::copy(src, src + c, dst);
        std::copy(src + c + 1, src + img.cols, dst + c);
    }
    return out;
}

bool isValidSeam(const Seam& s, int rows, int cols) {
    if (static_cast<int>(s.size()) != rows) return false;
    for (int r = 0; r < rows; ++r) {
        if (s[r] < 0 || s[r] >= cols) return false;
        if (r > 0 && std::abs(s[r] - s[r - 1]) > 1) return false;
    }
    return true;
}

float seamCost(const cv::Mat1f& e, const Seam& s) {
    double sum = 0.0;  // accumulate in double to limit rounding error
    for (int r = 0; r < static_cast<int>(s.size()); ++r)
        sum += e(r, s[r]);
    return static_cast<float>(sum);
}

namespace {
void requireValid(const Seam& s, int rows, int cols, const ISeamFinder& finder,
                  const char* direction) {
    if (!isValidSeam(s, rows, cols))
        throw std::runtime_error(std::string("carve: finder '") + finder.name() +
                                 "' returned an invalid " + direction + " seam");
}
}  // namespace

cv::Mat3b carve(const cv::Mat3b& img, int targetW, int targetH,
                ISeamFinder& finder, StepCallback cb) {
    if (img.empty())
        throw std::invalid_argument("carve: empty image");
    if (targetW < 1 || targetH < 1 || targetW > img.cols || targetH > img.rows)
        throw std::invalid_argument("carve: target must be in [1, current size] (shrink only)");

    cv::Mat3b out = img.clone();

    // ---- Phase 1: vertical seams reduce the width -------------------------
    while (out.cols > targetW) {
        cv::Mat1f e = computeEnergy(out);  // recomputed every iteration (brief 1c)
        Seam seam = finder.find(e);
        requireValid(seam, out.rows, out.cols, finder, "vertical");
        if (cb) cb(out, seam, true);
        out = removeVerticalSeam(out, seam);
    }

    // ---- Phase 2: horizontal seams reduce the height ----------------------
    // A horizontal seam in `out` is a vertical seam in transpose(out).
    // So transpose, remove vertical seams until the transposed WIDTH equals
    // targetH, then transpose back. Finders never see a horizontal seam.
    if (out.rows > targetH) {
        cv::Mat3b t;
        cv::transpose(out, t);  // t is (out.cols rows) x (out.rows cols)

        while (t.cols > targetH) {
            cv::Mat1f e = computeEnergy(t);
            Seam seam = finder.find(e);
            requireValid(seam, t.rows, t.cols, finder, "horizontal");
            if (cb) {
                // Row r of t is column r of the display image, so
                // seam[r] = c in t means "in display column r, remove row c",
                // i.e. seam[col] = row: exactly the vertical == false convention.
                cv::Mat3b display;
                cv::transpose(t, display);
                cb(display, seam, false);
            }
            t = removeVerticalSeam(t, seam);
        }
        cv::transpose(t, out);
    }

    return out;
}
