#pragma once
#include "scorer.hpp"
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <sstream>
#include <unordered_map>
#include <vector>

class BenchmarkReport{
    private:
        std::string saveFolder_;
        std::vector<Score>  benchmarkScores_;

        //Timestamp generation
        std::string getTimestampedFile(const std::string& baseName, const std::string& extension) const {
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);

            std::ostringstream oss;
            oss << baseName << "_"
                << std::put_time(std::localtime(&t), "%Y-%m-%d_%H-%M-%S")
                << extension;

            return oss.str(); 
        }
            
    public:
        //Constructor
        BenchmarkReport(const std::string& saveFolder);

        //Getters
        const std::string& getSaveFolder() const;
        const std::vector<Score>& getBenchmarkScores() const;
        double getCombinedScore() const;

        //Setters
        void setSaveFolder(const std::string& newSaveFolder);

        //Add score
        void addScore(const std::vector<Score>& benchmarks);

        //Save benchmark
        void saveBenchmark();
};