#pragma once
#include "scorer.hpp"
#include <iomanip>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <string>
#include <filesystem>

class benchmark_report{
    private:
        std::vector<score>  benchmark_scores_;
        std::string cpuname = "";
        std::string ram_info = "";
        std::string os_info = "";
        std::string compiler_info = "";
        std::string cpu_frequency_info = "";
        unsigned int logical_cpu_cores_ = 0;
        int active_thread_count_ = 0;

        static std::string collect_cpu_name();
        static std::string collect_ram_info();
        static std::string collect_os_info();
        static std::string collect_compiler_info();
        static std::string collect_cpu_frequency_info();

    public:
        //Constructor
        benchmark_report();

        //Getters
        const std::vector<score>& get_benchmark_scores() const;
        double get_combined_score() const;
        const std::string& get_cpu_name() const {return cpuname;}
        const std::string& get_ram_info() const {return ram_info;}
        const std::string& get_os_info() const {return os_info;}
        const std::string& get_compiler_info() const {return compiler_info;}
        const std::string& get_cpu_frequency_info() const {return cpu_frequency_info;}
        unsigned int get_logical_cpu_cores() const {return logical_cpu_cores_;}
        int get_active_thread_count() const {return active_thread_count_;}
        void set_active_thread_count(int thread_count) {active_thread_count_ = thread_count;}
        
        // Get statistics for a benchmark
        struct benchmark_stats {
            double min_time;
            double max_time;
            double std_dev;
            double p95;
            double p99;
            int count;
        };
        
        benchmark_stats get_stats(const std::string& benchmark_name) const;

        //Add score
        void add_score(const std::vector<score>& benchmarks);
        
        // Add scores with statistics
        void add_scores_with_stats(const std::vector<std::pair<std::string, double>>& benchmark_times);
        
        // Compute statistics for all benchmarks
        void compute_all_statistics();
        
        // Compare with previous benchmark runs
        bool compare_with_previous(const std::string& save_folder, const std::string& mode_type,
                                   const std::string& current_report_file) const;
        
        // Check if CPU and RAM are similar between runs
        static bool are_systems_similar(const std::string& cpu1, const std::string& ram1, 
                                        const std::string& cpu2, const std::string& ram2,
                                        double similarity_threshold = 0.8);
};
