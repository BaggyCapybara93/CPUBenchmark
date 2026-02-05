#include "parseArguments.hpp"
#include <vector>
#include <iostream>

//Constructor
ParseArguments::ParseArguments(int argc, char* argv[]){
    parse(argc, argv);
}

void ParseArguments::parse(int argc, char* argv[]){
    std::vector<std::string> args(argv + 1, argv + argc);
    
    if(args.empty()){
        _mode = CommandMode::Invalid;
        return;
    }

    for(size_t i = 0; i < args.size(); ++i){
        const std::string& arg = args[i];

        if(arg == "--help" || arg == "--h"){
            printUsage();
            return;
        }
        else if(arg == "--threads" || arg == "--t"){
            if(i + 1 >= args.size()) {
                std::cerr << "Error: --threads option requires a value.\n";
                return;
            }
            try{
                _threadCount = std::stoi(args[++i]);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing thread count: " << e.what() << '\n';
                return;
            }
        }
        else if(arg == "--iterations" || arg == "--i"){
            if(i + 1 >= args.size()) {
                std::cerr << "Error: --iterations option requires a value.\n";
                return;
            }
            try{
                _iterations = std::stoi(args[++i]);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing iterations: " << e.what() << '\n';
                return;
            }
        }
        else if(arg == "--intensityMultiplier" || arg == "--im"){
            if(i + 1 >= args.size()) {
                std::cerr << "Error: --intensityMultiplier option requires a value.\n";
                return;
            }
            try{
                _intensityMultiplier = std::stoi(args[++i]);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing intensityMultiplier: " << e.what() << '\n';
                return;
            }
        }
        else if(arg == "--matrixMultiplySize" || arg == "--mm"){
            if(i + 1 >= args.size()) {
                std::cerr << "Error: --matrixMultiplySize option requires a value.\n";
                return;
            }
            try{
                _matrixMultiplySize = std::stoi(args[++i]);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing matrixMultiplySize: " << e.what() << '\n';
                return;
            }
        }
        else if(arg == "--repeat" || arg == "--r"){
            if(i + 1 >= args.size()) {
                std::cerr << "Error: --repeat option requires a value.\n";
                return;
            }
            try{
                _repeatCount = std::stoi(args[++i]);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing repeat count: " << e.what() << '\n';
            }
        }
        else if(arg == "--singleThreaded" || arg == "--st"){
            _mode = CommandMode::RunSingleThreaded;
        }
        else if(arg == "--multiThreaded" || arg == "--mt"){
            _mode = CommandMode::RunMultiThreaded;
        }
        else{
            std::cerr << "Warning: Unrecognized argument '" << arg << "'\n";
            return;
        }
    } 

}

//Getters
CommandMode ParseArguments::getMode() const{return _mode;}
int ParseArguments::getThreadCount() const{return _threadCount;}
int ParseArguments::getIterations() const{return _iterations;}
int ParseArguments::getRepeatCount() const{return _repeatCount;}
int ParseArguments::getMatrixMultiplySize() const{return _matrixMultiplySize;}
int ParseArguments::getIntensityMultiplier() const{return _intensityMultiplier;}
bool ParseArguments::shouldSaveResults() const{return _saveResults;}
        
//Print
void ParseArguments::printUsage() const {
    std::cout << "Usage:\n"
              << "  --singleThreaded | --st      Run in single-threaded mode\n"
              << "  --multiThreaded  | --mt      Run in multi-threaded mode\n"
              << "  --threads <N>    | --t <N>   Number of threads to use\n"
              << "  --iterations <N> | --i <N>   Iterations per thread\n"
              << "  --intensityMultiplier <N> | --im <N>   Intensity Multiplier\n"
              << "  --matrixMultiplySize <N> | --mm <N>   Size used for martix multiply benchmark\n"
              << "  --repeat <N>     | --r <N>   Repeat benchmark N times\n"
              << "  --help           | --h       Show this help message\n";
}