#include "benchmark.hpp"
#include "parse_arguments.hpp"
#include "benchmark_report.hpp"
#include "benchmark_coordinator.hpp"
#include "file_manager.hpp"
#include <filesystem>
#include <iostream>
#include <memory>

std::filesystem::path get_output_root(char* argv[]) {
    if (argv == nullptr || argv[0] == nullptr || argv[0][0] == '\0') {
        return std::filesystem::current_path();
    }

    std::filesystem::path executable_path(argv[0]);
    if (!executable_path.is_absolute()) {
        executable_path = std::filesystem::absolute(executable_path);
    }

    std::error_code ec;
    const auto resolved_path = std::filesystem::weakly_canonical(executable_path, ec);
    if (!ec && resolved_path.has_parent_path()) {
        return resolved_path.parent_path();
    }

    return executable_path.has_parent_path()
        ? executable_path.parent_path()
        : std::filesystem::current_path();
}

int main(int argc, char* argv[]) {
    std::shared_ptr<Settings> settings = std::make_shared<Settings>();

    parse_arguments args(argc, argv, settings);
    file_manager fm(settings);
    const std::filesystem::path output_root = get_output_root(argv);
    const std::filesystem::path single_threaded_output_dir = output_root / "scores" / "single_threaded";
    const std::filesystem::path multi_threaded_output_dir = output_root / "scores" / "multi_threaded";

    mode mode = settings.get()->mode;

    switch (mode) {

        case mode::single_threaded: {
            std::cout << "==========================================\n";
            std::cout << "Running Single-Threaded Benchmark...\n";

            benchmark_report report;
            benchmark_coordinator coordinator(report, settings);

            coordinator.start(mode::single_threaded);
            fm.save_report(report, single_threaded_output_dir.string());
            return 0;
        }

        case mode::multi_threaded: {
            std::cout << "==========================================\n";
            std::cout << "Running Multi-Threaded Benchmark...\n";

            benchmark_report report;
            benchmark_coordinator coordinator(report, settings);

            coordinator.start(mode::multi_threaded);
            fm.save_report(report, multi_threaded_output_dir.string());
            return 0;
        }

        case mode::both: {
            std::cout << "==========================================\n";
            std::cout << "Running Single-Threaded Benchmark...\n";

            benchmark_report single_report;
            benchmark_coordinator single_coordinator(single_report, settings);
            single_coordinator.start(mode::single_threaded);
            fm.save_report(single_report, single_threaded_output_dir.string());

            std::cout << "\n\n==========================================\n";
            std::cout << "Running Multi-Threaded Benchmark...\n";

            benchmark_report multi_report;
            benchmark_coordinator multi_coordinator(multi_report, settings);
            multi_coordinator.start(mode::multi_threaded);
            fm.save_report(multi_report, multi_threaded_output_dir.string());

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
