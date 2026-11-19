// the purpose of this script is to benchmark the multithreaded vs single threaded energy computation

#include "core/energy_mt.h"

#include <opencv2/core.hpp>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <thread>
#include <fstream>
#include <vector>

static double TimeMs(const cv::Mat1b& gray, int threads, int runs)
{
    ComputeEnergyMT(gray, threads); //warm up

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < runs; i++)
    {
        ComputeEnergyMT(gray, threads);
    }

    auto end = std::chrono::steady_clock::now();

    double elapsed = std::chrono::duration<double, std::milli>(end - start).count();

    return elapsed / runs;
}

int main()
{
    const int runs = 20;
    const std::vector<cv::Size> sizes = {          // (width, height)
        cv::Size(200, 150),
        cv::Size(1000, 750),
        cv::Size(4000, 3000),
    };
    const std::vector<int> threadCounts = { 1, 2, 4, 8, 16 };  // 1 first: it is the baseline

    std::ofstream csv("energy_bench.csv");
    csv << "width,height,threads,ms,speedup\n";

    std::cout << "Energy benchmark: average of " << runs << " runs per case "
              << "(hardware threads: " << std::thread::hardware_concurrency() << ")\n\n";
    std::cout << std::fixed << std::setprecision(3);

    cv::RNG rng(12345);                            // fixed seed: same images every run

    for (const cv::Size& size : sizes)
    {
        cv::Mat1b gray(size);                      // cv::Size -> rows = height, cols = width
        rng.fill(gray, cv::RNG::UNIFORM, 0, 256);

        std::cout << size.width << "x" << size.height << std::endl;

        double t1 = 0.0;
        for (int n : threadCounts)
        {
            double ms = TimeMs(gray, n, runs);
            if (n == 1) t1 = ms;
            double speedup = t1 / ms;

            std::cout << "  threads=" << std::setw(2) << n
                      << "  " << std::setw(10) << ms << " ms"
                      << "  speedup " << std::setw(6) << speedup << "x" << std::endl;

            csv << size.width << "," << size.height << "," << n << ","
                << ms << "," << speedup << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "Wrote energy_bench.csv\n";
    return 0;
}