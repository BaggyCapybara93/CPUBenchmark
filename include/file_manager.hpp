#pragma once
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <sstream>
#include "parse_arguments.hpp"
#include "benchmark_report.hpp"

class FileManager{
    private:
        const ParseArguments& args_;

        bool ensure_directory(const std::string& path) const;
        bool write_txt(const BenchmarkReport& report, const std::string& save_folder) const;
        bool write_json(const BenchmarkReport& report, const std::string& save_folder) const;

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
        FileManager(const ParseArguments& args) 
            : args_(args){};

        void save_report(const BenchmarkReport& report, const std::string& save_folder) const;

};