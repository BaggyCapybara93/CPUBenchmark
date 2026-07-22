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

        static std::string collect_cpu_name();

    public:
        //Constructor
        benchmark_report();

        //Getters
        const std::vector<score>& get_benchmark_scores() const;
        double get_combined_score() const;
        const std::string& get_cpu_name() const {return cpuname;}

        //Add score
        void add_score(const std::vector<score>& benchmarks);
};