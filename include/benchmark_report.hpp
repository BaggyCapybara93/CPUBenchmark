#pragma once
#include "scorer.hpp"
#include <iomanip>
#include <unordered_map>
#include <vector>
#include <iostream>

class benchmark_report{
    private:
        std::vector<score>  benchmark_scores_;
        std::string cpuname = "";
        std::string ram_info = "";

        static std::string collect_cpu_name();
        static std::string collect_ram_info();

    public:
        //Constructor
        benchmark_report();

        //Getters
        const std::vector<score>& get_benchmark_scores() const;
        double get_combined_score() const;
        const std::string& get_cpu_name() const {return cpuname;}
        const std::string& get_ram_info() const {return ram_info;}

        //Add score
        void add_score(const std::vector<score>& benchmarks);
};