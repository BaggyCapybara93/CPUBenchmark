#pragma once
#include "parseArguments.hpp"
#include "benchmark.hpp"
#include "benchmarkReport.hpp"
#include <functional>

class BenchmarkCoordinator{
    private:
        const ParseArguments& args_;
        BenchmarkReport& report_;

        using RunnerFunction = std::function<std::vector<Score>(const ParseArguments&, BenchmarkCoordinator&)>;
    
        //These should be changed to be more useful and customizable.
        static std::vector<Score> runMultithreadedBenchmark(const int numThreads, int iterationsPerThread, const int intensityMultiplier = 50, const int matrixMultiplySize = 128, const int numRuns = 10);
        static std::vector<Score> runSingleThreadedBenchmark(int iterations, const int intensityMultiplier = 50, const int matrixMultiplySize = 128, const int numRuns = 10);

        void runBenchmark(RunnerFunction runner);

    public:
        BenchmarkCoordinator(const ParseArguments& args, BenchmarkReport& report)
            : args_(args), report_(report){}

        void run(const Mode& mode);
};      