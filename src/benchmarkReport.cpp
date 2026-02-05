#include "benchmarkReport.hpp"
#include <iostream>
#include <filesystem>

//Constructor
    BenchmarkReport::BenchmarkReport(const std::string& saveFolder) : saveFolder_(saveFolder){}

//Getters
    const std::string& BenchmarkReport::getSaveFolder() const{return saveFolder_;}
    const std::unordered_map<std::string, std::vector<double>>& BenchmarkReport::getBenchmarkScores() const{return benchmarkScores_;}

//Setters
void BenchmarkReport::setSaveFolder(const std::string& newSaveFolder){saveFolder_ = newSaveFolder;}

//Add score
void BenchmarkReport::addScore(std::unordered_map<std::string, std::vector<double>>& benchmarks){
    benchmarkScores_.insert(benchmarks.begin(), benchmarks.end());
}

//Save benchmark
void BenchmarkReport::saveBenchmark(){
    std::filesystem::create_directories(saveFolder_);
    std::string filename = getTimestampedFile("benchmark_scores", ".txt");

    std::filesystem::path fullPath = std::filesystem::path(saveFolder_) / filename;

    std::ofstream file(fullPath);
    if (file.is_open()) {
        for (const auto& [benchmarkType, scores] : benchmarkScores_) {
            file << benchmarkType << " Scores: ";
            for (size_t i = 0; i < scores.size(); ++i) {
                file << scores[i];
                if (i < scores.size() - 1) {
                    file << ", ";
                }
            }
            file << "\n";
        }
        file.close();
    } else {
        std::cerr << "Failed to open file: " << filename << "\n";
    }
}