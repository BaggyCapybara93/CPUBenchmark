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
        ArgType argtype_ = ArgType::Invalid;
        Mode mode_ = Mode::Invalid;
        bool saveResults_ = true;
        int threadCount_ = std::thread::hardware_concurrency();
        int iterations_ = 999999;
        int repeatCount_ = 1;
        int matrixMultiplySize_ = 128;
        int intensityMultiplier_ = 50;
        int numRuns_ = 5;

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