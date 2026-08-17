#include "scorer.hpp"
#include "iostream"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <map>
#include <utility>

void scorer::normalize_weights() {
    try{
        double sum = 0.0;
        for (auto& w : weights_) sum += w.second;
        for (auto& w : weights_) w.second /= sum;
    } catch (const std::exception& e) {
        std::cerr << "Error normalizing weights: " << e.what() << "\n";
    }
}

// Add a raw score (elapsed time)
void scorer::add_score(const std::string& benchmark_name, const double& raw_score, const double& time) {
    try{
        (void)raw_score; //Unused parameter, will be used in future scorer updates
        double normalized = process_score(benchmark_name, time);

        std::lock_guard<std::mutex> lock(scores_mutex_);
        scores_.emplace_back(benchmark_name, normalized, time);
    } catch (const std::exception& e) {
        std::cerr << "Error adding score: " << e.what() << "\n";
    }
}

// Add multiple scores at once (for batch processing)
void scorer::add_scores(const std::vector<std::pair<std::string, double>>& benchmark_times) {
    try {
        std::lock_guard<std::mutex> lock(scores_mutex_);
        
        // Group times by benchmark name
        std::map<std::string, std::vector<double>> times_map;
        for (const auto& pt : benchmark_times) {
            times_map[pt.first].push_back(pt.second);
        }
        
        // Add scores and store raw times for statistics
        for (auto& [name, times] : times_map) {
            double normalized = process_score(name, times[0]);
            scores_.emplace_back(name, normalized, times[0]);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error adding scores: " << e.what() << "\n";
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
        if (!scores_.empty() && scores_.front().benchmark_name_ == benchmark_name) {
            scores_.front().min_time_ = min_t;
            scores_.front().max_time_ = max_t;
            scores_.front().std_dev_ = std_dev;
            scores_.front().p95_ = p95;
            scores_.front().p99_ = p99;
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
    double baseline = baseline_times_[benchmark_name];
    double normalized = baseline / elapsed_time;
    return normalized * 2500.0; 
}

// Load custom weights
void scorer::load_weights(std::shared_ptr<std::unordered_map<std::string, double>> new_weights) {
    weights_ = *new_weights;
    normalize_weights();
}

// Load custom baseline times
void scorer::load_baseline_times(std::shared_ptr<std::unordered_map<std::string, double>> new_baseline_times) {
    baseline_times_ = *new_baseline_times;
}

// Compute final weighted score
double scorer::compute_final_score() {
    try{
        std::lock_guard<std::mutex> lock(scores_mutex_);
        
        std::unordered_map<std::string, double> averages;

        // Compute average score per benchmark
        for (auto& s : scores_) {
            if (s.benchmark_name_ == "Combined")
            continue;
            averages[s.benchmark_name_] += s.score_;
        }
        for (auto& a : averages) {
            int count = 0;
            for (auto& s : scores_)
                if (s.benchmark_name_ == a.first) count++;

            a.second /= count;
        }

        // Weighted sum
        double final_score = 0.0;
        for (auto& a : averages) {
            final_score += a.second * weights_[a.first];
        }

        return final_score;
    } catch (const std::exception& e) {
        std::cerr << "Error computing final score: " << e.what() << "\n";
        return 0.0;
    }
}

const std::vector<score>& scorer::get_scores(){
    std::lock_guard<std::mutex> lock(scores_mutex_);
    return scores_;
}