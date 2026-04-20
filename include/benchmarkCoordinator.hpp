#pragma once
#include "parseArguments.hpp"
#include "benchmark.hpp"
#include "benchmarkReport.hpp"
#include <functional>

class BenchmarkCoordinator{
    private:
        const ParseArguments& args_;
        Benchmark& benchmark_;
        BenchmarkReport& report_;

        using RunnerFunction = std::function<std::vector<Score>(const ParseArguments&, Benchmark&)>;
    
    public:
        BenchmarkCoordinator(const ParseArguments& args, Benchmark& benchmark, BenchmarkReport& report)
            : args_(args), benchmark_(benchmark), report_(report){}

        void runMode(RunnerFunction runner);
};      