#pragma once
#include "scorer.hpp"
#include <string>
#include <iomanip>
#include <unordered_map>
#include <vector>

class BenchmarkReport{
    private:
        std::vector<Score>  benchmarkScores_;
            
    public:
        //Constructor
        BenchmarkReport();

        //Getters
        const std::vector<Score>& getBenchmarkScores() const;
        double getCombinedScore() const;

        //Add score
        void addScore(const std::vector<Score>& benchmarks);
};