#include "benchmark_report.hpp"
#include "file_manager.hpp"
#include "scorer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool approximately_equal(double actual, double expected) {
    return std::abs(actual - expected) < 1e-9 * std::max(1.0, std::abs(expected));
}

void test_scorer() {
    const std::unordered_map<std::string, double> weights = {
        {"alpha", 1.0},
        {"beta", 3.0},
    };
    const std::unordered_map<std::string, double> baselines = {
        {"alpha", 2.0},
        {"beta", 4.0},
    };

    scorer combined;
    combined.load_weights(weights);
    combined.load_baseline_times(baselines);
    combined.add_score("alpha", 1.0);
    combined.add_score("beta", 8.0);

    const double expected = 2500.0 * std::exp(0.25 * std::log(2.0) + 0.75 * std::log(0.5));
    require(approximately_equal(combined.compute_final_score(), expected),
            "combined score should be a weighted geometric mean of baseline ratios");

    scorer partial;
    partial.load_weights(weights);
    partial.load_baseline_times(baselines);
    partial.add_score("alpha", 2.0);
    require(approximately_equal(partial.compute_final_score(), 2500.0),
            "a partial profile should normalize weights over benchmarks that ran");

    bool rejected_invalid_time = false;
    try {
        partial.add_score("alpha", 0.0);
    } catch (const std::invalid_argument&) {
        rejected_invalid_time = true;
    }
    require(rejected_invalid_time, "the scorer must reject non-positive elapsed times");
}

void test_profile(const std::string& profile_path) {
    auto settings = std::make_shared<Settings>();
    file_manager manager(settings);
    manager.load_profile(profile_path);

    require(settings->profile.name == "quick_test", "profile name was not loaded");
    require(settings->profile.benchmarks.size() == 2, "profile benchmarks were not loaded");
    require(settings->iterations == 200 && settings->repeat_count == 3 && settings->num_runs == 4,
            "profile execution settings were not applied");
}

void test_report_combined_score() {
    benchmark_report report;
    report.add_score({score("alpha", 100.0, 1.0), score("Combined", 321.0, 0.0)});
    require(approximately_equal(report.get_combined_score(), 321.0),
            "reports must preserve the scorer's combined score");
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        require(argc == 2, "expected a profile fixture path");
        test_scorer();
        test_profile(argv[1]);
        test_report_combined_score();
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
