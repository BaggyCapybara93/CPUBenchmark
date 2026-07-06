#include "parse_arguments.hpp"
#include <vector>
#include <iostream>
#include <sstream>
#include <cctype>
#include <stdexcept>

//Constructor
parse_arguments::parse_arguments(int argc, char* argv[]){
    parse(argc, argv);
}

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

            ("threads,t", po::value<int>(&thread_count_), "Number of threads")
            ("iterations,i", po::value<int>(&iterations_), "Iterations per thread")
            ("intensityMultiplier,im", po::value<int>(&intensity_multiplier_), "Intensity multiplier")
            ("matrixMultiplySize,mm", po::value<int>(&matrix_multiply_size_), "Matrix multiply size")
            ("repeat,r", po::value<int>(&repeat_count_), "Repeat benchmark N times")
            ("numRuns,n", po::value<int>(&num_runs_), "Number of runs")
            ("saveMethod,sm", po::value<std::string>(&save_method_), "Method used for saving scores(JSON/txt).")
            ("weights,w", po::value<std::string>(), "Override scoring weights as comma-separated name=value pairs")
            ("baselineTimes,bt", po::value<std::string>(), "Override baseline times as comma-separated name=value pairs");
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);


        // Handle help
        if (vm.count("help")) {
            std::cout << desc << "\n";
            mode_ = mode::none;
            return;
        }

        if (vm.count("weights")) {
            std::unordered_map<std::string, double> parsed_weights;
            parse_key_value_map(vm["weights"].as<std::string>(), parsed_weights);
            for (const auto& [name, value] : parsed_weights) {
                weights_[name] = value;
            }
        }

        if (vm.count("baselineTimes")) {
            std::unordered_map<std::string, double> parsed_baselines;
            parse_key_value_map(vm["baselineTimes"].as<std::string>(), parsed_baselines);
            for (const auto& [name, value] : parsed_baselines) {
                baseline_times_[name] = value;
            }
        }

        // Determine mode
        if (vm.count("singleThreaded"))
            mode_ = mode::single_threaded;
        else if (vm.count("multiThreaded"))
            mode_ = mode::multi_threaded;
        else
            mode_ = mode::both; // default

    } catch(const std::exception& e){
        std::cerr << "Argument parsing error: " << e.what() << "\n";
    }
}