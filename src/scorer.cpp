#include "scorer.hpp"

void Scorer::normalizeWeights() {
    double sum = 0.0;
    for (auto& w : weights_) sum += w.second;
    for (auto& w : weights_) w.second /= sum;
}

// Add a raw score (elapsed time)
void Scorer::addScore(const std::string& benchmarkName, const double& rawScore, const double& time) {
    double normalized = processScore(benchmarkName, time);
    scores_.emplace_back(benchmarkName, normalized, time);
}

// Clear all stored scores
void Scorer::clearScores() {
    scores_.clear();
}

double Scorer::processScore(const std::string& benchmarkName, const double& elapsedTime) {
    double baseline = baselineTimes_[benchmarkName];
    double normalized = baseline / elapsedTime;
    return normalized * 2500.0; 
}

// Load custom weights
void Scorer::loadWeights(std::shared_ptr<std::unordered_map<std::string, double>> newWeights) {
    weights_ = *newWeights;
    normalizeWeights();
}

// Load custom baseline times
void Scorer::loadBaselineTimes(std::shared_ptr<std::unordered_map<std::string, double>> newBaselineTimes) {
    baselineTimes_ = *newBaselineTimes;
}

// Compute final weighted score
double Scorer::computeFinalScore() {
    std::unordered_map<std::string, double> averages;

    // Compute average score per benchmark
    for (auto& s : scores_) {
        if (s.benchmarkName == "Combined")
           continue;
        averages[s.benchmarkName] += s.score;
    }
    for (auto& a : averages) {
        int count = 0;
        for (auto& s : scores_)
            if (s.benchmarkName == a.first) count++;

        a.second /= count;
    }

    // Weighted sum
    double finalScore = 0.0;
    for (auto& a : averages) {
        finalScore += a.second * weights_[a.first];
    }

    return finalScore;
}

const std::vector<Score>& Scorer::getScores(){
    return scores_;
}