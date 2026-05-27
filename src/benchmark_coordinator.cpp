#include "benchmark_coordinator.hpp"
#include <iostream>

//Threading
void BenchmarkThreadPool::runAll(const std::function<void()>& fn) {
    currentTask_ = fn;

    startBarrier_.arrive_and_wait();

    doneBarrier_.arrive_and_wait();
}

void BenchmarkThreadPool::workerLoop() {
    while (!stop_) {
        startBarrier_.arrive_and_wait();
        if (stop_) break;

        currentTask_();

        doneBarrier_.arrive_and_wait();
    }
}



void BenchmarkCoordinator::runBenchmark(RunnerFunction runner)
{
    std::vector<Score> scores = runner(args_, *this);
    report_.addScore(scores);
}

void BenchmarkCoordinator::run(const Mode& mode)
{
    switch (mode)
    {
        case Mode::SingleThreaded:
        {
            runBenchmark([&](const ParseArguments& a, BenchmarkCoordinator& b) {
                return b.runSingleThreadedBenchmark(
                    a.getIterations(),
                    a.getIntensityMultiplier(),
                    a.getMatrixMultiplySize(),
                    a.getNumRuns()
                );
            });
            break;
        }

        case Mode::MultiThreaded:
        {
            runBenchmark([&](const ParseArguments& a, BenchmarkCoordinator& b) {
                int iterationsPerThread = a.getIterations() / a.getThreadCount();
                return b.runMultithreadedBenchmark(
                    a.getThreadCount(),
                    iterationsPerThread,
                    a.getIntensityMultiplier(),
                    a.getMatrixMultiplySize(),
                    a.getNumRuns()
                );
            });
            break;
        }

        case Mode::Both:
        default:
        {
            // Run single-threaded
            runBenchmark([&](const ParseArguments& a, BenchmarkCoordinator& b) {
                return b.runSingleThreadedBenchmark(
                    a.getIterations(),
                    a.getIntensityMultiplier(),
                    a.getMatrixMultiplySize(),
                    a.getNumRuns()
                );
            });

            // Run multi-threaded
            runBenchmark([&](const ParseArguments& a, BenchmarkCoordinator& b) {
                int iterationsPerThread = a.getIterations() / a.getThreadCount();
                return b.runMultithreadedBenchmark(
                    a.getThreadCount(),
                    iterationsPerThread,
                    a.getIntensityMultiplier(),
                    a.getMatrixMultiplySize(),
                    a.getNumRuns()
                );
            });

            break;
        }
    }
}

std::vector<Score>  BenchmarkCoordinator::runMultithreadedBenchmark(const int numThreads, int iterationsPerThread, const int intensityMultiplier, const int matrixMultiplySize, const int numRuns) {
    Scorer scorer;
    iterationsPerThread *= intensityMultiplier;

    BenchmarkThreadPool pool(numThreads);

    // Run floating-point benchmark first
    auto floatRunner = [&]() {
        pool.runAll([&]() {
            Benchmark::floatingPointBenchmark(iterationsPerThread);
        });
    };

    double elapsedFloat = Benchmark::runBenchmark(floatRunner, numRuns);
    std::cout << "Floating-point benchmark completed in " << elapsedFloat << " seconds.\n";
    scorer.addScore("FloatingPoint", elapsedFloat, elapsedFloat);

    // Integer arithmetic benchmark 
    auto intRunner = [&]() {
        pool.runAll([&]() {
            Benchmark::integerArithmeticBenchmark(iterationsPerThread);
        });
    };

    double elapsedInt = Benchmark::runBenchmark(intRunner, numRuns);
    std::cout << "Integer arithmetic benchmark completed in " << elapsedInt << " seconds.\n";
    scorer.addScore("Integer", elapsedInt, elapsedInt);

    //Matrix Multiplication Benchmark
    int matrixSize = static_cast<int>(matrixMultiplySize * std::sqrt(static_cast<double>(intensityMultiplier)));
    auto matrixRunner = [&]() {
        pool.runAll([&]() {
            Benchmark::matrixMultiplyBenchmark(matrixSize);
        });
    };

    double elapsedMatrix = Benchmark::runBenchmark(matrixRunner, numRuns);
    long long matrixOps = 1LL * matrixSize * matrixSize * matrixSize * numThreads; // actual number of operations
    std::cout << "Matrix multiply benchmark completed in " << elapsedMatrix << " seconds.\n";
    scorer.addScore("MatrixMultiply", static_cast<double>(matrixOps), elapsedMatrix);

    //Branch Prediction Benchmark
    auto branchRunner = [&]() {
        pool.runAll([&]() {
            Benchmark::branchPredictionBenchmark(iterationsPerThread);
        });
    };

    double elapsedBranch = Benchmark::runBenchmark(branchRunner, numRuns);
    std::cout << "Branch prediction benchmark completed in " << elapsedBranch << " seconds.\n";
    scorer.addScore("Branch", elapsedBranch, elapsedBranch);

    //Nbody Benchmark
    int steps = 10;
    int bodies = static_cast<int>(std::sqrt(iterationsPerThread));

    auto nbodyRunner = [&]() {
        pool.runAll([&]() {
            Benchmark::nBodyBenchmark(bodies, steps);
        });
    };

    double nbodyOps = bodies * bodies * steps;
    double elapsedNbody = Benchmark::runBenchmark(nbodyRunner, numRuns);
    std::cout << "NBody benchmark completed in " << elapsedNbody << " seconds.\n";
    scorer.addScore("NBody", nbodyOps, elapsedNbody);

    //Sorting Benchmark
    int sortSize = 50000 * static_cast<int>(std::sqrt(intensityMultiplier));
    auto sortRunner = [&]() {
        pool.runAll([&]() {
            Benchmark::sortingBenchmark(sortSize);
        });
    };

    double sortOps = 1.0 * sortSize * std::log2(static_cast<double>(sortSize));
    double elapsedSort = Benchmark::runBenchmark(sortRunner, numRuns);
    std::cout << "Sorting benchmark completed in " << elapsedSort << " seconds.\n";
    scorer.addScore("Sorting", sortOps, elapsedSort);

    // Return total duration
    double totalTime = elapsedFloat + elapsedInt + elapsedMatrix + elapsedBranch + elapsedNbody;
    std::cout << "Total multi-threaded benchmark time: " << totalTime << " seconds.\n";

    return scorer.getScores();
}

std::vector<Score> BenchmarkCoordinator::runSingleThreadedBenchmark(int iterations, const int intensityMultiplier, const int matrixMultiplySize, const int numRuns){
    Scorer scorer;
    iterations *= intensityMultiplier;

    //Floating-point benchmark
    auto floatRunner = [&]() {
        Benchmark::floatingPointBenchmark(iterations);
    };
    double elapsedFloat = Benchmark::runBenchmark(floatRunner, numRuns);
    std::cout << "Floating-point benchmark completed in " << elapsedFloat << " seconds.\n";
    scorer.addScore("FloatingPoint", elapsedFloat, elapsedFloat);

    //Integer arithmetic benchmark
    auto intRunner = [&]() {
        Benchmark::integerArithmeticBenchmark(iterations);
    };

    double elapsedInt = Benchmark::runBenchmark(intRunner, numRuns);
    std::cout << "Integer arithmetic benchmark completed in " << elapsedInt << " seconds.\n";
    scorer.addScore("Integer", elapsedInt, elapsedInt);

    //Matrix multiply benchmark
    int matrixSize = static_cast<int>(matrixMultiplySize * std::sqrt(static_cast<double>(intensityMultiplier)));
    auto matrixRunner = [&]() {
        Benchmark::matrixMultiplyBenchmark(matrixSize);
    };

    double elapsedMatrix = Benchmark::runBenchmark(matrixRunner, numRuns);
    std::cout << "Matrix multiply benchmark completed in " << elapsedMatrix << " seconds.\n";
    long long matrixOps = 1LL * matrixSize * matrixSize * matrixSize;
    scorer.addScore("MatrixMultiply", static_cast<double>(matrixOps), elapsedMatrix);

    //Branch prediction benchmark
    auto branchRunner = [&]() {
        Benchmark::branchPredictionBenchmark(iterations);
    };
    
    double elapsedBranch = Benchmark::runBenchmark(branchRunner, numRuns);
    std::cout << "Branch prediction benchmark completed in " << elapsedBranch << " seconds.\n";
    scorer.addScore("Branch", elapsedBranch, elapsedBranch);

    //NBodies Benchmark
    int steps = 10;
    int bodies = static_cast<int>(std::sqrt(iterations));
    auto nbodyRunner = [&]() {
        Benchmark::nBodyBenchmark(bodies, steps);
    };
    
    double elapsedNbody = Benchmark::runBenchmark(nbodyRunner, numRuns);
    double nbodyOps = bodies * bodies * steps;
    std::cout << "Nbody benchmark completed in " << elapsedNbody << " seconds.\n";
    scorer.addScore("NBody", nbodyOps, elapsedNbody);
    
    //Sorting benchmark 
    int sortSize = static_cast<int>(50000 * std::sqrt(static_cast<double>(intensityMultiplier)));
    auto sortRunner = [&]() {
        Benchmark::sortingBenchmark(sortSize);
    };

    double elapsedSort = Benchmark::runBenchmark(sortRunner, numRuns);
    double sortOps = 1.0 * sortSize * std::log2(static_cast<double>(sortSize));
    std::cout << "Sorting benchmark completed in " << elapsedSort << " seconds.\n";
    scorer.addScore("Sorting", sortOps, elapsedSort);

    double totalTime = elapsedFloat + elapsedInt + elapsedMatrix + elapsedBranch + elapsedNbody;
    std::cout << "Total single-threaded benchmark time: " << totalTime << " seconds.\n";

    return scorer.getScores();
}