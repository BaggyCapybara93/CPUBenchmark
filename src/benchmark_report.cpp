#include "benchmark_report.hpp"
#include <string> 
#include <cstring> 
#include <iostream>
#include <fstream>
#include <map>
#include <numeric>
#include <algorithm>

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

// Constructor
benchmark_report::benchmark_report() {
    cpuname = collect_cpu_name();
    ram_info = collect_ram_info();
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
            if (s.benchmark_name_ == "Combined")
                continue; // skip the synthetic entry

            total += s.score_;
            count++;
        }

        if (count == 0)
            return 0.0;

        return total / count; 
    } catch (const std::exception& e) {
        std::cerr << "Error calculating combined score: " << e.what() << "\n";
        return 0.0;
    }
}

// Add scores (append)
void benchmark_report::add_score(const std::vector<score>& benchmarks) {
    try{
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
            double normalized = 0.0;
            if (!times.empty()) {
                normalized = 2500.0 * 0.020 / times[0]; // Simplified normalization
            }
            new_scores.emplace_back(name, normalized, times[0]);
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
    try {
        std::map<std::string, std::vector<double>> times_map;
        
        // Collect all times by benchmark name
        for (const auto& s : benchmark_scores_) {
            times_map[s.benchmark_name_].push_back(s.time_);
        }
        
        // Compute statistics for each benchmark
        for (auto& [name, times] : times_map) {
            if (times.empty()) continue;
            
            std::sort(times.begin(), times.end());
            
            double min_t = times.front();
            double max_t = times.back();
            
            // Standard deviation
double mean = std::accumulate(times.begin(), times.end(), 0.0) / static_cast<double>(times.size());
        double variance = 0.0;
        for (double t : times) {
            variance += (t - mean) * (t - mean);
        }
        double std_dev = std::sqrt(variance / static_cast<double>(times.size()));
            
            // P95 and P99
            double p95_idx = 0.95 * static_cast<double>(times.size() - 1);
            double p99_idx = 0.99 * static_cast<double>(times.size() - 1);
            
            size_t p95_idx_int = static_cast<size_t>(std::floor(p95_idx));
            size_t p99_idx_int = static_cast<size_t>(std::floor(p99_idx));
            
            double p95 = times[p95_idx_int];
            double p99 = times[p99_idx_int];
            
            // Update the first score entry
            auto it = std::find_if(benchmark_scores_.begin(), benchmark_scores_.end(),
                [&name](const score& s) { return s.benchmark_name_ == name; });
            if (it != benchmark_scores_.end()) {
                it->min_time_ = min_t;
                it->max_time_ = max_t;
                it->std_dev_ = std_dev;
                it->p95_ = p95;
                it->p99_ = p99;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error computing all statistics: " << e.what() << "\n";
    }
}

// Get statistics for a benchmark
benchmark_report::benchmark_stats benchmark_report::get_stats(const std::string& benchmark_name) const {
    benchmark_report::benchmark_stats stats = {0.0, 0.0, 0.0, 0.0, 0.0, 0};
    
    try {
        std::vector<double> times;
        for (const auto& s : benchmark_scores_) {
            if (s.benchmark_name_ == benchmark_name) {
                times.push_back(s.time_);
            }
        }
        
        stats.count = static_cast<int>(times.size());
        
        if (times.empty()) {
            return stats;
        }
        
        std::sort(times.begin(), times.end());
        
        stats.min_time = times.front();
        stats.max_time = times.back();
        
        double mean = std::accumulate(times.begin(), times.end(), 0.0) / static_cast<double>(times.size());
        double variance = 0.0;
        for (double t : times) {
            variance += (t - mean) * (t - mean);
        }
        stats.std_dev = std::sqrt(variance / static_cast<double>(times.size()));
        
        double p95_idx = 0.95 * static_cast<double>(times.size() - 1);
        double p99_idx = 0.99 * static_cast<double>(times.size() - 1);
        
        size_t p95_idx_int = static_cast<size_t>(std::floor(p95_idx));
        size_t p99_idx_int = static_cast<size_t>(std::floor(p99_idx));
        
        stats.p95 = times[p95_idx_int];
        stats.p99 = times[p99_idx_int];
    } catch (const std::exception& e) {
        std::cerr << "Error getting stats: " << e.what() << "\n";
    }
    
    return stats;
}