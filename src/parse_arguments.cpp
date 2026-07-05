#include "parse_arguments.hpp"
#include <vector>
#include <iostream>

//Constructor
parse_arguments::parse_arguments(int argc, char* argv[]){
    parse(argc, argv);
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
            ("saveMethod,sm", po::value<std::string>(&save_method_), "Method used for saving scores(JSON/txt).");
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);


        // Handle help
        if (vm.count("help")) {
            std::cout << desc << "\n";
            mode_ = mode::none;
            return;
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