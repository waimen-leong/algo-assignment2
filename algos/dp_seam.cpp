#include "algos/dp_seam.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

cv::Mat1f buildCostTable(const cv::Mat1f& e)
{
    const int rows = e.rows, cols = e.cols;
    cv::Mat1f M(rows, cols);
    if (rows == 0 || cols == 0) return M;

    e.row(0).copyTo(M.row(0));  // base case: a one-pixel seam

    for (int i = 1; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            float best = M(i - 1, j);                        // straight up
            if (j > 0)
                best = std::min(best, M(i - 1, j - 1));  // up-left
            if (j < cols - 1)
                best = std::min(best, M(i - 1, j + 1));  // up-right
            M(i, j) = e(i, j) + best;
        }
    }
    return M;
}

Seam backtrack(const cv::Mat1f& M)
{
    const int rows = M.rows, cols = M.cols;
    if (rows == 0 || cols == 0)
        return {};

    Seam seam(rows);

    // TIE RULE: strict '<' while scanning left to right, so on equal values
    // the leftmost column is kept. Same rule at every step, and the same
    // rule as the low-memory version, so both return identical seams.
    int j = 0;
    for (int c = 1; c < cols; ++c)
        if (M(rows - 1, c) < M(rows - 1, j))
            j = c;
    seam[rows - 1] = j;

    for (int i = rows - 1; i >= 1; --i)
    {
        int best = j;
        float bestVal = M(i - 1, j);
        if (j > 0 && M(i - 1, j - 1) <= bestVal)
        {  // left wins ties
            best = j - 1;
            bestVal = M(i - 1, j - 1);
        }
        if (j < cols - 1 && M(i - 1, j + 1) < bestVal)
        {
            best = j + 1;
        }
        j = best;
        seam[i - 1] = j;
    }
    return seam;
}

Seam DPSeamFinderLowMem::find(const cv::Mat1f& e)
{
    const int rows = e.rows, cols = e.cols;
    if (rows == 0 || cols == 0)
        return {};

    std::vector<float> prev(cols), cur(cols);
    cv::Mat_<int8_t> from(rows, cols, int8_t(0));  // -1, 0, +1: parent offset

    for (int j = 0; j < cols; ++j)
        prev[j] = e(0, j);

    for (int i = 1; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            // Same left-to-right order and tie rule as backtrack().
            int bestD = 0;
            float bestVal = prev[j];
            if (j > 0 && prev[j - 1] <= bestVal)
            {
                bestD = -1;
                bestVal = prev[j - 1];
            }
            if (j < cols - 1 && prev[j + 1] < bestVal)
            {
                bestD = +1;
                bestVal = prev[j + 1];
            }
            cur[j] = e(i, j) + bestVal;
            from(i, j) = static_cast<int8_t>(bestD);
        }
        std::swap(prev, cur);
    }

    Seam seam(rows);
    int j = 0;
    for (int c = 1; c < cols; ++c)
        if (prev[c] < prev[j])
            j = c;
    seam[rows - 1] = j;
    for (int i = rows - 1; i >= 1; --i)
    {
        j += from(i, j);
        seam[i - 1] = j;
    }
    return seam;
}
