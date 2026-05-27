#include "benchmark_report.hpp"
#include <iostream>

// Constructor
BenchmarkReport::BenchmarkReport(const std::string& saveFolder)
    : saveFolder_(saveFolder){}

// Getters
const std::string& BenchmarkReport::getSaveFolder() const {
    return saveFolder_;
}

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

// Setter
void BenchmarkReport::setSaveFolder(const std::string& newSaveFolder) {
    saveFolder_ = newSaveFolder;
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

// Save benchmark results
void BenchmarkReport::saveBenchmark() {
    try {
        try{
            std::filesystem::create_directories(saveFolder_);
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error creating directory: " << e.what() << "\n";
            return;
        }

        std::string filename = getTimestampedFile("benchmark_scores", ".txt");

        std::filesystem::path fullPath = std::filesystem::path(saveFolder_) / filename;

        std::ofstream file(fullPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << "\n";
            return;
        }

        for (const auto& s : benchmarkScores_) {
            file << s.benchmarkName
                << " | Score: " << s.score
                << " | Time: " << s.time << "s\n";
        }

        file.close();
        std::cout << "Benchmark saved to: " << fullPath << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error saving benchmark: " << e.what() << "\n";
    }
}