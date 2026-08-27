#include "scorer.hpp"
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

void scorer::add_scores(const std::vector<std::pair<std::string, double>>& benchmark_times) {
    for (const auto& [name, elapsed_time] : benchmark_times) {
        add_score(name, elapsed_time);
    }
}

// Compute statistics for a benchmark
void scorer::compute_statistics(const std::string& benchmark_name) {
    try {
        std::lock_guard<std::mutex> lock(scores_mutex_);
        
        // Collect all times for this benchmark
        std::vector<double> times;
        for (const auto& s : scores_) {
            if (s.benchmark_name_ == benchmark_name) {
                times.push_back(s.time_);
            }
        }
        
        if (times.empty()) {
            return;
        }
        
        // Sort times
        std::sort(times.begin(), times.end());
        
        // Min and max
        double min_t = times.front();
        double max_t = times.back();
        
        // Standard deviation
        double mean = std::accumulate(times.begin(), times.end(), 0.0) / static_cast<double>(times.size());
        double variance = 0.0;
        for (double t : times) {
            variance += (t - mean) * (t - mean);
        }
        double std_dev = std::sqrt(variance / static_cast<double>(times.size()));
        
        // P95 and P99
        double p95_idx = 0.95 * static_cast<double>(times.size() - 1);
        double p99_idx = 0.99 * static_cast<double>(times.size() - 1);
        
        size_t p95_idx_int = static_cast<size_t>(std::floor(p95_idx));
        size_t p99_idx_int = static_cast<size_t>(std::floor(p99_idx));
        
        double p95 = times[p95_idx_int];
        double p99 = times[p99_idx_int];
        
        // Update the first score entry with statistics
        const auto score_it = std::find_if(scores_.begin(), scores_.end(),
            [&benchmark_name](const score& entry) { return entry.benchmark_name_ == benchmark_name; });
        if (score_it != scores_.end()) {
            score_it->min_time_ = min_t;
            score_it->max_time_ = max_t;
            score_it->std_dev_ = std_dev;
            score_it->p95_ = p95;
            score_it->p99_ = p99;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error computing statistics: " << e.what() << "\n";
    }
}

// Get statistics for a benchmark
scorer::benchmark_stats scorer::get_stats(const std::string& benchmark_name) {
    benchmark_stats stats = {0.0, 0.0, 0.0, 0.0, 0.0, 0};
    
    try {
        std::lock_guard<std::mutex> lock(scores_mutex_);
        
        std::vector<double> times;
        for (const auto& s : scores_) {
            if (s.benchmark_name_ == benchmark_name) {
                times.push_back(s.time_);
            }
        }
        
        stats.count = static_cast<int>(times.size());
        
        if (times.empty()) {
            return stats;
        }
        
        std::sort(times.begin(), times.end());
        
        stats.min_time = times.front();
        stats.max_time = times.back();
        
        double mean = std::accumulate(times.begin(), times.end(), 0.0) / static_cast<double>(times.size());
        double variance = 0.0;
        for (double t : times) {
            variance += (t - mean) * (t - mean);
        }
        stats.std_dev = std::sqrt(variance / static_cast<double>(times.size()));
        
        double p95_idx = 0.95 * static_cast<double>(times.size() - 1);
        double p99_idx = 0.99 * static_cast<double>(times.size() - 1);
        
        size_t p95_idx_int = static_cast<size_t>(std::floor(p95_idx));
        size_t p99_idx_int = static_cast<size_t>(std::floor(p99_idx));
        
        stats.p95 = times[p95_idx_int];
        stats.p99 = times[p99_idx_int];
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
