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

class benchmark {
public:
    static void integer_arithmetic_benchmark(int iterations);
    static void floating_point_benchmark(int iterations);
    static void matrix_multiply_benchmark(int iterations);
    static void branch_prediction_benchmark(int iterations);
    static void n_body_benchmark(int n_bodies, int steps);
    static void sorting_benchmark(int size);
    static void memory_bandwidth_benchmark(size_t size);
    static void pointer_chase_benchmark(size_t size);

    //Score calculator
    static double get_score(double intake, int iterations);

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
    static double run_benchmark(Function&& func, Args&&... args, int num_runs){
        if (num_runs <= 0) {
            throw std::invalid_argument("Number of runs must be positive.");
        }

        //Warmup
        const int warmup_runs = 5;
        for (int i = 0; i < warmup_runs; ++i) {
            benchmark::execute(func, args...);
        }

        // Measurement
        std::vector<double> run_times;
        
        for (int i = 0; i < num_runs; ++i) {
            double duration = benchmark::time(func, args...); 
            run_times.push_back(duration);
        }

        // Calculation
        double total_time = std::accumulate(run_times.begin(), run_times.end(), 0.0);
        
        return total_time / num_runs; // Returns the average 
    }

    //Non-timing Functon Execution
    template <typename Function, typename... Args>
    static void execute(Function&& func, Args&&... args){
        std::invoke(std::forward<Function>(func), std::forward<Args>(args)...);
    }   
};
