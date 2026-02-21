#pragma once

#include <string>
#include <thread>

enum class CommandMode{
    Invalid, 
    RunMultiThreaded,
    RunSingleThreaded,
    RunBoth
};

class ParseArguments{
    private:
        CommandMode _mode = CommandMode::Invalid;
        bool _saveResults = true;
        int _threadCount = std::thread::hardware_concurrency();
        int _iterations = 999999;
        int _repeatCount = 1;
        int _matrixMultiplySize = 128;
        int _intensityMultiplier = 50;

        void parse(int argc, char* argv[]);
    public:
        //Constructor
        ParseArguments(int argc, char* argv[]);

        //Getters
        CommandMode getMode() const;
        int getThreadCount() const;
        int getIterations() const;
        int getRepeatCount() const;
        int getMatrixMultiplySize() const;
        int getIntensityMultiplier() const;
        bool shouldSaveResults() const;
        
        //Print
        void printUsage() const;

};