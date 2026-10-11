#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <mutex>
#include <utility>
#include <cstdint>
#include <optional>

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
    std::string work_unit_;
    std::uint64_t total_work_ = 0;
    std::uint64_t estimated_bytes_per_work_ = 0;

    std::optional<double> work_per_second() const {
        if (total_work_ == 0 || work_unit_.empty() || !std::isfinite(time_) || time_ <= 0.0) {
            return std::nullopt;
        }
        const double rate = static_cast<double>(total_work_) / time_;
        return std::isfinite(rate) ? std::optional<double>(rate) : std::nullopt;
    }

    std::optional<double> estimated_gb_per_second() const {
        const auto rate = work_per_second();
        if (!rate || estimated_bytes_per_work_ == 0) return std::nullopt;
        const double gb_per_second = *rate * static_cast<double>(estimated_bytes_per_work_) / 1e9;
        return std::isfinite(gb_per_second)
            ? std::optional<double>(gb_per_second) : std::nullopt;
    }

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
    double add_score(const std::string& benchmark_name, std::vector<double> run_times,
                     std::string work_unit = {}, std::uint64_t total_work = 0,
                     std::uint64_t estimated_bytes_per_work = 0);

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
