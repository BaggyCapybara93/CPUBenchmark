#pragma once
#include "scorer.hpp"
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
    static void sortingBenchmark(int size);
    static void dryRun(int iterations);
    static std::vector<Score> runMultithreadedBenchmark(const int numThreads, int iterationsPerThread, const int intensityMultiplier = 50, const int matrixMultiplySize = 128, const int numRuns = 10);
    static std::vector<Score> runSingleThreadedBenchmark(int iterations, const int intensityMultiplier = 50, const int matrixMultiplySize = 128, const int numRuns = 10);

    //Score calculator
    static double getScore(double intake, int iterations);

    //Time
    template <typename Function, typename... Args>
    static double time(Function&& function, Args&& ... args);

    //Runner
    template <typename Function, typename... Args>
    static double runBenchmark(Function func, Args... args, int numRuns);

    //Non-timing Functon Execution
    template <typename Function, typename... Args>
    static void execute(Function&& func, Args&&... args);
};
