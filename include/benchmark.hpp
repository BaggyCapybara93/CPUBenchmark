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
#include <array>
#include <cstdint>

enum class arithmetic_operation { add, multiply, divide, fma };
enum class branch_pattern { predictable, unpredictable };

class benchmark {
public:
    static constexpr std::size_t arithmetic_lanes = 8;

    struct integer_kernel_workload {
        integer_kernel_workload(int iterations, arithmetic_operation operation);
        std::uint32_t run() const;

    private:
        int iterations_;
        arithmetic_operation operation_;
        std::array<std::uint32_t, 256> operands_{};
        std::array<std::uint32_t, 256> numerators_{};
    };

    struct floating_kernel_workload {
        floating_kernel_workload(int iterations, arithmetic_operation operation);
        double run() const;

    private:
        int iterations_;
        arithmetic_operation operation_;
        std::array<double, 256> operands_{};
        std::array<double, 256> addends_{};
    };

    struct branch_pattern_workload {
        branch_pattern_workload(int iterations, branch_pattern pattern);
        std::uint64_t run() const;
        std::uint64_t taken_count() const { return taken_count_; }

    private:
        std::vector<std::uint8_t> pattern_;
        std::uint64_t taken_count_ = 0;
    };

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
        double run(std::size_t elements_to_process);

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

    // One randomly ordered cache line per node; construction is outside timing.
    struct cache_sweep_workload {
        explicit cache_sweep_workload(std::size_t working_set_bytes);
        void run(std::size_t hops) const;
        std::size_t working_set_bytes() const { return nodes_.size() * sizeof(node); }

    private:
        struct alignas(64) node {
            std::uint32_t next = 0;
        };
        std::vector<node> nodes_;
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
    static std::vector<double> run_benchmark(Function&& func, int num_runs, Setup&& setup){
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
        run_times.reserve(static_cast<std::size_t>(num_runs));
        
        for (int i = 0; i < num_runs; ++i) {
            std::invoke(setup);
            double duration = benchmark::time(func);
            run_times.push_back(duration);
        }

        return run_times;
    }

    template <typename Function>
    static std::vector<double> run_benchmark(Function&& func, int num_runs){
        return run_benchmark(std::forward<Function>(func), num_runs, [] {});
    }

    //Non-timing Functon Execution
    template <typename Function, typename... Args>
    static void execute(Function&& func, Args&&... args){
        std::invoke(std::forward<Function>(func), std::forward<Args>(args)...);
    }   
};
