#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cmath>

struct Score {
    std::string benchmarkName;
    double score; 
    double time; 

    Score(const std::string& name, double s, double t)
        : benchmarkName(name), score(s), time(t) {}
};

class Scorer {
private:
    std::vector<Score> _scores;

    std::unordered_map<std::string, double> _weights = {
        {"FloatingPoint", 0.25},
        {"Branch", 0.10},
        {"Integer",       0.20},
        {"Sorting",       0.20},
        {"MatrixMultiply",0.20},
        {"NBody",         0.15}
    };

    std::unordered_map<std::string, double> _baselineTimes = {
        {"FloatingPoint", 0.020},
        {"Branch", 0.020},
        {"Integer",       0.015},
        {"Sorting",       0.050},
        {"MatrixMultiply",0.040},
        {"NBody",         0.030},
        {"Combined",         0.150}
    };

    // Normalize weights to 1.0
    void normalizeWeights();

public:

    // Add a raw score (elapsed time)
    void addScore(const std::string& benchmarkName, const double& rawScore, const double& time);

    // Clear all stored scores
    void clearScores();

    double processScore(const std::string& benchmarkName, const double& elapsedTime);

    // Load custom weights
    void loadWeights(std::shared_ptr<std::unordered_map<std::string, double>> newWeights);

    // Load custom baseline times
    void loadBaselineTimes(std::shared_ptr<std::unordered_map<std::string, double>> newBaselineTimes);

    // Compute final weighted score
    double computeFinalScore();

    //Get the score for each specific method
    const std::vector<Score>& getScores();
};