#include "parseArguments.hpp"
#include <vector>
#include <iostream>

//Constructor
ParseArguments::ParseArguments(int argc, char* argv[]){
    parse(argc, argv);
}

void ParseArguments::parse(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);

    static const std::unordered_map<std::string, ArgType> argMap = {
        {"--help", ArgType::Help},
        {"--h", ArgType::Help},
        {"--threads", ArgType::Threads},
        {"--t", ArgType::Threads},
        {"--iterations", ArgType::Iterations},
        {"--i", ArgType::Iterations},
        {"--intensityMultiplier", ArgType::IntensityMultiplier},
        {"--im", ArgType::IntensityMultiplier},
        {"--matrixMultiplySize", ArgType::MatrixMultiplySize},
        {"--mm", ArgType::MatrixMultiplySize},
        {"--repeat", ArgType::Repeat},
        {"--r", ArgType::Repeat},
        {"--numRuns", ArgType::NumRuns},
        {"--n", ArgType::NumRuns},
        {"--singleThreaded", ArgType::SingleThreaded},
        {"--st", ArgType::SingleThreaded},
        {"--multiThreaded", ArgType::MultiThreaded},
        {"--mt", ArgType::MultiThreaded}
    };

    if (args.empty()) {
        mode_ = Mode::Both;
        return;
    }

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];

        auto it = argMap.find(arg);
        if (it == argMap.end()) {
            std::cerr << "Warning: Unrecognized argument '" << arg << "'\n";
            return;
        }

        switch (it->second) {
            case ArgType::Help:
                printUsage();
                return;
            case ArgType::Threads:
                if (i + 1 >= args.size()) {
                    std::cerr << "Error: --threads option requires a value.\n";
                    return;
                }
                try {
                    threadCount_ = std::stoi(args[++i]);
                } catch (...) {
                    std::cerr << "Error parsing thread count.\n";
                    return;
                }
                break;
            case ArgType::Iterations:
                if (i + 1 >= args.size()) {
                    std::cerr << "Error: --iterations option requires a value.\n";
                    return;
                }
                try {
                    iterations_ = std::stoi(args[++i]);
                } catch (...) {
                    std::cerr << "Error parsing iterations.\n";
                    return;
                }
                break;
            case ArgType::IntensityMultiplier:
                if (i + 1 >= args.size()) {
                    std::cerr << "Error: --intensityMultiplier option requires a value.\n";
                    return;
                }
                try {
                    intensityMultiplier_ = std::stoi(args[++i]);
                } catch (...) {
                    std::cerr << "Error parsing intensity multiplier.\n";
                    return;
                }
                break;
            case ArgType::MatrixMultiplySize:
                if (i + 1 >= args.size()) {
                    std::cerr << "Error: --matrixMultiplySize option requires a value.\n";
                    return;
                }
                try {
                    matrixMultiplySize_ = std::stoi(args[++i]);
                } catch (...) {
                    std::cerr << "Error parsing matrix multiply size.\n";
                    return;
                }
                break;
            case ArgType::Repeat:
                if (i + 1 >= args.size()) {
                    std::cerr << "Error: --repeat option requires a value.\n";
                    return;
                }
                try {
                    repeatCount_ = std::stoi(args[++i]);
                } catch (...) {
                    std::cerr << "Error parsing repeat count.\n";
                    return;
                }
                break;
            case ArgType::NumRuns:
                if (i + 1 >= args.size()) {
                    std::cerr << "Error: --numRuns option requires a value.\n";
                    return;
                }
                try {
                    numRuns_ = std::stoi(args[++i]);
                } catch (...) {
                    std::cerr << "Error parsing numRuns.\n";
                    return;
                }
                break;
            case ArgType::SingleThreaded:
                mode_ = Mode::SingleThreaded;
                break;
            case ArgType::MultiThreaded:
                mode_ = Mode::MultiThreaded;
                break;
            default:
                break;
        }
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