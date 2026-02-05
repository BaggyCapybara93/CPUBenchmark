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
        mode_ = CommandMode::Invalid;
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
                threadCount_ = std::stoi(args[++i]);
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
                iterations_ = std::stoi(args[++i]);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing iterations: " << e.what() << '\n';
                return;
            }
        }
        else if(arg == "--repeat" || arg == "--r"){
            if(i + 1 >= args.size()) {
                std::cerr << "Error: --repeat option requires a value.\n";
                return;
            }
            try{
                repeatCount_ = std::stoi(args[++i]);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing repeat count: " << e.what() << '\n';
            }
        }
        else if(arg == "--singleThreaded" || arg == "--st"){
            mode_ = CommandMode::RunSingleThreaded;
        }
        else if(arg == "--multiThreaded" || arg == "--mt"){
            mode_ = CommandMode::RunMultiThreaded;
        }else{
            std::cerr << "Warning: Unrecognized argument '" << arg << "'\n";
            return;
        }
    } 

}

//Getters
CommandMode ParseArguments::getMode() const{return mode_;}
int ParseArguments::getThreadCount() const{return threadCount_;}
int ParseArguments::getIterations() const{return iterations_;}
int ParseArguments::getRepeatCount() const{return repeatCount_;}

//Check compression
bool ParseArguments::shouldSaveResults() const{return saveResults_;}
        
//Print
void ParseArguments::printUsage() const {
    std::cout << "Usage:\n"
              << "  --singleThreaded | --st      Run in single-threaded mode\n"
              << "  --multiThreaded  | --mt      Run in multi-threaded mode\n"
              << "  --threads <N>    | --t <N>   Number of threads to use\n"
              << "  --iterations <N> | --i <N>   Iterations per thread\n"
              << "  --repeat <N>     | --r <N>   Repeat benchmark N times\n"
              << "  --help           | --h       Show this help message\n";
}