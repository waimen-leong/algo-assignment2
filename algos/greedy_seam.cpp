#include "algos/greedy_seam.h"

Seam GreedySeamFinder::find(const cv::Mat1f& energy)
{
    const int rows = energy.rows;
    const int col = energy.cols;

    Seam seam(rows);

    if (!rows || !col) return seam;

    // find smallest energy in first row
    int start = 0;
    for (int i = 1; i < col; i++)
    {
        if (energy(0,i) < energy(0,start))
        {
            start = i;
        }
    }
    seam[0] = start;

    // continue for subseq rows
    for (int i = 1; i < rows; i++)
    {
        int j = seam[i - 1];
        int best = j;

        if (j - 1 >= 0 && energy(i, j - 1) <= energy(i, best))
        {
            best = j - 1;
        }

        if ( j + 1 < col && energy(i, j + 1) < energy(i, best))
        {
            best = j + 1;
        }

        seam[i] = best;
    }

    return seam;
}

const char* GreedySeamFinder::name() const
{
    return "greedy";
}