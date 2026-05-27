#include "benchmark_report.hpp"
#include <iostream>

// Constructor
BenchmarkReport::BenchmarkReport(){}

// Getters
const std::vector<Score>& BenchmarkReport::getBenchmarkScores() const {
    return benchmarkScores_;
}

double BenchmarkReport::getCombinedScore() const {
    try{
        double total = 0.0;
        int count = 0;

        for (const auto& s : benchmarkScores_) {
            if (s.benchmarkName == "Combined")
                continue; // skip the synthetic entry

            total += s.score;
            count++;
        }

        if (count == 0)
            return 0.0;

        return total / count; 
    } catch (const std::exception& e) {
        std::cerr << "Error calculating combined score: " << e.what() << "\n";
        return 0.0;
    }
}

// Add scores (append)
void BenchmarkReport::addScore(const std::vector<Score>& benchmarks) {
    try{
        benchmarkScores_.insert(
            benchmarkScores_.end(),
            benchmarks.begin(),
            benchmarks.end()
        );
    } catch (const std::exception& e) {
        std::cerr << "Error adding scores: " << e.what() << "\n";   
    }
}