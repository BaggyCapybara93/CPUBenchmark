#include "parseArguments.hpp"
#include <vector>
#include <iostream>

//Constructor
ParseArguments::ParseArguments(int argc, char* argv[]){
    parse(argc, argv);
}

void ParseArguments::parse(int argc, char* argv[]) {
    try {
        po::options_description desc("Allowed options");

        desc.add_options()
            ("help,h", "Show help message")

            ("singleThreaded,st", "Run in single-threaded mode")
            ("multiThreaded,mt", "Run in multi-threaded mode")

            ("threads,t", po::value<int>(&threadCount_), "Number of threads")
            ("iterations,i", po::value<int>(&iterations_), "Iterations per thread")
            ("intensityMultiplier,im", po::value<int>(&intensityMultiplier_), "Intensity multiplier")
            ("matrixMultiplySize,mm", po::value<int>(&matrixMultiplySize_), "Matrix multiply size")
            ("repeat,r", po::value<int>(&repeatCount_), "Repeat benchmark N times")
            ("numRuns,n", po::value<int>(&numRuns_), "Number of runs");
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);


        // Handle help
        if (vm.count("help")) {
            std::cout << desc << "\n";
            mode_ = Mode::None;
            return;
        }

        // Determine mode
        if (vm.count("singleThreaded"))
            mode_ = Mode::SingleThreaded;
        else if (vm.count("multiThreaded"))
            mode_ = Mode::MultiThreaded;
        else
            mode_ = Mode::Both; // default

    } catch(const std::exception& e){
        std::cerr << "Argument parsing error: " << e.what() << "\n";
    }
}

//Getters
Mode ParseArguments::getMode() const{return mode_;}
int ParseArguments::getThreadCount() const{return threadCount_;}
int ParseArguments::getIterations() const{return iterations_;}
int ParseArguments::getRepeatCount() const{return repeatCount_;}
int ParseArguments::getMatrixMultiplySize() const{return matrixMultiplySize_;}
int ParseArguments::getIntensityMultiplier() const{return intensityMultiplier_;}
bool ParseArguments::shouldSaveResults() const{return saveResults_;}
int ParseArguments::getNumRuns() const{return numRuns_;}