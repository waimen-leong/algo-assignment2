#include "core/energy.h"
#include <opencv2/imgproc.hpp>

cv::Mat1f computeEnergy(const cv::Mat3b& img) {
    // 1. Colour -> one intensity channel.
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

    // 2. Horizontal and vertical gradients. CV_32F output keeps the sign
    //    and avoids overflow (Sobel values can exceed 255).
    cv::Mat gx, gy;
    cv::Sobel(gray, gx, CV_32F, 1, 0, 3);
    cv::Sobel(gray, gy, CV_32F, 0, 1, 3);

    // 3. L1 gradient magnitude. Symmetric in x and y, so the energy of a
    //    transposed image is the transpose of the energy - this is what
    //    makes the transpose trick for horizontal seams valid.
    cv::Mat1f energy = cv::abs(gx) + cv::abs(gy);
    return energy;
}
