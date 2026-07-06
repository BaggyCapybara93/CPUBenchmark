#pragma once

#include <string>
#include <unordered_map>
#include <boost/program_options.hpp>
#include <thread>

enum class mode{
    single_threaded,
    multi_threaded,
    both,
    invalid,
    none,
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

    std::unordered_map<std::string, double> weights = {
            {"floating_point", 0.25},
            {"branch", 0.10},
            {"integer", 0.20},
            {"sorting", 0.20},
            {"matrix_multiply", 0.20},
            {"n_body", 0.15},
            {"memory_bandwidth", 0.10},
            {"pointer_chase", 0.10}
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
            {"combined", 0.150}
        };
};