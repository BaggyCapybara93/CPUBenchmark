#include "benchmark.hpp"
#include "parse_arguments.hpp"
#include "benchmark_report.hpp"
#include "benchmark_coordinator.hpp"
#include "file_manager.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    ParseArguments args(argc, argv);
    FileManager fm(args);

    Mode mode = args.getMode();

    switch (mode) {

        case Mode::SingleThreaded: {
            std::cout << "==========================================\n";
            std::cout << "Running Single-Threaded Benchmark...\n";

            BenchmarkReport report;
            BenchmarkCoordinator coordinator(args, report);

            coordinator.run(Mode::SingleThreaded);
            fm.save_report(report, "./scores/single_threaded");
            return 0;
        }

        case Mode::MultiThreaded: {
            std::cout << "==========================================\n";
            std::cout << "Running Multi-Threaded Benchmark...\n";

            BenchmarkReport report;
            BenchmarkCoordinator coordinator(args, report);

            coordinator.run(Mode::MultiThreaded);
            fm.save_report(report, "./scores/multi_threaded");
            return 0;
        }

        case Mode::Both: {
            std::cout << "==========================================\n";
            std::cout << "Running Single-Threaded Benchmark...\n";

            BenchmarkReport singleReport;
            BenchmarkCoordinator singleCoordinator(args, singleReport);
            singleCoordinator.run(Mode::SingleThreaded);
            fm.save_report(singleReport, "./scores/single_threaded");

            std::cout << "\n\n==========================================\n";
            std::cout << "Running Multi-Threaded Benchmark...\n";

            BenchmarkReport multiReport;
            BenchmarkCoordinator multiCoordinator(args, multiReport);
            multiCoordinator.run(Mode::MultiThreaded);
            fm.save_report(multiReport, "./scores/multi_threaded");

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
