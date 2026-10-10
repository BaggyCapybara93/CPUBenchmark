#include "file_manager.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

#include <iomanip>


void file_manager::save_report(const benchmark_report& report, const std::string& save_folder) const{
    try{
        if(settings_.get()->save_method == "json"){
            write_json(report, save_folder);
        }
        else if(settings_.get()->save_method == "txt"){
            write_txt(report, save_folder);
        }
    } catch(const std::exception& e){
        std::cerr << "Failed to save report: " << e.what() << "\n";
    }
}

bool file_manager::ensure_directory(const std::string& path) const{
    try{
        if (!fs::exists(path)) {
            return fs::create_directories(path);
        }
        return true;
    }catch(const std::exception& e){
        std::cerr << "Directory creation failed: " << e.what() << "\n";
        return false;
    }
}

bool file_manager::write_txt(const benchmark_report& report, const std::string& save_folder) const{
    try {
        try{
            std::filesystem::create_directories(save_folder);
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error creating directory: " << e.what() << "\n";
            return false;
        }

        std::string filename = get_timestamped_file("benchmark_scores", ".txt");

        std::filesystem::path fullPath = std::filesystem::path(save_folder) / filename;

        std::ofstream file(fullPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << "\n";
            return false;
        }

        // Write CPU name and RAM at the top
        file << "==========================================\n";
        file << "CPU: " << report.get_cpu_name() << "\n";
        file << "RAM: " << report.get_ram_info() << "\n";
        file << "OS: " << report.get_os_info() << "\n";
        file << "Compiler: " << report.get_compiler_info() << "\n";
        file << "Logical CPU Cores: " << report.get_logical_cpu_cores() << "\n";
        file << "CPU Frequency: " << report.get_cpu_frequency_info() << "\n";
        file << "Active Benchmark Threads: " << report.get_active_thread_count() << "\n";
        file << "Measurement Version: " << benchmark_report::measurement_version << "\n";
        file << "==========================================\n\n";

        // Compute and write statistics for each benchmark
        for (const auto& s : report.get_benchmark_scores()) {
            if (s.benchmark_name_ == "Combined") continue;
            
            auto stats = report.get_stats(s.benchmark_name_);
            
            file << "------------------------------------------\n";
            file << s.benchmark_name_ << "\n";
            file << "------------------------------------------\n";
            file << "Score: " << std::fixed << std::setprecision(2) << s.score_ << "\n";
            file << "Time (mean): " << std::fixed << std::setprecision(6) << s.time_ << "s\n";
            file << "Min: " << stats.min_time << "s\n";
            file << "Max: " << stats.max_time << "s\n";
            file << "Std Dev: " << std::fixed << std::setprecision(6) << stats.std_dev << "s\n";
            file << "P95: " << stats.p95 << "s\n";
            file << "P99: " << stats.p99 << "s\n";
            file << "Sample Count: " << stats.count << "\n";
            file << "\n";
        }

        // Write combined score (no statistics - it's a synthetic aggregate)
        file << "==========================================\n";
        file << "COMBINED SCORE\n";
        file << "==========================================\n";
        file << "Score: " << std::fixed << std::setprecision(2) << report.get_combined_score() << "\n";
        file << "Note: Combined score is a weighted aggregate of individual benchmarks\n";

        file.close();
        std::cout << "Benchmark saved to: " << fullPath << "\n";
        
        // Compare with previous runs
        std::string mode_str;
        if (settings_.get()->mode == mode::single_threaded) {
            mode_str = "single_threaded";
        } else if (settings_.get()->mode == mode::multi_threaded) {
            mode_str = "multi_threaded";
        } else {
            mode_str = "both";
        }
        report.compare_with_previous(save_folder, mode_str, fullPath.string());
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving benchmark: " << e.what() << "\n";
        return false;
    }
}

bool file_manager::write_json(const benchmark_report& report, const std::string& save_folder) const{
    if(!ensure_directory(save_folder)){
        return false;
    }

    const std::string file_path = save_folder + "/" + get_timestamped_file("scores", ".json");

    try{
        json j;

        j["saveFolder"] = save_folder;
        j["combinedScore"] = report.get_combined_score();
        j["measurementVersion"] = benchmark_report::measurement_version;
        j["cpuName"] = report.get_cpu_name();
        j["ramInfo"] = report.get_ram_info();
        j["systemMetadata"] = {
            {"os", report.get_os_info()},
            {"compiler", report.get_compiler_info()},
            {"logicalCpuCores", report.get_logical_cpu_cores()},
            {"cpuFrequency", report.get_cpu_frequency_info()},
            {"activeBenchmarkThreads", report.get_active_thread_count()}
        };

        // Serialize scores with statistics
        j["scores"] = json::array();
        for (const auto& s : report.get_benchmark_scores()) {
            if (s.benchmark_name_ == "Combined") continue;
            auto stats = report.get_stats(s.benchmark_name_);
            
            j["scores"].push_back({
                {"benchmarkName", s.benchmark_name_},
                {"score", s.score_},
                {"time", s.time_},
                {"minTime", stats.min_time},
                {"maxTime", stats.max_time},
                {"stdDev", stats.std_dev},
                {"p95", stats.p95},
                {"p99", stats.p99},
                {"sampleCount", stats.count}
            });
        }

        std::ofstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Could not open file for writing: " << file_path << "\n";
            return false;
        }

        file << j.dump(4); // print with 4-space indent
        return true;

    }catch(const std::exception& e){
        std::cerr << "Error saving report: " << e.what() << "\n";
        return false;
    }
}

bool file_manager::read_profile_txt(const std::string& path, benchmark_profile& profile) const{
    try{
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open profile file: " << path << "\n";
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::string key;
            std::string value;
            const std::size_t separator = line.find('=');
            if (separator != std::string::npos) {
                key = line.substr(0, separator);
                value = line.substr(separator + 1);
            } else {
                std::istringstream iss(line);
                iss >> key >> value;
            }
            if (!key.empty()) {
                if (key == "benchmarks") {
                    // Read comma-separated benchmark names
                    std::string benchmarks_str;
                    if (!value.empty()) {
                        benchmarks_str = value;
                        std::stringstream ss(benchmarks_str);
                        std::string benchmark;
                        while (std::getline(ss, benchmark, ',')) {
                            std::string trimmed = benchmark;
                            // Trim whitespace
                            size_t start = trimmed.find_first_not_of(" \t");
                            size_t end = trimmed.find_last_not_of(" \t");
                            if (start != std::string::npos) {
                                trimmed = trimmed.substr(start, end - start + 1);
                                if (settings_.get()->known_benchmarks.contains(trimmed)) {
                                    profile.benchmarks.push_back(trimmed);
                                }
                            }
                        }
                    }
                } else if (key == "iterations") {
                    if (!value.empty()) {
                        profile.iterations = std::stoi(value);
                    }
                } else if (key == "repeat_count") {
                    if (!value.empty()) {
                        profile.repeat_count = std::stoi(value);
                    }
                } else if (key == "intensity_multiplier") {
                    if (!value.empty()) {
                        profile.intensity_multiplier = std::stoi(value);
                    }
                } else if (key == "matrix_multiply_size") {
                    if (!value.empty()) {
                        profile.matrix_multiply_size = std::stoi(value);
                    }
                } else if (key == "num_runs") {
                    if (!value.empty()) {
                        profile.num_runs = std::stoi(value);
                    }
                } else if (key == "name") {
                    if (!value.empty()) {
                        profile.name = value;
                    }
                }
            }
        }

        return true;
    }catch(const std::exception& e){
        std::cerr << "Error reading profile: " << e.what() << "\n";
        return false;
    }
}

bool file_manager::write_profile_txt(const benchmark_profile& profile, const std::string& save_folder) const{
    try{
        std::filesystem::create_directories(save_folder);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error creating directory: " << e.what() << "\n";
        return false;
    }

    std::string filename = get_timestamped_file("profile", ".txt");
    std::filesystem::path fullPath = std::filesystem::path(save_folder) / filename;

    std::ofstream file(fullPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << fullPath << "\n";
        return false;
    }

    file << "# Benchmark Profile\n";
    file << "# Name: " << profile.name << "\n\n";

    file << "name=" << profile.name << "\n";
    file << "benchmarks=";
    for (size_t i = 0; i < profile.benchmarks.size(); ++i) {
        if (i > 0) file << ",";
        file << " " << profile.benchmarks[i];
    }
    file << "\n";
    file << "iterations=" << profile.iterations << "\n";
    file << "repeat_count=" << profile.repeat_count << "\n";
    file << "intensity_multiplier=" << profile.intensity_multiplier << "\n";
    file << "matrix_multiply_size=" << profile.matrix_multiply_size << "\n";
    file << "num_runs=" << profile.num_runs << "\n";

    file.close();
    std::cout << "Profile saved to: " << fullPath << "\n";
    return true;
}

void file_manager::read_profile(const std::string& path){
    benchmark_profile temp_profile;
    if (read_profile_txt(path, temp_profile)) {
        // Apply profile settings to current settings
        settings_.get()->profile = temp_profile;
    }
}

void file_manager::load_profile(const std::string& profile_path){
    benchmark_profile temp_profile;
    if (read_profile_txt(profile_path, temp_profile)) {
        // Apply profile settings to current settings
        settings_.get()->profile = temp_profile;
        settings_.get()->iterations = temp_profile.iterations;
        settings_.get()->repeat_count = temp_profile.repeat_count;
        settings_.get()->intensity_multiplier = temp_profile.intensity_multiplier;
        settings_.get()->matrix_multiply_size = temp_profile.matrix_multiply_size;
        settings_.get()->num_runs = temp_profile.num_runs;
        std::cout << "Profile loaded: " << profile_path << "\n";
        std::cout << "  Profile Name: " << temp_profile.name << "\n";
        std::cout << "  Benchmarks: ";
        for (const auto& b : temp_profile.benchmarks) {
            std::cout << b << " ";
        }
        std::cout << "\n";
        std::cout << "  Iterations: " << temp_profile.iterations << "\n";
        std::cout << "  Repeat Count: " << temp_profile.repeat_count << "\n";
        std::cout << "  Intensity Multiplier: " << temp_profile.intensity_multiplier << "\n";
        std::cout << "  Matrix Multiply Size: " << temp_profile.matrix_multiply_size << "\n";
        std::cout << "  Num Runs: " << temp_profile.num_runs << "\n";
    } else {
        std::cerr << "Failed to load profile: " << profile_path << "\n";
    }
}

bool file_manager::save_profile(const benchmark_profile& profile, const std::string& save_folder) const{
    return write_profile_txt(profile, save_folder);
}
