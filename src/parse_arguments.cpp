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

//Getters
mode parse_arguments::get_mode() const{return mode_;}
int parse_arguments::get_thread_count() const{return thread_count_;}
int parse_arguments::get_iterations() const{return iterations_;}
int parse_arguments::get_repeat_count() const{return repeat_count_;}
int parse_arguments::get_matrix_multiply_size() const{return matrix_multiply_size_;}
int parse_arguments::get_intensity_multiplier() const{return intensity_multiplier_;}
bool parse_arguments::should_save_results() const{return save_results_;}
int parse_arguments::get_num_runs() const{return num_runs_;}