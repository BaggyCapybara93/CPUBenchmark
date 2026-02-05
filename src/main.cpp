#include "benchmark.hpp"
#include "parseArguments.hpp"
#include "benchmarkReport.hpp"
#include <iostream>
#include <thread>
#include <vector>
void parseArguments(int argc, char* argv[], std::string& mode, int& iterations, int& repeatCount);

int main(int argc, char* argv[]){
    ParseArguments args(argc, argv);
    Benchmark benchmark;
    BenchmarkReport br("./scores");
    std::vector<double> scores;

    switch(args.getMode()){
        case CommandMode::RunSingleThreaded:{
            for(size_t i = 0; i < args.getRepeatCount(); i++){
                std::unordered_map<std::string, std::vector<double>> runScore = benchmark.runSingleThreadedBenchmark(args.getIterations());
                br.addScore(runScore);
            }

            for (const auto& score : br.getBenchmarkScores()) {
                double sum = 0.0;
                for (double val : score.second) {
                    sum += val;
                }
                double avg = score.second.empty() ? 0.0 : sum / score.second.size();
                std::cout << score.first << " (avg): " << avg << std::endl;
            }

            if(args.shouldSaveResults()){
                br.saveBenchmark();
            }
            return 1;
        }
        case CommandMode::RunMultiThreaded:{
            for(size_t i = 0; i < args.getRepeatCount(); i++){
                std::unordered_map<std::string, std::vector<double>> runScore = benchmark.runMultithreadedBenchmark(args.getThreadCount(), args.getIterations());
                br.addScore(runScore);
            }

            for (const auto& score : br.getBenchmarkScores()) {
                double sum = 0.0;
                for (double val : score.second) {
                    sum += val;
                }
                double avg = score.second.empty() ? 0.0 : sum / score.second.size();
                std::cout << score.first << " (avg): " << avg << std::endl;
            }

            if(args.shouldSaveResults()){
                br.saveBenchmark();
            }

            return 1;
        }

        case CommandMode::Invalid:{
            return 0;
        }

    }

    return 0;
}
