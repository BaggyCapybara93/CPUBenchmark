#include "scorer.hpp"
#include "iostream"

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