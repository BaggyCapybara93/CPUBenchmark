#pragma once

#include <string>
#include <thread>

enum class Mode{
    SingleThreaded,
    MultiThreaded,
    Both,
    Invalid
};

enum class ArgType{
    Help,
    Threads,
    Iterations,
    IntensityMultiplier,
    MatrixMultiplySize,
    Repeat,
    NumRuns,
    SingleThreaded,
    MultiThreaded,
    Both,
    Invalid
};

class ParseArguments{
    private:
        ArgType _argtype = ArgType::Invalid;
        Mode _mode = Mode::Invalid;
        bool _saveResults = true;
        int _threadCount = std::thread::hardware_concurrency();
        int _iterations = 999999;
        int _repeatCount = 1;
        int _matrixMultiplySize = 128;
        int _intensityMultiplier = 50;
        int _numRuns = 5;

        void parse(int argc, char* argv[]);
    public:
        //Constructor
        ParseArguments(int argc, char* argv[]);

        //Getters
        Mode getMode() const;
        int getThreadCount() const;
        int getIterations() const;
        int getRepeatCount() const;
        int getMatrixMultiplySize() const;
        int getIntensityMultiplier() const;
        bool shouldSaveResults() const;
        int getNumRuns() const;
        
        //Print
        void printUsage() const;

};