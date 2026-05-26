#include "benchmark.hpp"
#include "parseArguments.hpp"
#include "benchmarkReport.hpp"
#include "benchmarkCoordinator.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    ParseArguments args(argc, argv);

    Mode mode = args.getMode();

    switch (mode) {

        case Mode::SingleThreaded: {
            std::cout << "==========================================\n";
            std::cout << "Running Single-Threaded Benchmark...\n";

            BenchmarkReport report("./scores_singlethreaded");
            BenchmarkCoordinator coordinator(args, report);

            coordinator.run(Mode::SingleThreaded);
            return 0;
        }

        case Mode::MultiThreaded: {
            std::cout << "==========================================\n";
            std::cout << "Running Multi-Threaded Benchmark...\n";

            BenchmarkReport report("./scores_multithreaded");
            BenchmarkCoordinator coordinator(args, report);

            coordinator.run(Mode::MultiThreaded);
            return 0;
        }

        case Mode::Both: {
            std::cout << "==========================================\n";
            std::cout << "Running Single-Threaded Benchmark...\n";

            BenchmarkReport singleReport("./scores_singlethreaded");
            BenchmarkCoordinator singleCoordinator(args, singleReport);
            singleCoordinator.run(Mode::SingleThreaded);

            std::cout << "\n\n==========================================\n";
            std::cout << "Running Multi-Threaded Benchmark...\n";

            BenchmarkReport multiReport("./scores_multithreaded");
            BenchmarkCoordinator multiCoordinator(args, multiReport);
            multiCoordinator.run(Mode::MultiThreaded);

            return 0;
        }

        case Mode::None:{
            return 1;
        }

        case Mode::Invalid:
        default: {
            std::cout << "Invalid benchmarking mode. Exiting...\n";
            return 1;
        }
    }
}
