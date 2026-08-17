#include "file_manager.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;


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
        file << "CPU: " << report.get_cpu_name() << "\n";
        file << "RAM: " << report.get_ram_info() << "\n\n";

        for (const auto& s : report.get_benchmark_scores()) {
            file << s.benchmark_name_
                << " | Score: " << s.score_
                << " | Time: " << s.time_ << "s\n";
        }

        file.close();
        std::cout << "Benchmark saved to: " << fullPath << "\n";
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

    const std::string file_path = save_folder + "/" + get_timestamped_file("scores", "json");

    try{
        json j;

        j["saveFolder"] = save_folder;
        j["combinedScore"] = report.get_combined_score();
        j["cpuName"] = report.get_cpu_name();
        j["ramInfo"] = report.get_ram_info();

        // Serialize scores
        j["scores"] = json::array();
        for (const auto& s : report.get_benchmark_scores()) {
            j["scores"].push_back({
                {"benchmarkName", s.benchmark_name_},
                {"score", s.score_}
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

            std::istringstream iss(line);
            std::string key;
            std::string value;

            if (iss >> key) {
                if (key == "benchmarks") {
                    // Read comma-separated benchmark names
                    std::string benchmarks_str;
                    if (iss >> value) {
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
                    if (iss >> value) {
                        profile.iterations = std::stoi(value);
                    }
                } else if (key == "repeat_count") {
                    if (iss >> value) {
                        profile.repeat_count = std::stoi(value);
                    }
                } else if (key == "intensity_multiplier") {
                    if (iss >> value) {
                        profile.intensity_multiplier = std::stoi(value);
                    }
                } else if (key == "matrix_multiply_size") {
                    if (iss >> value) {
                        profile.matrix_multiply_size = std::stoi(value);
                    }
                } else if (key == "num_runs") {
                    if (iss >> value) {
                        profile.num_runs = std::stoi(value);
                    }
                } else if (key == "name") {
                    if (iss >> value) {
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