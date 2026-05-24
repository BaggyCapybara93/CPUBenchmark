#include "benchmarkCoordinator.hpp"
#include <iostream>

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

    report_.saveBenchmark();
}

std::vector<Score>  BenchmarkCoordinator::runMultithreadedBenchmark(const int numThreads, int iterationsPerThread, const int intensityMultiplier, const int matrixMultiplySize, const int numRuns) {
    Scorer scorer;
    iterationsPerThread *= intensityMultiplier;

    // Run floating-point benchmark first
    auto floatRunner = [&]() {
        std::vector<std::thread> floatThreads;
        for (int i = 0; i < numThreads; ++i) {
            floatThreads.emplace_back(Benchmark::floatingPointBenchmark, iterationsPerThread);
        }
        for (auto& thread : floatThreads) {
            thread.join();
        }
    };

    double elapsedFloat = Benchmark::runBenchmark(floatRunner, numRuns);
    std::cout << "Floating-point benchmark completed in " << elapsedFloat << " seconds.\n";
    scorer.addScore("FloatingPoint", elapsedFloat, elapsedFloat);

    // Integer arithmetic benchmark 
    auto intRunner = [&]() {
        std::vector<std::thread> intThreads;
        for (int i = 0; i < numThreads; ++i) {
            intThreads.emplace_back(Benchmark::integerArithmeticBenchmark, iterationsPerThread);
        }
        for (auto& thread : intThreads) {
            thread.join();
        }
    };

    double elapsedInt = Benchmark::runBenchmark(intRunner, numRuns);
    std::cout << "Integer arithmetic benchmark completed in " << elapsedInt << " seconds.\n";
    scorer.addScore("Integer", elapsedInt, elapsedInt);

    //Matrix Multiplication Benchmark
    int matrixSize = static_cast<int>(matrixMultiplySize * std::sqrt(static_cast<double>(intensityMultiplier)));
    auto matrixRunner = [&]() {
        std::vector<std::thread> matrixThreads;
        for (int i = 0; i < numThreads; ++i) {
            matrixThreads.emplace_back(Benchmark::matrixMultiplyBenchmark, matrixSize);
        }
        for (auto& thread : matrixThreads) {
            thread.join();
        }
    };

    double elapsedMatrix = Benchmark::runBenchmark(matrixRunner, numRuns);
    int matrixOps = matrixSize * matrixSize * numThreads; // actual number of operations
    std::cout << "Matrix multiply benchmark completed in " << elapsedMatrix << " seconds.\n";
    scorer.addScore("MatrixMultiply", matrixOps, elapsedMatrix);

    //Branch Prediction Benchmark
    auto branchRunner = [&]() {
        std::vector<std::thread> branchThreads;
        for (int i = 0; i < numThreads; ++i) {
            branchThreads.emplace_back(Benchmark::branchPredictionBenchmark, iterationsPerThread);
        }
        for (auto& thread : branchThreads) {
            thread.join();
        }
    };

    double elapsedBranch = Benchmark::runBenchmark(branchRunner, numRuns);
    std::cout << "Branch prediction benchmark completed in " << elapsedBranch << " seconds.\n";
    scorer.addScore("Branch", elapsedBranch, elapsedBranch);

    //Nbody Benchmark
    int steps = 10;
    int bodies = static_cast<int>(std::sqrt(iterationsPerThread));

    auto nbodyRunner = [&]() {
        std::vector<std::thread> nbodyThreads;
        for (int i = 0; i < numThreads; ++i) {
            nbodyThreads.emplace_back(Benchmark::nBodyBenchmark, bodies, steps);
        }
        for (auto& thread : nbodyThreads) {
            thread.join();
        }
    };

    double nbodyOps = bodies * bodies * steps;
    double elapsedNbody = Benchmark::runBenchmark(nbodyRunner, numRuns);
    std::cout << "NBody benchmark completed in " << elapsedNbody << " seconds.\n";
    scorer.addScore("NBody", nbodyOps, elapsedNbody);

    //Sorting Benchmark
    int sortSize = 50000 * static_cast<int>(std::sqrt(intensityMultiplier));
    auto sortRunner = [&]() {
        std::vector<std::thread> sortThreads;
        for (int i = 0; i < numThreads; ++i) {
            sortThreads.emplace_back(Benchmark::sortingBenchmark, iterationsPerThread);
        }
        for (auto& thread : sortThreads) {
            thread.join();
        }
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
    int matrixOps = matrixSize * matrixSize; // actual number of operations
    scorer.addScore("MatrixMultiply", matrixOps, elapsedMatrix);

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