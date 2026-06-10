#pragma once
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <sstream>
#include "parse_arguments.hpp"
#include "benchmark_report.hpp"

class file_manager{
    private:
        const parse_arguments& args_;

        bool ensure_directory(const std::string& path) const;
        bool write_txt(const benchmark_report& report, const std::string& save_folder) const;
        bool write_json(const benchmark_report& report, const std::string& save_folder) const;

        //Timestamp generation
        std::string get_timestamped_file(const std::string& base_name, const std::string& extension) const {
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);

            std::ostringstream oss;
            oss << base_name << "_"
                << std::put_time(std::localtime(&t), "%Y-%m-%d_%H-%M-%S")
                << extension;

            return oss.str(); 
        }
    public:
        file_manager(const parse_arguments& args) 
            : args_(args){};

        void save_report(const benchmark_report& report, const std::string& save_folder) const;

};