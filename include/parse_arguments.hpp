#pragma once

#include <string>
#include <boost/program_options.hpp>
#include <thread>

enum class Mode{
    SingleThreaded,
    MultiThreaded,
    Both,
    Invalid,
    None,
};

namespace po = boost::program_options;

class ParseArguments{
    private:
        Mode mode_ = Mode::Invalid;
        bool saveResults_ = true;
        std::string saveMethod_ = "txt"; //Make this into an enum when more methods are added
        int threadCount_ = static_cast<int>(std::thread::hardware_concurrency());
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
        const std::string getSaveMethod() const {return saveMethod_;}
        int getThreadCount() const;
        int getIterations() const;
        int getRepeatCount() const;
        int getMatrixMultiplySize() const;
        int getIntensityMultiplier() const;
        bool shouldSaveResults() const;
        int getNumRuns() const;

};

