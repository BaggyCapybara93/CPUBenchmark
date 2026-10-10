#pragma once
#include "scorer.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>
#include <unordered_map>

class benchmark {
public:
    struct sorting_workload {
        explicit sorting_workload(int size);
        void reset();
        void run();

    private:
        std::vector<int> input_;
        std::vector<int> working_;
    };

    struct memory_bandwidth_workload {
        explicit memory_bandwidth_workload(std::size_t size);
        void run();

    private:
        std::vector<double> a_;
        std::vector<double> b_;
        std::vector<double> c_;
    };

    struct pointer_chase_workload {
        explicit pointer_chase_workload(std::size_t size);
        void run() const;

    private:
        std::vector<std::size_t> next_;
    };

    static void integer_arithmetic_benchmark(int iterations);
    static void floating_point_benchmark(int iterations);
    static void matrix_multiply_benchmark(int iterations);
    static void branch_prediction_benchmark(int iterations);
    static void n_body_benchmark(int n_bodies, int steps);
    static void sorting_benchmark(int size);
    static void memory_bandwidth_benchmark(size_t size);
    static void pointer_chase_benchmark(size_t size);
    static void cache_latency_benchmark(size_t size, size_t stride);

    //Score calculator
    static double get_score(double intake, int iterations);

    //Time
    template <typename Function, typename... Args>
    static double time(Function&& func, Args&& ... args){
        auto start = std::chrono::steady_clock::now();
        std::invoke(std::forward<Function>(func), std::forward<Args>(args)...);
        auto end = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(end - start).count();
    }

    // Run setup before every warmup and sample, outside the timed interval.
    template <typename Function, typename Setup>
    static double run_benchmark(Function&& func, int num_runs, Setup&& setup){
        if (num_runs <= 0) {
            throw std::invalid_argument("Number of runs must be positive.");
        }

        //Warmup
        const int warmup_runs = 5;
        for (int i = 0; i < warmup_runs; ++i) {
            std::invoke(setup);
            benchmark::execute(func);
        }

        // Measurement
        std::vector<double> run_times;
        
        for (int i = 0; i < num_runs; ++i) {
            std::invoke(setup);
            double duration = benchmark::time(func);
            run_times.push_back(duration);
        }

        // Calculation
        double total_time = std::accumulate(run_times.begin(), run_times.end(), 0.0);
        
        return total_time / num_runs; // Returns the average 
    }

    template <typename Function>
    static double run_benchmark(Function&& func, int num_runs){
        return run_benchmark(std::forward<Function>(func), num_runs, [] {});
    }

    //Non-timing Functon Execution
    template <typename Function, typename... Args>
    static void execute(Function&& func, Args&&... args){
        std::invoke(std::forward<Function>(func), std::forward<Args>(args)...);
    }   
};
