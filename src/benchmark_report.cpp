#include "benchmark_report.hpp"
#include "timing_statistics.hpp"
#include <string> 
#include <cstdint>
#include <cstring> 
#include <iostream>
#include <fstream>
#include <map>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <thread>
#include <utility>
#include <unordered_set>
#include <functional>

#if defined(_WIN32)
#include <intrin.h>
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

std::string benchmark_report::collect_cpu_name() {
    std::string name;
    #if defined(_WIN32)
        int data[4] = {0};
        char brand[49] = {0};

        __cpuid(data, 0x80000002);
        memcpy(brand, data, 16);

        __cpuid(data, 0x80000003);
        memcpy(brand + 16, data, 16);

        __cpuid(data, 0x80000004);
        memcpy(brand + 32, data, 16);

        return std::string(brand);

    #elif defined(__x86_64__) || defined(__i386__)
        unsigned int eax, ebx, ecx, edx;
        char brand[49] = {0};

        __get_cpuid(0x80000002, &eax, &ebx, &ecx, &edx);
        memcpy(brand, &eax, 4); memcpy(brand+4, &ebx, 4);
        memcpy(brand+8, &ecx, 4); memcpy(brand+12, &edx, 4);

        __get_cpuid(0x80000003, &eax, &ebx, &ecx, &edx);
        memcpy(brand+16, &eax, 4); memcpy(brand+20, &ebx, 4);
        memcpy(brand+24, &ecx, 4); memcpy(brand+28, &edx, 4);

        __get_cpuid(0x80000004, &eax, &ebx, &ecx, &edx);
        memcpy(brand+32, &eax, 4); memcpy(brand+36, &ebx, 4);
        memcpy(brand+40, &ecx, 4); memcpy(brand+44, &edx, 4);

        return std::string(brand);

    #elif defined(__APPLE__)
        char buffer[256];
        size_t size = sizeof(buffer);

        // Intel macOS
        if (sysctlbyname("machdep.cpu.brand_string", &buffer, &size, NULL, 0) == 0)
            return std::string(buffer);

        // Apple Silicon (M1/M2/M3…)
        if (sysctlbyname("hw.model", &buffer, &size, NULL, 0) == 0)
            return std::string(buffer);

        return "Apple CPU";

    #elif defined(__linux__)
        FILE* fp = fopen("/proc/cpuinfo", "r");
        if (!fp) return "Unknown CPU";

        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "model name") || strstr(line, "Processor")) {
                fclose(fp);
                char* colon = strchr(line, ':');
                if (colon) {
                    std::string s = colon + 2;
                    // Trim trailing newline
                    if (!s.empty() && s.back() == '\n')
                        s.pop_back();
                    return s;
                }
            }
        }

        fclose(fp);
        return "Unknown CPU";

    #else
        return "Unknown CPU";
    #endif
}

// Get RAM size in GB
std::string benchmark_report::collect_ram_info() {
    std::string ram_info = "Unknown RAM";
    
    #if defined(__APPLE__)
        int64_t memory_size = 0;
        size_t size = sizeof(memory_size);
        
        if (sysctlbyname("hw.memsize", &memory_size, &size, NULL, 0) == 0) {
            // Convert bytes to GB
            double ram_gb = static_cast<double>(memory_size) / (1024.0 * 1024.0 * 1024.0);
            ram_info = std::to_string(static_cast<long long>(ram_gb)) + " GB";
        }
    
    #elif defined(__linux__)
        std::ifstream meminfo("/proc/meminfo");
        if (meminfo.is_open()) {
            std::string line;
            while (std::getline(meminfo, line)) {
                if (line.find("MemTotal:") == 0) {
                    // Parse the value (format: "MemTotal:       16384320 kB")
                    std::istringstream iss(line);
                    std::string label, value_unit;
                    iss >> label >> value_unit;
                    
                    long long kb = std::stoll(value_unit);
                    double mb = static_cast<double>(kb) / 1024.0;
                    double gb = mb / 1024.0;
                    
                    ram_info = std::to_string(static_cast<long long>(gb)) + " GB";
                    break;
                }
            }
            meminfo.close();
        }
    #elif defined(_WIN32)
        SIZE_T physical_memory_kib = GetPhysicallyInstalledSystemMemory();
        
        // Convert KB to GB
        double ram_gb = static_cast<double>(physical_memory_kib) / (1024.0 * 1024.0);
        ram_info = std::to_string(static_cast<long long>(ram_gb)) + " GB";
    #else
        ram_info = "Unknown RAM";
    #endif
    
    return ram_info;
}

std::string benchmark_report::collect_os_info() {
#if defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#else
    return "Unknown OS";
#endif
}

std::string benchmark_report::collect_compiler_info() {
#if defined(__clang__)
    return "Clang " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__);
#elif defined(__GNUC__)
    return "GCC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__);
#elif defined(_MSC_VER)
    return "MSVC " + std::to_string(_MSC_VER);
#else
    return "Unknown compiler";
#endif
}

std::string benchmark_report::collect_cpu_frequency_info() {
#if defined(__APPLE__)
    std::uint64_t frequency_hz = 0;
    size_t size = sizeof(frequency_hz);
    if (sysctlbyname("hw.cpufrequency", &frequency_hz, &size, nullptr, 0) == 0 && frequency_hz > 0) {
        const double frequency_ghz = static_cast<double>(frequency_hz) / 1'000'000'000.0;
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2) << frequency_ghz << " GHz";
        return stream.str();
    }
#elif defined(__linux__)
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("cpu MHz") == 0) {
            const std::size_t separator = line.find(':');
            if (separator != std::string::npos) {
                try {
                    const double frequency_mhz = std::stod(line.substr(separator + 1));
                    std::ostringstream stream;
                    stream << std::fixed << std::setprecision(2) << frequency_mhz / 1000.0 << " GHz";
                    return stream.str();
                } catch (const std::exception&) {
                    break;
                }
            }
        }
    }
#endif
    return "Unavailable";
}

// Constructor
benchmark_report::benchmark_report() {
    cpuname = collect_cpu_name();
    ram_info = collect_ram_info();
    os_info = collect_os_info();
    compiler_info = collect_compiler_info();
    cpu_frequency_info = collect_cpu_frequency_info();
    logical_cpu_cores_ = std::thread::hardware_concurrency();
}

void benchmark_report::set_run_configuration(
    const Settings& settings, const std::vector<std::string>& benchmark_names,
    int active_thread_count) {
    active_thread_count_ = active_thread_count;
    run_configuration_ = {
        {"measurementVersion", measurement_version},
        {"activeThreads", active_thread_count},
        {"logicalShards", std::min(settings.thread_count, settings.iterations)},
        {"iterations", settings.iterations},
        {"intensityMultiplier", settings.intensity_multiplier},
        {"matrixMultiplySize", settings.matrix_multiply_size},
        {"numRuns", settings.num_runs},
        {"repeatCount", settings.repeat_count},
        {"profileName", settings.profile.benchmarks.empty() ? "full_suite" : settings.profile.name},
        {"os", os_info},
        {"benchmarks", benchmark_names},
        {"weights", settings.weights},
        {"baselineTimes", settings.baseline_times},
        {"build", {
            {"type", CPUBENCHMARK_BUILD_TYPE},
            {"compilerPath", CPUBENCHMARK_COMPILER_PATH},
            {"compilerId", CPUBENCHMARK_COMPILER_ID},
            {"compilerVersion", CPUBENCHMARK_COMPILER_VERSION},
            {"commonFlags", CPUBENCHMARK_COMMON_FLAGS},
            {"debugFlags", CPUBENCHMARK_DEBUG_FLAGS},
            {"releaseFlags", CPUBENCHMARK_RELEASE_FLAGS},
            {"relWithDebInfoFlags", CPUBENCHMARK_RELWITHDEBINFO_FLAGS},
            {"minSizeRelFlags", CPUBENCHMARK_MINSIZEREL_FLAGS},
            {"cppStandard", 20}
        }}
    };
}

// Getters
const std::vector<score>& benchmark_report::get_benchmark_scores() const {
    return benchmark_scores_;
}

double benchmark_report::get_combined_score() const {
    try{
        double total = 0.0;
        int count = 0;

        for (const auto& s : benchmark_scores_) {
            if (s.benchmark_name_ == "Combined") {
                total += s.score_;
                ++count;
            }
        }

        if (count == 0)
        {
            for (const auto& s : benchmark_scores_) {
                if (s.benchmark_name_ != "Combined") {
                    total += s.score_;
                    ++count;
                }
            }
        }

        if (count == 0)
            return 0.0;

        return total / static_cast<double>(count);
    } catch (const std::exception& e) {
        std::cerr << "Error calculating combined score: " << e.what() << "\n";
        return 0.0;
    }
}

// Add scores (append)
void benchmark_report::add_score(const std::vector<score>& benchmarks) {
    try{
        if (!run_configuration_.is_object()) {
            run_configuration_ = nlohmann::json::object();
        }
        for (const auto& result : benchmarks) {
            if (result.benchmark_name_ != "Combined" && !result.work_unit_.empty()) {
                run_configuration_["workloads"][result.benchmark_name_] = {
                    {"totalWork", result.total_work_}, {"workUnit", result.work_unit_}
                };
            }
        }
        benchmark_scores_.insert(
            benchmark_scores_.end(),
            benchmarks.begin(),
            benchmarks.end()
        );
    } catch (const std::exception& e) {
        std::cerr << "Error adding scores: " << e.what() << "\n";   
    }
}

// Add scores with statistics
void benchmark_report::add_scores_with_stats(const std::vector<std::pair<std::string, double>>& benchmark_times) {
    try {
        std::vector<score> new_scores;
        
        // Group times by benchmark name
        std::map<std::string, std::vector<double>> times_map;
        for (const auto& pt : benchmark_times) {
            times_map[pt.first].push_back(pt.second);
        }
        
        // Add scores
        for (auto& [name, times] : times_map) {
            const timing_statistics stats = calculate_timing_statistics(times);
            const double normalized = 2500.0 * 0.020 / stats.mean; // Simplified normalization
            new_scores.emplace_back(name, normalized, stats.mean, std::move(times));
        }
        
        benchmark_scores_.insert(
            benchmark_scores_.end(),
            new_scores.begin(),
            new_scores.end()
        );
    } catch (const std::exception& e) {
        std::cerr << "Error adding scores with stats: " << e.what() << "\n";
    }
}

// Compute statistics for all benchmarks
void benchmark_report::compute_all_statistics() {
    for (auto& result : benchmark_scores_) {
        if (result.benchmark_name_ == "Combined") {
            continue;
        }
        const timing_statistics stats = calculate_timing_statistics(result.samples_);
        result.min_time_ = stats.min;
        result.max_time_ = stats.max;
        result.std_dev_ = stats.std_dev;
        result.median_time_ = stats.median;
        result.p95_ = stats.p95;
        result.p99_ = stats.p99;
    }
}

// Get statistics for a benchmark
benchmark_report::benchmark_stats benchmark_report::get_stats(const std::string& benchmark_name) const {
    std::vector<double> times;
    for (const auto& result : benchmark_scores_) {
        if (result.benchmark_name_ == benchmark_name) {
            times.insert(times.end(), result.samples_.begin(), result.samples_.end());
        }
    }
    const timing_statistics stats = calculate_timing_statistics(std::move(times));
    return {stats.min, stats.max, stats.std_dev, stats.median, stats.p95, stats.p99,
            static_cast<int>(stats.count)};
}

benchmark_report::benchmark_stats benchmark_report::get_stats(const score& result) {
    const timing_statistics stats = calculate_timing_statistics(result.samples_);
    return {stats.min, stats.max, stats.std_dev, stats.median, stats.p95, stats.p99,
            static_cast<int>(stats.count)};
}

// Check if CPU and RAM are similar between runs
bool benchmark_report::are_systems_similar(const std::string& cpu1, const std::string& ram1, 
                                            const std::string& cpu2, const std::string& ram2,
                                            double similarity_threshold) {
    // Simple string comparison for CPU (exact match or substring match)
    bool cpu_similar = (cpu1 == cpu2) || (cpu1.find(cpu2) != std::string::npos) || 
                       (cpu2.find(cpu1) != std::string::npos);
    
    // For RAM, extract numeric value and compare
    bool ram_similar = false;
    if (ram1.empty() || ram2.empty()) {
        ram_similar = false;
    } else {
        // Extract numeric part from RAM strings like "8 GB" or "16 GB"
        auto extract_ram_value = [](const std::string& ram_str) -> double {
            size_t pos = ram_str.find(' ');
            if (pos != std::string::npos) {
                try {
                    return std::stod(ram_str.substr(0, pos));
                } catch (...) {
                    return -1.0;
                }
            }
            try {
                return std::stod(ram_str);
            } catch (...) {
                return -1.0;
            }
        };
        
        double ram1_val = extract_ram_value(ram1);
        double ram2_val = extract_ram_value(ram2);
        
        if (ram1_val > 0 && ram2_val > 0) {
            double ratio = std::min(ram1_val, ram2_val) / std::max(ram1_val, ram2_val);
            ram_similar = (ratio >= similarity_threshold);
        } else {
            ram_similar = false;
        }
    }
    
    return cpu_similar && ram_similar;
}

// Compare with the newest run that used the same workload and build configuration.
bool benchmark_report::compare_with_previous(const std::string& save_folder, const std::string& mode_type,
                                             const std::string& current_report_file) const {
    if (!run_configuration_.is_object()) {
        return false;
    }

    struct saved_run {
        std::string cpu;
        std::string ram;
        std::string configuration;
        std::map<std::string, std::vector<double>> times;
        std::map<std::string, std::vector<double>> scores;
    };

    std::map<std::string, std::vector<double>> current_times;
    std::map<std::string, std::vector<double>> current_scores;
    std::unordered_set<std::string> benchmark_names;
    for (const auto& result : benchmark_scores_) {
        if (result.benchmark_name_ != "Combined") {
            benchmark_names.insert(result.benchmark_name_);
            current_times[result.benchmark_name_].push_back(result.time_);
            current_scores[result.benchmark_name_].push_back(result.score_);
        }
    }

    const auto parse_file = [&](const std::string& path, saved_run& run) {
        std::ifstream input(path);
        if (!input) {
            return false;
        }
        std::string line;
        std::string current_benchmark;
        try {
            while (std::getline(input, line)) {
                if (line.starts_with("CPU: ")) {
                    run.cpu = line.substr(5);
                } else if (line.starts_with("RAM: ")) {
                    run.ram = line.substr(5);
                } else if (line.starts_with("Run Configuration: ")) {
                    run.configuration = line.substr(std::string("Run Configuration: ").size());
                } else if (line == "COMBINED SCORE") {
                    current_benchmark.clear();
                } else if (benchmark_names.contains(line)) {
                    current_benchmark = line;
                } else if (line.starts_with("Score: ") && !current_benchmark.empty()) {
                    run.scores[current_benchmark].push_back(std::stod(line.substr(7)));
                } else if (line.starts_with("Time (mean): ") && !current_benchmark.empty()) {
                    run.times[current_benchmark].push_back(std::stod(line.substr(13)));
                }
            }
        } catch (const std::exception&) {
            return false;
        }
        return true;
    };

    try {
        std::vector<std::string> previous_files;
        if (std::filesystem::exists(save_folder)) {
            for (const auto& entry : std::filesystem::directory_iterator(save_folder)) {
                if (!entry.is_regular_file()) {
                    continue;
                }
                std::error_code ec;
                if (std::filesystem::equivalent(entry.path(), current_report_file, ec) && !ec) {
                    continue;
                }
                const std::string filename = entry.path().filename().string();
                if (filename.starts_with("benchmark_scores_") && entry.path().extension() == ".txt") {
                    previous_files.push_back(entry.path().string());
                }
            }
        }
        std::sort(previous_files.begin(), previous_files.end(), std::greater<>());

        saved_run previous;
        std::string previous_path;
        const std::string current_configuration = run_configuration_.dump();
        for (const auto& path : previous_files) {
            saved_run candidate;
            if (!parse_file(path, candidate) || candidate.configuration != current_configuration ||
                candidate.cpu != cpuname || candidate.ram != ram_info) {
                continue;
            }
            bool complete = true;
            for (const auto& name : benchmark_names) {
                if (candidate.times[name].size() != current_times.at(name).size() ||
                    candidate.scores[name].size() != current_scores.at(name).size() ||
                    std::any_of(candidate.times[name].begin(), candidate.times[name].end(),
                        [](double value) { return !std::isfinite(value) || value <= 0.0; }) ||
                    std::any_of(candidate.scores[name].begin(), candidate.scores[name].end(),
                        [](double value) { return !std::isfinite(value) || value <= 0.0; })) {
                    complete = false;
                    break;
                }
            }
            if (complete) {
                previous = std::move(candidate);
                previous_path = path;
                break;
            }
        }

        if (previous_path.empty()) {
            std::cout << "No comparable previous run for " << mode_type
                      << " (workload, threads, profile, build, or hardware differs).\n";
            return false;
        }

        const auto average = [](const std::vector<double>& values) {
            return std::accumulate(values.begin(), values.end(), 0.0) /
                   static_cast<double>(values.size());
        };
        const auto format = [](double value, int precision) {
            std::ostringstream output;
            output << std::fixed << std::setprecision(precision) << value;
            return output.str();
        };

        std::cout << "\nComparison with " << previous_path << " (" << mode_type << ")\n";
        std::cout << "Benchmark | Previous (s) | Current (s) | Improvement\n";
        for (const auto& [name, values] : current_times) {
            const double before = average(previous.times.at(name));
            const double now = average(values);
            const double improvement = (before - now) / before * 100.0;
            std::cout << name << " | " << format(before, 9) << " | " << format(now, 9)
                      << " | " << (improvement >= 0.0 ? "+" : "") << format(improvement, 2) << "%\n";
        }
        std::cout << "Score change\n";
        for (const auto& [name, values] : current_scores) {
            const double before = average(previous.scores.at(name));
            const double now = average(values);
            const double change = (now - before) / before * 100.0;
            std::cout << name << " | " << format(before, 2) << " | " << format(now, 2)
                      << " | " << (change >= 0.0 ? "+" : "") << format(change, 2) << "%\n";
        }
        return true;
    } catch (const std::exception& error) {
        std::cerr << "Error during comparison: " << error.what() << "\n";
        return false;
    }
}
