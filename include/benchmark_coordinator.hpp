#pragma once
#include "parse_arguments.hpp"
#include "benchmark.hpp"
#include "benchmark_report.hpp"
#include <functional>
#include <barrier>

//Move this to a threading util file in the future
class BenchmarkThreadPool{
    private:
        int numThreads_;
        std::vector<std::thread> workers_;
        std::function<void()> currentTask_;
        std::barrier<> startBarrier_;
        std::barrier<> doneBarrier_;
        bool stop_ = false; 

        void workerLoop();
    public:
        BenchmarkThreadPool(int numThreads)
            :   numThreads_(numThreads),
                startBarrier_(numThreads + 1),   // +1 for the main thread
                doneBarrier_(numThreads + 1)
            {
                workers_.reserve(static_cast<size_t>(numThreads));
                for (int i = 0; i < numThreads_; ++i) {
                    workers_.emplace_back([this]() { workerLoop(); });
                }
            }

        ~BenchmarkThreadPool() {
            stop_ = true;

            // Release workers waiting on start barrier
            startBarrier_.arrive_and_wait();

            for (auto& t : workers_)
                t.join();
        }

        void runAll(const std::function<void()>& fn);
};

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