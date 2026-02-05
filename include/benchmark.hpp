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
    static void floatingPointTask(int iterations);
    static void matrixMultiplyTask(int iterations);
    static void dryRun(int iterations);
    static std::unordered_map<std::string, std::vector<double>> runMultithreadedBenchmark(int numThreads, int iterationsPerThread);
    static std::unordered_map<std::string, std::vector<double>> runSingleThreadedBenchmark(int iterations);

    //Memory Benchmarks
    static void runMemoryBandwidthTest(size_t size);

    //Score calculator
    static double getScore(double intake, int iterations);
};
