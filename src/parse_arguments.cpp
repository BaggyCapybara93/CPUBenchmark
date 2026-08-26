#include "parse_arguments.hpp"
#include <vector>
#include <iostream>
#include <sstream>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include "file_manager.hpp"

std::string parse_arguments::trim_string(const std::string& value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }

    return value.substr(start, end - start);
}

void parse_arguments::parse_key_value_map(const std::string& input, std::unordered_map<std::string, double>& target) {
    std::stringstream stream(input);
    std::string entry;

    while (std::getline(stream, entry, ',')) {
        const std::size_t separator = entry.find('=');
        if (separator == std::string::npos) {
            throw std::invalid_argument("Expected entries in the form name=value");
        }

        const std::string key = trim_string(entry.substr(0, separator));
        const std::string value = trim_string(entry.substr(separator + 1));
        if (key.empty() || value.empty()) {
            throw std::invalid_argument("Empty key or value in scoring map");
        }

        target[key] = std::stod(value);
    }
}

void parse_arguments::parse(int argc, char* argv[]) {
    try {
        po::options_description desc("Allowed options");

        desc.add_options()
            ("help,h", "Show help message")

            ("singleThreaded,st", "Run in single-threaded mode")
            ("multiThreaded,mt", "Run in multi-threaded mode")

            ("threads,t", po::value<int>(&settings_.get()->thread_count), "Number of threads")
            ("iterations,i", po::value<int>(&settings_.get()->iterations), "Iterations per thread")
            ("intensityMultiplier,im", po::value<int>(&settings_.get()->intensity_multiplier), "Intensity multiplier")
            ("matrixMultiplySize,mm", po::value<int>(&settings_.get()->matrix_multiply_size), "Matrix multiply size")
            ("repeat,r", po::value<int>(&settings_.get()->repeat_count), "Repeat benchmark N times")
            ("numRuns,n", po::value<int>(&settings_.get()->num_runs), "Number of runs")
            ("saveMethod,sm", po::value<std::string>(&settings_.get()->save_method), "Method used for saving scores(JSON/txt).")
            ("weights,w", po::value<std::string>(), "Override scoring weights as comma-separated name=value pairs")
            ("baselineTimes,bt", po::value<std::string>(), "Override baseline times as comma-separated name=value pairs")
            ("profile,p", po::value<std::string>(), "Load a benchmark profile from a .txt file");
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);


        // Handle help
        if (vm.count("help")) {
            std::cout << desc << "\n";
            settings_.get()->mode = mode::none;
            return;
        }

        if (vm.count("profile")) {
            std::string profile_path = vm["profile"].as<std::string>();
            file_manager fm(settings_);
            fm.load_profile(profile_path);
        }

        if (vm.count("weights")) {
            std::unordered_map<std::string, double> parsed_weights;
            parse_key_value_map(vm["weights"].as<std::string>(), parsed_weights);
            for (const auto& [name, value] : parsed_weights) {
                settings_.get()->weights[name] = value;
            }
        }

        if (vm.count("baselineTimes")) {
            std::unordered_map<std::string, double> parsed_baselines;
            parse_key_value_map(vm["baselineTimes"].as<std::string>(), parsed_baselines);
            for (const auto& [name, value] : parsed_baselines) {
                settings_.get()->baseline_times[name] = value;
            }
        }

        const Settings& parsed_settings = *settings_;
        if (parsed_settings.thread_count <= 0 || parsed_settings.iterations <= 0 ||
            parsed_settings.intensity_multiplier <= 0 || parsed_settings.matrix_multiply_size <= 0 ||
            parsed_settings.repeat_count <= 0 || parsed_settings.num_runs <= 0) {
            throw std::invalid_argument("threads, iterations, repeat, numRuns, intensityMultiplier, and matrixMultiplySize must be positive");
        }
        if (!vm.count("singleThreaded") &&
            parsed_settings.iterations < parsed_settings.thread_count) {
            throw std::invalid_argument("iterations must be at least the thread count in multi-threaded mode");
        }
        double weight_sum = 0.0;
        for (const auto& [name, value] : parsed_settings.weights) {
            if (!std::isfinite(value) || value < 0.0) {
                throw std::invalid_argument("Weight for '" + name + "' must be finite and non-negative");
            }
            weight_sum += value;
        }
        if (!std::isfinite(weight_sum) || weight_sum <= 0.0) {
            throw std::invalid_argument("At least one scoring weight must be positive");
        }
        for (const auto& [name, value] : parsed_settings.baseline_times) {
            if (!std::isfinite(value) || value <= 0.0) {
                throw std::invalid_argument("Baseline time for '" + name + "' must be finite and positive");
            }
        }

        // Determine mode
        if (vm.count("singleThreaded"))
           settings_.get()->mode = mode::single_threaded;
        else if (vm.count("multiThreaded"))
            settings_.get()->mode = mode::multi_threaded;
        else
            settings_.get()->mode = mode::both; // default

    } catch(const std::exception& e){
        std::cerr << "Argument parsing error: " << e.what() << "\n";
        settings_->mode = mode::invalid;
    }
}
