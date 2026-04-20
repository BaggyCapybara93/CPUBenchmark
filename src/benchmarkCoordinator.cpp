#include "benchmarkCoordinator.hpp"
#include <iostream>

void BenchmarkCoordinator::runMode(RunnerFunction runner){
    for (size_t i = 0; i < args_.getRepeatCount(); i++) {
        const auto& results = runner(args_, benchmark_);
        report_.addScore(results);
    }
    
    std::cout << "===== Results =====\n";
              
    for (const auto& s : report_.getBenchmarkScores()) {
        std::cout << s.benchmarkName
                  << " | Score: " << s.score
                  << " | Time: " << s.time << "s\n";
    }

    if (args_.getMode() == Mode::MultiThreaded) {
        std::cout << "Combined Score: "
                  << report_.getCombinedScore()
                  << "\n";
    }

    if (args_.shouldSaveResults()) {
        report_.saveBenchmark();
    }
}