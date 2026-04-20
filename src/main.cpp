#include "benchmark.hpp"
#include "parseArguments.hpp"
#include "benchmarkReport.hpp"
#include "benchmarkCoordinator.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    ParseArguments args(argc, argv);
    Benchmark benchmark;

    switch (args.getMode()) {
        case Mode::SingleThreaded: {
            BenchmarkReport singleReport("./scores_singlethreaded");
            // Initialize and run the coordinator
            BenchmarkCoordinator coordinator(args, benchmark, singleReport);
            
            // Run the coordinator with the single-threaded runner
            coordinator.runMode(
                [](const ParseArguments& a, Benchmark& b) {
                    return b.runSingleThreadedBenchmark(
                        a.getIterations(),
                        a.getIntensityMultiplier(),
                        a.getMatrixMultiplySize()
                    );
                }
            );
            return 0;
        }
        case Mode::MultiThreaded: {
            BenchmarkReport multiReport("./scores_multithreaded");
            BenchmarkCoordinator coordinator(args, benchmark, multiReport);

            // Run the coordinator with the multi-threaded runner
            coordinator.runMode(
                [](const ParseArguments& a, Benchmark& b) {
                    return b.runMultithreadedBenchmark(
                        a.getThreadCount(),
                        a.getIterations(),
                        a.getIntensityMultiplier(),
                        a.getMatrixMultiplySize()
                    );
                }
            );
            return 0;
        }

        case Mode::Both: {
            std::cout << "==========================================\n";
            std::cout << "Starting Single-Threaded Benchmark...\n";
            
            BenchmarkReport singleReport("./scores_singlethreaded");
            BenchmarkCoordinator singleCoordinator(args, benchmark, singleReport);

            auto singleRunner = [](const ParseArguments& a, Benchmark& b) {
                return b.runSingleThreadedBenchmark(
                    a.getIterations(),
                    a.getIntensityMultiplier(),
                    a.getMatrixMultiplySize()
                );
            };
            
            singleCoordinator.runMode(singleRunner);


            std::cout << "\n\n==========================================\n";
            std::cout << "Starting Multi-Threaded Benchmark...\n";

            BenchmarkReport multiReport("./scores_multithreaded");
            BenchmarkCoordinator multiCoordinator(args, benchmark, multiReport);

            auto multiRunner = [](const ParseArguments& a, Benchmark& b) {
                return b.runMultithreadedBenchmark(
                    a.getThreadCount(),
                    a.getIterations(),
                    a.getIntensityMultiplier(),
                    a.getMatrixMultiplySize()
                );
            };
            
            multiCoordinator.runMode(multiRunner);
            
            return 0;
        }

        case Mode::Invalid:{
            std::cout << "Invalid benchmarking mode. Exiting...\n";
            return 1;
        }
    }
}