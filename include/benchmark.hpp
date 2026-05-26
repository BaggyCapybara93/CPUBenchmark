#pragma once
#include "scorer.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <functional>
#include <numeric>
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

    //Score calculator
    static double getScore(double intake, int iterations);

    //Time
    template <typename Function, typename... Args>
    static double time(Function&& func, Args&& ... args){
        auto start = std::chrono::high_resolution_clock::now();
        std::invoke(std::forward<Function>(func), std::forward<Args>(args)...);
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(end - start).count();
    }

    //Runner
    template <typename Function, typename... Args>
    static double runBenchmark(Function&& func, Args&&... args, int numRuns){
        if (numRuns <= 0) {
            throw std::invalid_argument("Number of runs must be positive.");
        }

        //Warmup
        const int warmupRuns = 5;
        for (int i = 0; i < warmupRuns; ++i) {
            Benchmark::execute(func, args...);
        }

        // Measurement
        std::vector<double> runTimes;
        
        for (int i = 0; i < numRuns; ++i) {
            double duration = Benchmark::time(func, args...); 
            runTimes.push_back(duration);
        }

        // Calculation
        double totalTime = std::accumulate(runTimes.begin(), runTimes.end(), 0.0);
        
        return totalTime / numRuns; // Returns the average 
    }

    //Non-timing Functon Execution
    template <typename Function, typename... Args>
    static void execute(Function&& func, Args&&... args){
        std::invoke(std::forward<Function>(func), std::forward<Args>(args)...);
    }   
};
