#include "scorer.hpp"

void Scorer::normalizeWeights() {
    double sum = 0.0;
    for (auto& w : _weights) sum += w.second;
    for (auto& w : _weights) w.second /= sum;
}

// Add a raw score (elapsed time)
void Scorer::addScore(const std::string& benchmarkName, const double& rawScore, const double& time) {
    double normalized = processScore(benchmarkName, time);
    _scores.emplace_back(benchmarkName, normalized, time);
}

// Clear all stored scores
void Scorer::clearScores() {
    _scores.clear();
}

double Scorer::processScore(const std::string& benchmarkName, const double& elapsedTime) {
    double baseline = _baselineTimes[benchmarkName];
    double normalized = baseline / elapsedTime;
    return normalized * 2500.0; 
}

// Load custom weights
void Scorer::loadWeights(std::shared_ptr<std::unordered_map<std::string, double>> newWeights) {
    _weights = *newWeights;
    normalizeWeights();
}

// Load custom baseline times
void Scorer::loadBaselineTimes(std::shared_ptr<std::unordered_map<std::string, double>> newBaselineTimes) {
    _baselineTimes = *newBaselineTimes;
}

// Compute final weighted score
double Scorer::computeFinalScore() {
    std::unordered_map<std::string, double> averages;

    // Compute average score per benchmark
    for (auto& s : _scores) {
        if (s.benchmarkName == "Combined")
           continue;
        averages[s.benchmarkName] += s.score;
    }
    for (auto& a : averages) {
        int count = 0;
        for (auto& s : _scores)
            if (s.benchmarkName == a.first) count++;

        a.second /= count;
    }

    // Weighted sum
    double finalScore = 0.0;
    for (auto& a : averages) {
        finalScore += a.second * _weights[a.first];
    }

    return finalScore;
}

const std::vector<Score>& Scorer::getScores(){
    return _scores;
}