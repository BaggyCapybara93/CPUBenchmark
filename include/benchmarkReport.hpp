#pragma once

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
        std::unordered_map<std::string, std::vector<double>> benchmarkScores_; //Benchmark Type -> scores

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
        const std::unordered_map<std::string, std::vector<double>>& getBenchmarkScores() const;

        //Setters
        void setSaveFolder(const std::string& newSaveFolder);

        //Add score
        void addScore(std::unordered_map<std::string, std::vector<double>>& benchmarks);

        //Remove score
            //To be implemented 

        //Save benchmark
        void saveBenchmark();
};