#pragma once

#include <string>
#include <thread>

enum class CommandMode{
    Invalid, 
    RunMultiThreaded,
    RunSingleThreaded
};

class ParseArguments{
    private:
        CommandMode mode_ = CommandMode::Invalid;
        bool saveResults_ = true;
        int threadCount_ = std::thread::hardware_concurrency();
        int iterations_ = 1000000;
        int repeatCount_ = 1;

        void parse(int argc, char* argv[]);
    public:
        //Constructor
        ParseArguments(int argc, char* argv[]);

        //Getters
        CommandMode getMode() const;
        int getThreadCount() const;
        int getIterations() const;
        int getRepeatCount() const;

        //Check compression
        bool shouldSaveResults() const;
        
        //Print
        void printUsage() const;

};