#include "benchmark.hpp"
#include "parse_arguments.hpp"
#include "benchmark_report.hpp"
#include "benchmark_coordinator.hpp"
#include "file_manager.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    parse_arguments args(argc, argv);
    file_manager fm(args);

    mode mode = args.get_mode();

    switch (mode) {

        case mode::single_threaded: {
            std::cout << "==========================================\n";
            std::cout << "Running Single-Threaded Benchmark...\n";

            benchmark_report report;
            benchmark_coordinator coordinator(args, report);

            coordinator.run(mode::single_threaded);
            fm.save_report(report, "./scores/single_threaded");
            return 0;
        }

        case mode::multi_threaded: {
            std::cout << "==========================================\n";
            std::cout << "Running Multi-Threaded Benchmark...\n";

            benchmark_report report;
            benchmark_coordinator coordinator(args, report);

            coordinator.run(mode::multi_threaded);
            fm.save_report(report, "./scores/multi_threaded");
            return 0;
        }

        case mode::both: {
            std::cout << "==========================================\n";
            std::cout << "Running Single-Threaded Benchmark...\n";

            benchmark_report single_report;
            benchmark_coordinator single_coordinator(args, single_report);
            single_coordinator.run(mode::single_threaded);
            fm.save_report(single_report, "./scores/single_threaded");

            std::cout << "\n\n==========================================\n";
            std::cout << "Running Multi-Threaded Benchmark...\n";

            benchmark_report multi_report;
            benchmark_coordinator multi_coordinator(args, multi_report);
            multi_coordinator.run(mode::multi_threaded);
            fm.save_report(multi_report, "./scores/multi_threaded");

            return 0;
        }

        case mode::none:{
            return 1;
        }

        case mode::invalid:
        default: {
            std::cout << "Invalid benchmarking mode. Exiting...\n";
            return 1;
        }
    }
}
