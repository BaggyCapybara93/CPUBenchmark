#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <mutex>

struct score {
    std::string benchmark_name_;
    double score_; 
    double time_; 

    score(const std::string& name, double s, double t)
        : benchmark_name_(name), score_(s), time_(t) {}
};

class scorer {
private:
    std::vector<score> scores_;
    mutable std::mutex scores_mutex_;

    std::unordered_map<std::string, double> weights_ = {
        {"floating_point", 0.25},
        {"branch", 0.10},
        {"integer",       0.20},
        {"sorting",       0.20},
        {"matrix_multiply",0.20},
        {"n_body",         0.15},
        {"memory_bandwidth",0.10},
        {"pointer_chase",   0.10}
    };

    std::unordered_map<std::string, double> baseline_times_ = {
        {"floating_point", 0.020},
        {"branch", 0.020},
        {"integer",       0.015},
        {"sorting",       0.050},
        {"matrix_multiply",0.040},
        {"n_body",         0.030},
        {"memory_bandwidth",0.025},
        {"pointer_chase",   0.020},
        {"combined",         0.150}
    };

    // Normalize weights to 1.0
    void normalize_weights();

public:

    // Add a raw score (elapsed time)
    void add_score(const std::string& benchmark_name, const double& raw_score, const double& time);

    // Clear all stored scores
    void clear_scores();

    double process_score(const std::string& benchmark_name, const double& elapsed_time);

    // Load custom weights
    void load_weights(std::shared_ptr<std::unordered_map<std::string, double>> new_weights);

    // Load custom baseline times
    void load_baseline_times(std::shared_ptr<std::unordered_map<std::string, double>> new_baseline_times);

    // Compute final weighted score
    double compute_final_score();

    //Get the score for each specific method
    const std::vector<score>& get_scores();
};