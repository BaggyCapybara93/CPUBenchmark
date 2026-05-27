#include "file_manager.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;


void FileManager::save_report(const BenchmarkReport& report, const std::string& save_folder) const{
    try{
        if(args_.getSaveMethod() == "json"){
            write_json(report, save_folder);
        }
        else if(args_.getSaveMethod() == "txt"){
            write_txt(report, save_folder);
        }
    } catch(const std::exception& e){
        std::cerr << "Failed to save report: " << e.what() << "\n";
    }
}

bool FileManager::ensure_directory(const std::string& path) const{
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

bool FileManager::write_txt(const BenchmarkReport& report, const std::string& save_folder) const{
    try {
        try{
            std::filesystem::create_directories(save_folder);
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error creating directory: " << e.what() << "\n";
            return false;
        }

        std::string filename = getTimestampedFile("benchmark_scores", ".txt");

        std::filesystem::path fullPath = std::filesystem::path(save_folder) / filename;

        std::ofstream file(fullPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << "\n";
            return false;
        }

        for (const auto& s : report.getBenchmarkScores()) {
            file << s.benchmarkName
                << " | Score: " << s.score
                << " | Time: " << s.time << "s\n";
        }

        file.close();
        std::cout << "Benchmark saved to: " << fullPath << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving benchmark: " << e.what() << "\n";
        return false;
    }
}

bool FileManager::write_json(const BenchmarkReport& report, const std::string& save_folder) const{
    if(!ensure_directory(save_folder)){
        return false;
    }

    const std::string filePath = save_folder + "/" + getTimestampedFile("scores", "json");

    try{
        json j;

        j["saveFolder"] = save_folder;
        j["combinedScore"] = report.getCombinedScore();

        // Serialize scores
        j["scores"] = json::array();
        for (const auto& s : report.getBenchmarkScores()) {
            j["scores"].push_back({
                {"benchmarkName", s.benchmarkName},
                {"score", s.score}
            });
        }

        std::ofstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Could not open file for writing: " << filePath << "\n";
            return false;
        }

        file << j.dump(4); // print with 4-space indent
        return true;

    }catch(const std::exception& e){
        std::cerr << "Error saving report: " << e.what() << "\n";
        return false;
    }
}