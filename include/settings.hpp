#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <boost/program_options.hpp>
#include <thread>

enum class mode{
    single_threaded,
    multi_threaded,
    both,
    invalid,
    none,
};

struct benchmark_profile{
    std::vector<std::string> benchmarks;
    int iterations = 999999;
    int repeat_count = 1;
    int intensity_multiplier = 50;
    int matrix_multiply_size = 128;
    int num_runs = 5;
    int cache_sweep_min_kib = 4;
    int cache_sweep_max_kib = 32768;
    int cache_sweep_hops = 262144;
    std::string name = "default";
};

struct Settings{
    mode mode = mode::invalid;
    bool save_results = true;
    std::string save_method = "txt"; //Make this into an enum when more methods are added
    int thread_count = static_cast<int>(std::thread::hardware_concurrency());
    int iterations = 999999;
    int repeat_count = 1;
    int matrix_multiply_size = 128;
    int intensity_multiplier = 50;
    int num_runs = 5;
    int cache_sweep_min_kib = 4;
    int cache_sweep_max_kib = 32768;
    int cache_sweep_hops = 262144;

    std::unordered_map<std::string, double> weights = {
            {"floating_point", 0.25},
            {"branch", 0.10},
            {"integer", 0.20},
            {"sorting", 0.20},
            {"matrix_multiply", 0.20},
            {"n_body", 0.15},
            {"memory_bandwidth", 0.10},
            {"pointer_chase", 0.10},
            {"cache_latency", 0.10}
        };

    std::unordered_map<std::string, double> baseline_times = {
            {"floating_point", 0.020},
            {"branch", 0.020},
            {"integer", 0.015},
            {"sorting", 0.050},
            {"matrix_multiply", 0.040},
            {"n_body", 0.030},
            {"memory_bandwidth", 0.025},
            {"pointer_chase", 0.020},
            {"cache_latency", 0.020},
            {"combined", 0.150}
    };

    std::unordered_set<std::string> known_benchmarks = {
        "integer_arithmetic",
        "floating_point",
        "matrix_multiply",
        "branch_prediction",
        "n_body",
        "sorting",
        "memory_bandwidth",
        "pointer_chase",
        "cache_latency",
        "cache_size_sweep",
        "integer_add",
        "integer_multiply",
        "integer_divide",
        "floating_add",
        "floating_multiply",
        "floating_divide"
    };
    benchmark_profile profile;
};
