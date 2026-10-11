#include "scorer.hpp"
#include "timing_statistics.hpp"
#include "iostream"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <map>
#include <utility>
#include <stdexcept>

namespace {
constexpr double score_scale = 2500.0;
}

void scorer::normalize_weights() {
    double sum = 0.0;
    for (const auto& [name, weight] : weights_) {
        if (!std::isfinite(weight) || weight < 0.0) {
            throw std::invalid_argument("Weight for '" + name + "' must be finite and non-negative");
        }
        sum += weight;
    }
    if (!std::isfinite(sum) || sum <= 0.0) {
        throw std::invalid_argument("At least one scoring weight must be positive and finite");
    }
    for (auto& [name, weight] : weights_) {
        (void)name;
        weight /= sum;
    }
}

void scorer::add_score(const std::string& benchmark_name, double elapsed_time) {
    const double normalized = process_score(benchmark_name, elapsed_time);
    std::lock_guard<std::mutex> lock(scores_mutex_);
    scores_.emplace_back(benchmark_name, normalized, elapsed_time);
}

double scorer::add_score(const std::string& benchmark_name, std::vector<double> run_times,
                         std::string work_unit, std::uint64_t total_work) {
    if (run_times.empty()) {
        throw std::invalid_argument("Benchmark must have at least one measured run");
    }
    for (double duration : run_times) {
        if (!std::isfinite(duration) || duration <= 0.0) {
            throw std::invalid_argument("Measured run times must be finite and positive");
        }
    }

    const timing_statistics stats = calculate_timing_statistics(run_times);
    const double normalized = process_score(benchmark_name, stats.mean);
    std::lock_guard<std::mutex> lock(scores_mutex_);
    scores_.emplace_back(benchmark_name, normalized, stats.mean, std::move(run_times));
    score& result = scores_.back();
    result.work_unit_ = std::move(work_unit);
    result.total_work_ = total_work;
    result.min_time_ = stats.min;
    result.max_time_ = stats.max;
    result.std_dev_ = stats.std_dev;
    result.median_time_ = stats.median;
    result.p95_ = stats.p95;
    result.p99_ = stats.p99;
    return stats.mean;
}

void scorer::add_scores(const std::vector<std::pair<std::string, double>>& benchmark_times) {
    for (const auto& [name, elapsed_time] : benchmark_times) {
        add_score(name, elapsed_time);
    }
}

// Compute statistics for a benchmark
void scorer::compute_statistics(const std::string& benchmark_name) {
    try {
        std::lock_guard<std::mutex> lock(scores_mutex_);
        
        std::vector<double> times;
        for (const auto& s : scores_) {
            if (s.benchmark_name_ == benchmark_name) {
                times.insert(times.end(), s.samples_.begin(), s.samples_.end());
            }
        }

        if (times.empty()) {
            return;
        }
        const timing_statistics stats = calculate_timing_statistics(std::move(times));
        
        // Update the first score entry with statistics
        const auto score_it = std::find_if(scores_.begin(), scores_.end(),
            [&benchmark_name](const score& entry) { return entry.benchmark_name_ == benchmark_name; });
        if (score_it != scores_.end()) {
            score_it->min_time_ = stats.min;
            score_it->max_time_ = stats.max;
            score_it->std_dev_ = stats.std_dev;
            score_it->median_time_ = stats.median;
            score_it->p95_ = stats.p95;
            score_it->p99_ = stats.p99;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error computing statistics: " << e.what() << "\n";
    }
}

// Get statistics for a benchmark
scorer::benchmark_stats scorer::get_stats(const std::string& benchmark_name) {
    benchmark_stats stats = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0};
    
    try {
        std::lock_guard<std::mutex> lock(scores_mutex_);
        
        std::vector<double> times;
        for (const auto& s : scores_) {
            if (s.benchmark_name_ == benchmark_name) {
                times.insert(times.end(), s.samples_.begin(), s.samples_.end());
            }
        }
        
        if (times.empty()) {
            return stats;
        }
        const timing_statistics calculated = calculate_timing_statistics(std::move(times));
        stats.min_time = calculated.min;
        stats.max_time = calculated.max;
        stats.std_dev = calculated.std_dev;
        stats.median_time = calculated.median;
        stats.p95 = calculated.p95;
        stats.p99 = calculated.p99;
        stats.count = static_cast<int>(calculated.count);
    } catch (const std::exception& e) {
        std::cerr << "Error getting stats: " << e.what() << "\n";
    }
    
    return stats;
}

// Clear all stored scores
void scorer::clear_scores() {
    std::lock_guard<std::mutex> lock(scores_mutex_);
    scores_.clear();
}

double scorer::process_score(const std::string& benchmark_name, const double& elapsed_time) {
    if (!std::isfinite(elapsed_time) || elapsed_time <= 0.0) {
        throw std::invalid_argument("Elapsed benchmark time must be finite and positive");
    }
    const auto baseline_it = baseline_times_.find(benchmark_name);
    if (baseline_it == baseline_times_.end() || !std::isfinite(baseline_it->second) || baseline_it->second <= 0.0) {
        throw std::invalid_argument("Missing or invalid baseline for benchmark: " + benchmark_name);
    }
    const double baseline = baseline_it->second;
    const double score_value = score_scale * baseline / elapsed_time;
    if (!std::isfinite(score_value)) {
        throw std::overflow_error("Score is not finite for benchmark: " + benchmark_name);
    }
    return score_value;
}

void scorer::load_weights(const std::unordered_map<std::string, double>& new_weights) {
    if (new_weights.empty()) {
        throw std::invalid_argument("Scoring weights cannot be empty");
    }
    weights_ = new_weights;
    normalize_weights();
}

void scorer::load_baseline_times(const std::unordered_map<std::string, double>& new_baseline_times) {
    if (new_baseline_times.empty()) {
        throw std::invalid_argument("Baseline times cannot be empty");
    }
    for (const auto& [name, time] : new_baseline_times) {
        if (!std::isfinite(time) || time <= 0.0) {
            throw std::invalid_argument("Baseline for '" + name + "' must be finite and positive");
        }
    }
    baseline_times_ = new_baseline_times;
}

double scorer::compute_final_score() {
    std::lock_guard<std::mutex> lock(scores_mutex_);

    std::unordered_map<std::string, std::pair<double, std::size_t>> times;
    for (const auto& entry : scores_) {
        times[entry.benchmark_name_].first += entry.time_;
        ++times[entry.benchmark_name_].second;
    }

    double applied_weight = 0.0;
    for (const auto& [name, aggregate] : times) {
        (void)aggregate;
        const auto weight_it = weights_.find(name);
        const auto baseline_it = baseline_times_.find(name);
        if (weight_it == weights_.end() || baseline_it == baseline_times_.end()) {
            throw std::invalid_argument("Missing weight or baseline for benchmark: " + name);
        }
        applied_weight += weight_it->second;
    }
    if (applied_weight <= 0.0) {
        return 0.0;
    }

    double weighted_log_ratio = 0.0;
    for (const auto& [name, aggregate] : times) {
        const double average_time = aggregate.first / static_cast<double>(aggregate.second);
        const double ratio = baseline_times_.at(name) / average_time;
        weighted_log_ratio += (weights_.at(name) / applied_weight) * std::log(ratio);
    }
    const double final_score = score_scale * std::exp(weighted_log_ratio);
    if (!std::isfinite(final_score)) {
        throw std::overflow_error("Combined score is not finite");
    }
    return final_score;
}

std::vector<score> scorer::get_scores() const {
    std::lock_guard<std::mutex> lock(scores_mutex_);
    return scores_;
}
