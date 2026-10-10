#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <mutex>
#include <utility>

struct score {
    std::string benchmark_name_;
    double score_; 
    double time_; 
    
    // Statistical data
    double min_time_;
    double max_time_;
    double std_dev_;
    double median_time_;
    double p95_;
    double p99_;
    std::vector<double> samples_;

    score(const std::string& name, double s, double t)
        : benchmark_name_(name), score_(s), time_(t),
          min_time_(t), max_time_(t), std_dev_(0.0), median_time_(t),
          p95_(t), p99_(t), samples_(t > 0.0 ? std::vector<double>{t} : std::vector<double>{}) {}

    score(const std::string& name, double s, double t, std::vector<double> samples)
        : benchmark_name_(name), score_(s), time_(t),
          min_time_(t), max_time_(t), std_dev_(0.0), median_time_(t),
          p95_(t), p99_(t), samples_(std::move(samples)) {}
    
    score(const std::string& name, double s, double t, double min_t, double max_t, double std_dev, double p95, double p99)
        : benchmark_name_(name), score_(s), time_(t),
          min_time_(min_t), max_time_(max_t), std_dev_(std_dev), median_time_(t),
          p95_(p95), p99_(p99), samples_(t > 0.0 ? std::vector<double>{t} : std::vector<double>{}) {}
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
        {"pointer_chase",   0.10},
        {"cache_latency",   0.10}
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
        {"cache_latency",   0.020},
        {"combined",         0.150}
    };

    // Normalize weights to 1.0
    void normalize_weights();

public:

    // Add one measured benchmark time.
    void add_score(const std::string& benchmark_name, double elapsed_time);
    double add_score(const std::string& benchmark_name, std::vector<double> run_times);

    // Clear all stored scores
    void clear_scores();

    double process_score(const std::string& benchmark_name, const double& elapsed_time);

    // Load custom weights
    void load_weights(const std::unordered_map<std::string, double>& new_weights);

    // Load custom baseline times
    void load_baseline_times(const std::unordered_map<std::string, double>& new_baseline_times);

    // Compute final weighted score
    double compute_final_score();

    //Get the score for each specific method
    std::vector<score> get_scores() const;
    
    // Add multiple scores at once (for batch processing)
    void add_scores(const std::vector<std::pair<std::string, double>>& benchmark_times);
    
    // Compute statistics for a benchmark
    void compute_statistics(const std::string& benchmark_name);
    
    // Get statistics for a benchmark
    struct benchmark_stats {
        double min_time;
        double max_time;
        double std_dev;
        double median_time;
        double p95;
        double p99;
        int count;
    };
    
    benchmark_stats get_stats(const std::string& benchmark_name);
};
