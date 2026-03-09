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
        {"--singleThreaded", ArgType::SingleThreaded},
        {"--st", ArgType::SingleThreaded},
        {"--multiThreaded", ArgType::MultiThreaded},
        {"--mt", ArgType::MultiThreaded}
    };

    if (args.empty()) {
        _mode = Mode::Both;
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
                    _threadCount = std::stoi(args[++i]);
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
                    _iterations = std::stoi(args[++i]);
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
                    _intensityMultiplier = std::stoi(args[++i]);
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
                    _matrixMultiplySize = std::stoi(args[++i]);
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
                    _repeatCount = std::stoi(args[++i]);
                } catch (...) {
                    std::cerr << "Error parsing repeat count.\n";
                    return;
                }
                break;
            case ArgType::SingleThreaded:
                _mode = Mode::SingleThreaded;
                break;
            case ArgType::MultiThreaded:
                _mode = Mode::MultiThreaded;
                break;
            default:
                break;
        }
    }
}

//Getters
Mode ParseArguments::getMode() const{return _mode;}
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