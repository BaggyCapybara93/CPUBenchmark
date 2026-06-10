#include "file_manager.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;


void file_manager::save_report(const benchmark_report& report, const std::string& save_folder) const{
    try{
        if(args_.get_save_method() == "json"){
            write_json(report, save_folder);
        }
        else if(args_.get_save_method() == "txt"){
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