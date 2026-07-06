#include "benchmark.hpp"
#include "parse_arguments.hpp"
#include "benchmark_report.hpp"
#include "benchmark_coordinator.hpp"
#include "file_manager.hpp"
#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
    std::shared_ptr<Settings> settings = std::make_shared<Settings>();

    parse_arguments args(argc, argv, settings);
    file_manager fm(settings);

    

    mode mode = settings.get()->mode;

    switch (mode) {

        case mode::single_threaded: {
            std::cout << "==========================================\n";
            std::cout << "Running Single-Threaded Benchmark...\n";

            benchmark_report report;
            benchmark_coordinator coordinator(report, settings);

            coordinator.start(mode::single_threaded);
            fm.save_report(report, "./scores/single_threaded");
            return 0;
        }

        case mode::multi_threaded: {
            std::cout << "==========================================\n";
            std::cout << "Running Multi-Threaded Benchmark...\n";

            benchmark_report report;
            benchmark_coordinator coordinator(report, settings);

            coordinator.start(mode::multi_threaded);
            fm.save_report(report, "./scores/multi_threaded");
            return 0;
        }

        case mode::both: {
            std::cout << "==========================================\n";
            std::cout << "Running Single-Threaded Benchmark...\n";

            benchmark_report single_report;
            benchmark_coordinator single_coordinator(single_report, settings);
            single_coordinator.start(mode::single_threaded);
            fm.save_report(single_report, "./scores/single_threaded");

            std::cout << "\n\n==========================================\n";
            std::cout << "Running Multi-Threaded Benchmark...\n";

            benchmark_report multi_report;
            benchmark_coordinator multi_coordinator(multi_report, settings);
            multi_coordinator.start(mode::multi_threaded);
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
