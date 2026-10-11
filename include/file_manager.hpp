#pragma once
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <memory>
#include <iomanip>

#include "settings.hpp"
#include "parse_arguments.hpp"
#include "benchmark_report.hpp"

class file_manager{
    private:
        std::shared_ptr<Settings> settings_;

        bool ensure_directory(const std::string& path) const;
        bool write_txt(const benchmark_report& report, const std::string& save_folder) const;
        bool write_json(const benchmark_report& report, const std::string& save_folder) const;

        //Timestamp generation
        std::string get_timestamped_file(const std::string& base_name, const std::string& extension) const {
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(
                now.time_since_epoch()).count() % 1000000;

            std::ostringstream oss;
            oss << base_name << "_"
                << std::put_time(std::localtime(&t), "%Y-%m-%d_%H-%M-%S")
                << "_" << std::setw(6) << std::setfill('0') << microseconds
                << extension;

            return oss.str(); 
        }

        //Profile save/load helpers
        bool write_profile_txt(const benchmark_profile& profile, const std::string& save_folder) const;
        bool read_profile_txt(const std::string& path, benchmark_profile& profile) const;
    public:
        file_manager(std::shared_ptr<Settings> settings) 
            : settings_(std::move(settings)) {};

        void read_profile(const std::string& path);
        void save_report(const benchmark_report& report, const std::string& save_folder) const;
        bool save_profile(const benchmark_profile& profile, const std::string& save_folder) const;
        void load_profile(const std::string& profile_path);

};
