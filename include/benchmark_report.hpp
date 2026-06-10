#pragma once
#include "scorer.hpp"
#include <string>
#include <iomanip>
#include <unordered_map>
#include <vector>

class benchmark_report{
    private:
        std::vector<score>  benchmark_scores_;
            
    public:
        //Constructor
        benchmark_report();

        //Getters
        const std::vector<score>& get_benchmark_scores() const;
        double get_combined_score() const;

        //Add score
        void add_score(const std::vector<score>& benchmarks);
};