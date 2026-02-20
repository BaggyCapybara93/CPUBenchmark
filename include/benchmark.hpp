#pragma once

#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>

class Benchmark {
public:
    static void integerArithmeticBenchmark(int iterations);
    static void floatingPointBenchmark(int iterations);
    static void matrixMultiplyBenchmark(int iterations);
    static void branchPredictionBenchmark(int iterations);
    static void nBodyBenchmark(int nBodies, int steps);
    static void dryRun(int iterations);
    static std::unordered_map<std::string, std::vector<double>> runMultithreadedBenchmark(int numThreads, int iterationsPerThread, const int intensityMultiplier = 50, const int matrixMultiplySize = 128);
    static std::unordered_map<std::string, std::vector<double>> runSingleThreadedBenchmark(int iterations, const int intensityMultiplier = 50, const int matrixMultiplySize = 128);

    //Memory Benchmarks
    static void runMemoryBandwidthTest(size_t size);

    //Score calculator
    static double getScore(double intake, int iterations);

    //Time
    template <typename Function, typename... Args>
    static double time(Function&& function, Args&& ... args);
};
