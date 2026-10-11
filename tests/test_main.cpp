#include "benchmark.hpp"
#include "benchmark_report.hpp"
#include "benchmark_coordinator.hpp"
#include "file_manager.hpp"
#include "scorer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
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

void test_run_samples_and_statistics() {
    int setups = 0;
    int executions = 0;
    const auto samples = benchmark::run_benchmark(
        [&]() {
            require(setups == executions + 1, "each run must receive setup before execution");
            ++executions;
        },
        4,
        [&]() { ++setups; });
    require(samples.size() == 4 && setups == 9 && executions == 9,
            "warmups must be excluded from measured samples");

    scorer measured;
    measured.load_weights({{"alpha", 1.0}});
    measured.load_baseline_times({{"alpha", 2.0}});
    const double mean = measured.add_score("alpha", {1.0, 2.0, 3.0, 4.0});
    require(approximately_equal(mean, 2.5), "mean must use every measured run");
    const auto scores = measured.get_scores();
    require(scores.size() == 1 && scores.front().samples_.size() == 4,
            "scores must retain every measured run");
    require(approximately_equal(scores.front().score_, 2000.0),
            "scores must use the mean of measured runs");

    benchmark_report report;
    report.add_score({scores.front(), score("Combined", measured.compute_final_score(), 0.0)});
    const auto stats = report.get_stats(scores.front());
    require(stats.count == 4 && approximately_equal(stats.min_time, 1.0) &&
                approximately_equal(stats.max_time, 4.0) &&
                approximately_equal(stats.median_time, 2.5) &&
                approximately_equal(stats.std_dev, std::sqrt(1.25)) &&
                approximately_equal(stats.p95, 3.85) && approximately_equal(stats.p99, 3.97),
            "report statistics must describe measured samples");

    report.add_score({score("alpha", 2500.0, 2.0, {1.0, 3.0})});
    const auto across_repetitions = report.get_stats("alpha");
    require(across_repetitions.count == 6 && approximately_equal(across_repetitions.median_time, 2.5),
            "aggregate statistics must include samples from every suite repetition");
}

void test_arithmetic_kernels() {
    std::uint32_t expected_integer_add = 0;
    std::uint32_t expected_integer_multiply = 0;
    std::uint32_t expected_integer_divide = 0;
    double expected_floating_add = 0.0;
    double expected_floating_multiply = 0.0;
    double expected_floating_divide = 0.0;
    double expected_floating_fma = 0.0;
    for (std::uint32_t lane = 0; lane < benchmark::arithmetic_lanes; ++lane) {
        const std::uint32_t initial = lane + 1;
        const std::uint32_t operand = 3U + 2U * lane;
        const std::uint32_t numerator = 100'003U + 977U * lane;
        expected_integer_add += initial + operand;
        expected_integer_multiply += initial * operand;
        expected_integer_divide += initial + (numerator + (initial & 1023U)) / operand;
        const double factor = 1.0 + static_cast<double>(lane + 1) * 1e-7;
        expected_floating_add += static_cast<double>(initial) + factor;
        expected_floating_multiply += static_cast<double>(initial) * factor;
        expected_floating_divide += static_cast<double>(initial) / factor;
        expected_floating_fma += std::fma(static_cast<double>(initial), factor,
                                          static_cast<double>(lane + 1) * 1e-4);
    }

    using integer_workload = benchmark::integer_kernel_workload;
    using floating_workload = benchmark::floating_kernel_workload;
    require(integer_workload(1, arithmetic_operation::add).run() == expected_integer_add &&
                integer_workload(1, arithmetic_operation::multiply).run() == expected_integer_multiply &&
                integer_workload(1, arithmetic_operation::divide).run() == expected_integer_divide,
            "integer kernels must perform their named operation");
    require(approximately_equal(floating_workload(1, arithmetic_operation::add).run(), expected_floating_add) &&
                approximately_equal(floating_workload(1, arithmetic_operation::multiply).run(), expected_floating_multiply) &&
                approximately_equal(floating_workload(1, arithmetic_operation::divide).run(), expected_floating_divide) &&
                approximately_equal(floating_workload(1, arithmetic_operation::fma).run(), expected_floating_fma),
            "floating-point kernels must perform their named operation");
    require(std::isfinite(floating_workload(200'000, arithmetic_operation::multiply).run()) &&
                std::isfinite(floating_workload(200'000, arithmetic_operation::divide).run()) &&
                std::isfinite(floating_workload(200'000, arithmetic_operation::fma).run()),
            "floating-point kernels must remain finite over long runs");
}

void test_branch_patterns() {
    for (int iterations : {1, 2, 101, 1000}) {
        benchmark::branch_pattern_workload predictable(
            iterations, branch_pattern::predictable);
        benchmark::branch_pattern_workload unpredictable(
            iterations, branch_pattern::unpredictable);
        const auto expected_taken = static_cast<std::uint64_t>((iterations + 1) / 2);
        require(predictable.taken_count() == expected_taken &&
                    unpredictable.taken_count() == expected_taken &&
                    predictable.run() == expected_taken &&
                    unpredictable.run() == expected_taken,
                "branch patterns must make the same number of decisions and taken branches");
    }

    auto settings = std::make_shared<Settings>();
    settings->thread_count = 3;
    const std::vector<std::string> names = {
        "branch_predictable", "branch_unpredictable"};
    const auto profile_path = std::filesystem::temp_directory_path() /
        ("cpubenchmark_branch_patterns_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".txt");
    {
        std::ofstream profile(profile_path);
        profile << "name=branch_patterns\n"
                   "benchmarks=branch_predictable,branch_unpredictable\n"
                   "iterations=401\nintensity_multiplier=1\n"
                   "num_runs=2\nrepeat_count=1\n";
    }
    file_manager manager(settings);
    manager.load_profile(profile_path.string());
    std::filesystem::remove(profile_path);
    require(settings->profile.benchmarks == names,
            "profile loading must accept both branch pattern kernels");
    for (const mode run_mode : {mode::single_threaded, mode::multi_threaded}) {
        benchmark_report report;
        benchmark_coordinator coordinator(report, settings);
        coordinator.start_with_profile(names, run_mode);
        const auto& results = report.get_benchmark_scores();
        require(results.size() == 3 && results[0].benchmark_name_ == names[0] &&
                    results[1].benchmark_name_ == names[1],
                "both branch patterns must be independently selectable");
        for (std::size_t i = 0; i < names.size(); ++i) {
            require(settings->known_benchmarks.contains(names[i]) &&
                        results[i].total_work_ == 401 &&
                        results[i].work_unit_ == "branch decisions" &&
                        results[i].samples_.size() == 2 && results[i].work_per_second() &&
                        report.get_run_configuration().at("weights").contains(names[i]) &&
                        report.get_run_configuration().at("baselineTimes").contains(names[i]),
                    "branch pattern results must report equal work and scoring settings");
        }
    }
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

void test_required_io_failures() {
    auto settings = std::make_shared<Settings>();
    file_manager manager(settings);
    const auto directory = std::filesystem::temp_directory_path() /
        ("cpubenchmark_io_failures_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);

    bool rejected_missing_profile = false;
    try {
        manager.load_profile((directory / "missing.txt").string());
    } catch (const std::runtime_error&) {
        rejected_missing_profile = true;
    }
    require(rejected_missing_profile && settings->profile.benchmarks.empty(),
            "a missing profile must fail without changing settings");

    const auto invalid_profile = directory / "invalid.txt";
    {
        std::ofstream file(invalid_profile);
        file << "benchmarks=unknown_benchmark\n";
    }
    bool rejected_invalid_profile = false;
    try {
        manager.load_profile(invalid_profile.string());
    } catch (const std::runtime_error&) {
        rejected_invalid_profile = true;
    }
    require(rejected_invalid_profile && settings->profile.benchmarks.empty(),
            "an invalid profile must fail without changing settings");

    const auto empty_profile = directory / "empty.txt";
    {
        std::ofstream file(empty_profile);
        file << "name=empty\n";
    }
    bool rejected_empty_profile = false;
    try {
        manager.load_profile(empty_profile.string());
    } catch (const std::runtime_error&) {
        rejected_empty_profile = true;
    }
    require(rejected_empty_profile && settings->profile.benchmarks.empty(),
            "an empty profile must not fall back to the default suite");

    const auto blocked_folder = directory / "existing_file";
    {
        std::ofstream file(blocked_folder);
        file << "block report directory creation";
    }
    benchmark_report report;
    report.add_score({score("alpha", 100.0, 1.0)});
    for (const std::string method : {"txt", "json"}) {
        settings->save_method = method;
        bool rejected_failed_save = false;
        try {
            manager.save_report(report, blocked_folder.string());
        } catch (const std::runtime_error&) {
            rejected_failed_save = true;
        }
        require(rejected_failed_save, "a failed report save must reach its caller");
    }
    settings->save_method = "unsupported";
    bool rejected_save_method = false;
    try {
        manager.save_report(report, directory.string());
    } catch (const std::invalid_argument&) {
        rejected_save_method = true;
    }
    require(rejected_save_method, "an unsupported save method must fail");
    std::filesystem::remove_all(directory);
}

void test_report_combined_score() {
    benchmark_report report;
    report.set_active_thread_count(4);
    report.add_score({score("alpha", 100.0, 1.0), score("Combined", 321.0, 0.0)});
    require(approximately_equal(report.get_combined_score(), 321.0),
            "reports must preserve the scorer's combined score");
    require(!report.get_os_info().empty() && !report.get_compiler_info().empty() &&
                !report.get_ram_type().empty() && !report.get_ram_speed().empty() &&
                report.get_active_thread_count() == 4,
            "reports must expose system metadata");
}

void test_report_serialization() {
    auto settings = std::make_shared<Settings>();
    file_manager manager(settings);
    scorer measured;
    measured.add_score("memory_bandwidth", {2.0, 4.0}, "elements processed",
                       3'000'000'000ULL, 4 * sizeof(double));
    const auto workload = measured.get_scores().front();
    require(workload.work_per_second() &&
                approximately_equal(*workload.work_per_second(), 1'000'000'000.0) &&
                workload.estimated_gb_per_second() &&
                approximately_equal(*workload.estimated_gb_per_second(), 32.0),
            "throughput must use work per sample and mean measured time");
    require(!score("Combined", 100.0, 0.0).work_per_second(),
            "synthetic scores must not have throughput");
    benchmark_report report;
    report.set_run_configuration(*settings, {}, 1);
    report.add_score({workload});
    const auto directory = std::filesystem::temp_directory_path() /
        ("cpubenchmark_memory_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));

    for (const std::string method : {"txt", "json"}) {
        settings->save_method = method;
        const auto folder = directory / method;
        manager.save_report(report, folder.string());
        require(std::filesystem::exists(folder) &&
                    std::filesystem::directory_iterator(folder) != std::filesystem::directory_iterator(),
                "report file was not written");
        const auto path = std::filesystem::directory_iterator(folder)->path();
        std::ifstream input(path);
        require(input.good(), "could not read saved report");
        if (method == "json") {
            const auto data = nlohmann::json::parse(input);
            require(data.at("ramType") == report.get_ram_type() &&
                        data.at("ramSpeed") == report.get_ram_speed(),
                    "JSON report must include RAM type and speed");
            const auto& result = data.at("scores").at(0);
            require(approximately_equal(result.at("workPerSecond").get<double>(), 1'000'000'000.0) &&
                        result.at("throughputUnit") == "elements processed/s" &&
                        approximately_equal(result.at("estimatedLogicalGBps").get<double>(), 32.0) &&
                        result.at("estimatedBytesPerWork") == 32,
                    "JSON report must include work throughput and estimated logical GB/s");
        } else {
            std::ostringstream text;
            text << input.rdbuf();
            require(text.str().find("RAM Type: " + report.get_ram_type()) != std::string::npos &&
                        text.str().find("RAM Speed: " + report.get_ram_speed()) != std::string::npos,
                    "text report must include RAM type and speed");
            require(text.str().find("Throughput: 1000000000.00 elements processed/s") != std::string::npos &&
                        text.str().find("Estimated Logical Throughput: 32.000 GB/s") != std::string::npos,
                    "text report must include work throughput and estimated logical GB/s");
        }
    }
    std::filesystem::remove_all(directory);
}

void test_comparison_requires_matching_configuration() {
    Settings settings;
    settings.thread_count = 2;
    settings.iterations = 400;
    benchmark_report report;
    report.set_run_configuration(settings, {"alpha"}, 1);
    report.add_score({score("alpha", 100.0, 1.0)});

    const auto directory = std::filesystem::temp_directory_path() /
        ("cpubenchmark_comparison_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    const auto older = directory / "benchmark_scores_2026-01-01_00-00-00.txt";
    const auto newer = directory / "benchmark_scores_2026-01-02_00-00-00.txt";
    const auto write_run = [&](const std::filesystem::path& path, const nlohmann::json& config,
                               bool include_time = true) {
        std::ofstream output(path);
        require(output.good(), "could not create comparison fixture");
        output << "CPU: " << report.get_cpu_name() << '\n';
        output << "RAM: " << report.get_ram_info() << '\n';
        output << "RAM Type: " << report.get_ram_type() << '\n';
        output << "RAM Speed: " << report.get_ram_speed() << '\n';
        output << "Run Configuration: " << config.dump() << '\n';
        output << "alpha\nScore: 100.0\n";
        if (include_time) {
            output << "Time (mean): 1.0s\n";
        }
    };

    write_run(older, report.get_run_configuration());
    auto mismatched = report.get_run_configuration();
    mismatched["activeThreads"] = 2;
    write_run(newer, mismatched);
    require(report.compare_with_previous(directory.string(), "single_threaded", "current.txt"),
            "comparison should find the newest compatible run, skipping incompatible runs");

    write_run(older, report.get_run_configuration());
    {
        std::ofstream output(older, std::ios::app);
        output << "RAM Speed: different\n";
    }
    require(!report.compare_with_previous(directory.string(), "single_threaded", "current.txt"),
            "comparison must reject runs with a different RAM speed");

    write_run(older, report.get_run_configuration(), false);
    require(!report.compare_with_previous(directory.string(), "single_threaded", "current.txt"),
            "comparison must reject incomplete saved measurements");

    mismatched["activeThreads"] = 1;
    mismatched["iterations"] = 401;
    write_run(older, mismatched);
    require(!report.compare_with_previous(directory.string(), "single_threaded", "current.txt"),
            "comparison must reject runs with different work or thread settings");

    mismatched = report.get_run_configuration();
    mismatched["build"]["type"] = "different-build";
    write_run(older, mismatched);
    require(!report.compare_with_previous(directory.string(), "single_threaded", "current.txt"),
            "comparison must reject runs built with different settings");

    mismatched = report.get_run_configuration();
    mismatched["profileName"] = "different-profile";
    write_run(older, mismatched);
    require(!report.compare_with_previous(directory.string(), "single_threaded", "current.txt"),
            "comparison must reject a different profile");
    std::filesystem::remove_all(directory);
}

void test_same_work_in_both_modes() {
    auto settings = std::make_shared<Settings>();
    settings->thread_count = 3;
    settings->iterations = 401;
    settings->intensity_multiplier = 1;
    settings->matrix_multiply_size = 8;
    settings->num_runs = 1;
    settings->repeat_count = 1;
    const std::vector<std::string> names = {
        "floating_point", "integer_arithmetic", "matrix_multiply", "branch_prediction",
        "n_body", "sorting", "memory_bandwidth", "pointer_chase", "cache_latency",
        "integer_add", "integer_multiply", "integer_divide",
        "floating_add", "floating_multiply", "floating_divide", "floating_fma"
    };

    benchmark_report single;
    benchmark_coordinator single_runner(single, settings);
    single_runner.start_with_profile(names, mode::single_threaded);
    benchmark_report multi;
    benchmark_coordinator multi_runner(multi, settings);
    multi_runner.start_with_profile(names, mode::multi_threaded);

    const auto& single_scores = single.get_benchmark_scores();
    const auto& multi_scores = multi.get_benchmark_scores();
    require(single_scores.size() == multi_scores.size() && single_scores.size() == names.size() + 1,
            "both modes must run the same benchmarks");
    for (std::size_t i = 0; i < names.size(); ++i) {
        require(single_scores[i].benchmark_name_ == multi_scores[i].benchmark_name_ &&
                    single_scores[i].total_work_ == multi_scores[i].total_work_ &&
                    single_scores[i].work_unit_ == multi_scores[i].work_unit_,
                "single and multi threaded modes must perform the same recorded work");
    }
    require(single.get_run_configuration().at("logicalShards") == 3 &&
                multi.get_run_configuration().at("logicalShards") == 3 &&
                single.get_run_configuration().at("activeThreads") == 1 &&
                multi.get_run_configuration().at("activeThreads") == 3,
            "reports must distinguish work shards from active threads");
    require(single_scores[1].total_work_ == 398 && single_scores[2].total_work_ == 1536 &&
                single_scores[5].total_work_ == 150000 && single_scores[6].total_work_ == 3208,
            "work totals must include shard remainders and fixed-size workloads");
    require(single_scores[6].estimated_bytes_per_work_ == 32 &&
                multi_scores[6].estimated_bytes_per_work_ == 32 &&
                single_scores[6].estimated_gb_per_second() &&
                multi_scores[6].estimated_gb_per_second(),
            "memory bandwidth runs must report estimated logical byte throughput");
    for (std::size_t i = 9; i < names.size(); ++i) {
        require(settings->known_benchmarks.contains(names[i]) &&
                    single_scores[i].total_work_ == 408 &&
                    multi_scores[i].total_work_ == 408 &&
                    single_scores[i].work_per_second() && multi_scores[i].work_per_second() &&
                    single.get_run_configuration().at("weights").contains(names[i]) &&
                    single.get_run_configuration().at("baselineTimes").contains(names[i]),
                "arithmetic kernels must be selectable and report the same measured work in both modes");
    }
    require(single_scores[names.size() - 1].work_unit_ == "fused multiply-adds" &&
                multi_scores[names.size() - 1].work_unit_ == "fused multiply-adds",
            "FMA results must report fused operations as their work unit");
}

void test_cache_size_sweep() {
    const auto directory = std::filesystem::temp_directory_path() /
        ("cpubenchmark_cache_sweep_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    const auto profile_path = directory / "cache_profile.txt";
    {
        std::ofstream profile(profile_path);
        profile << "name=cache_test\nbenchmarks=cache_size_sweep\n"
                   "iterations=3\nintensity_multiplier=1\nnum_runs=2\n"
                   "cache_sweep_min_kib=4\ncache_sweep_max_kib=16\n"
                   "cache_sweep_hops=4096\n";
    }
    auto settings = std::make_shared<Settings>();
    settings->thread_count = 3;
    file_manager manager(settings);
    manager.load_profile(profile_path.string());
    require(settings->profile.benchmarks == std::vector<std::string>{"cache_size_sweep"} &&
                settings->cache_sweep_min_kib == 4 && settings->cache_sweep_max_kib == 16 &&
                settings->cache_sweep_hops == 4096,
            "cache sweep profile options must load");

    for (const auto [run_mode, workers] : {
             std::pair{mode::single_threaded, 1U},
             std::pair{mode::multi_threaded, 3U}}) {
        benchmark_report report;
        benchmark_coordinator coordinator(report, settings);
        coordinator.start_with_profile(settings->profile.benchmarks, run_mode);
        const auto& results = report.get_benchmark_scores();
        require(results.size() == 4 && report.get_combined_score() == 0.0,
                "cache sweep must produce three diagnostic points without affecting scoring");
        for (std::size_t i = 0; i < 3; ++i) {
            const auto& point = results[i];
            const std::uint64_t size_bytes = 4096ULL << i;
            require(point.benchmark_name_ == "cache_sweep_" + std::to_string(4U << i) + "KiB" &&
                        point.diagnostic_ && point.working_set_bytes_ == size_bytes &&
                        point.active_workers_ == workers && point.total_work_ == workers * 4096 &&
                        point.samples_.size() == 2 && point.nanoseconds_per_hop() &&
                        *point.nanoseconds_per_hop() > 0.0 &&
                        approximately_equal(*point.nanoseconds_per_hop(),
                            point.time_ * 1e9 * static_cast<double>(workers) /
                                static_cast<double>(point.total_work_)),
                    "each cache sweep point must record its size, work, samples, and effective latency");
        }
        require(report.get_run_configuration().at("cacheSweepMaxKiB") == 16 &&
                    report.get_run_configuration().at("cacheSweepHops") == 4096,
                "run configuration must identify the sweep range and hop count");
        settings->save_method = "json";
        const auto folder = directory / std::to_string(workers);
        manager.save_report(report, folder.string());
        const auto path = std::filesystem::directory_iterator(folder)->path();
        std::ifstream input(path);
        const auto data = nlohmann::json::parse(input);
        require(data.at("combinedScore").is_null() && data.at("scores").size() == 3 &&
                    data.at("scores").at(2).at("workingSetBytes") == 16384 &&
                    data.at("scores").at(2).at("score").is_null() &&
                    data.at("scores").at(2).at("effectiveNsPerHop").get<double>() > 0.0,
                "JSON reports must expose sweep sizes and effective latency as diagnostics");
        settings->save_method = "txt";
        const auto text_folder = directory / (std::to_string(workers) + "_text");
        manager.save_report(report, text_folder.string());
        std::ifstream text_input(std::filesystem::directory_iterator(text_folder)->path());
        std::ostringstream report_text;
        report_text << text_input.rdbuf();
        require(report_text.str().find("Working Set: 16384 bytes per worker") !=
                    std::string::npos &&
                    report_text.str().find("Effective Latency:") != std::string::npos &&
                    report_text.str().find("Score: N/A (diagnostic profile)") != std::string::npos,
                "text reports must display sweep working sets and effective latency");
    }
    settings->cache_sweep_min_kib = 3;
    bool rejected_invalid_size = false;
    try {
        benchmark_report invalid_report;
        benchmark_coordinator invalid_runner(invalid_report, settings);
        invalid_runner.start_with_profile(settings->profile.benchmarks, mode::single_threaded);
    } catch (const std::invalid_argument&) {
        rejected_invalid_size = true;
    }
    require(rejected_invalid_size, "cache sweeps must reject unsupported working-set sizes");
    scorer mixed;
    mixed.load_weights({{"alpha", 1.0}});
    mixed.load_baseline_times({{"alpha", 2.0}});
    mixed.add_score("alpha", 2.0);
    mixed.add_cache_sweep_point("cache_sweep_4KiB", {0.01, 0.02}, 4096, 4096, 1);
    require(approximately_equal(mixed.compute_final_score(), 2500.0),
            "cache sweep diagnostics must not change a scored benchmark's combined result");
    std::filesystem::remove_all(directory);
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        require(argc == 2, "expected a profile fixture path");
        test_scorer();
        test_run_samples_and_statistics();
        test_arithmetic_kernels();
        test_branch_patterns();
        test_profile(argv[1]);
        test_required_io_failures();
        test_report_combined_score();
        test_report_serialization();
        test_comparison_requires_matching_configuration();
        test_same_work_in_both_modes();
        test_cache_size_sweep();
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
