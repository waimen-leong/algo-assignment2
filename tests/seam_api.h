// seam_api.h — the ONE place the tests and benchmark include the team's code.
//
// CMake (SeamDetect.cmake) finds each header and passes its path in as a macro,
// so this usually needs no editing. If a function lives in a different header
// than expected (e.g. isValidSeam is in "seam_utils.h"), add an #include here.
//
// Functions the tests expect, straight from the team plan:
//   cv::Mat1f computeEnergy(const cv::Mat3b& img);
//   cv::Mat3b removeVerticalSeam(const cv::Mat3b& img, const Seam& s);
//   cv::Mat3b carve(const cv::Mat3b& img, int targetW, int targetH,
//                   ISeamFinder& f, StepCallback cb = {});
//   bool      isValidSeam(const Seam& s, int rows, int cols);
//   float     seamCost(const cv::Mat1f& energy, const Seam& s);
#pragma once

#include <opencv2/core.hpp>

#ifdef SEAM_TYPES_HEADER
#include SEAM_TYPES_HEADER
#endif
#ifdef SEAM_ENERGY_HEADER
#include SEAM_ENERGY_HEADER
#endif
#ifdef SEAM_CARVER_HEADER
#include SEAM_CARVER_HEADER
#endif

#ifdef SEAM_HAVE_DP
#include SEAM_DP_HEADER
#endif
#ifdef SEAM_HAVE_DP_LOWMEM
#include SEAM_DP_LOWMEM_HEADER
#endif
#ifdef SEAM_HAVE_GREEDY
#include SEAM_GREEDY_HEADER
#endif
#ifdef SEAM_HAVE_GRAPHCUT
#include SEAM_GRAPHCUT_HEADER
#endif
