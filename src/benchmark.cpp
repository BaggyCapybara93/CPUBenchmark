#include "benchmark.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <functional>
#include <thread>

//Time
template <typename Function, typename... Args>
double Benchmark::time(Function&& function, Args&&... args) {
    auto start = std::chrono::high_resolution_clock::now();
    std::invoke(std::forward<Function>(function), std::forward<Args>(args)...);
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(end - start).count();
}

template <typename Function, typename... Args>
void Benchmark::execute(Function&& function, Args&& ... args) {
    std::invoke(std::forward<Function>(function), std::forward<Args>(args)...);
}

void Benchmark::floatingPointBenchmark(int iterations) {
    volatile double result = 0.0;
    for (int i = 0; i < iterations; ++i) {
        result += std::sin(i) * std::cos(i) / std::tan(i + 1);
    }
}

template <typename Function, typename... Args>
double Benchmark::runBenchmark(Function func, Args... args, int numRuns) {
    if (numRuns <= 0) {
        // Handle this error gracefully, perhaps throwing an exception
        // or returning a specific error code/value.
        throw std::invalid_argument("Number of runs must be positive.");
    }

    //Warmup
    const int warmupRuns = 5;
    for (int i = 0; i < warmupRuns; ++i) {
        Benchmark::execute(func, args...);
    }

    // Measurement
    std::vector<double> runTimes;
    
    for (int i = 0; i < numRuns; ++i) {
        double duration = Benchmark::time(func, args...); 
        runTimes.push_back(duration);
    }

    // Calculation
    double totalTime = std::accumulate(runTimes.begin(), runTimes.end(), 0.0);
    
    return totalTime / numRuns; // Returns the average 
}

void Benchmark::integerArithmeticBenchmark(int iterations){
    volatile int result = 0;
    for(int i = 0; i < iterations; ++i){
        result += i * (i + 1)/(i + 2);
    }
}

void Benchmark::matrixMultiplyBenchmark(int size) {
    std::vector<double> A(size * size, 1.0);
    std::vector<double> B(size * size, 1.0);
    std::vector<double> C(size * size, 0.0);

    for (int i = 0; i < size; ++i) {
        for (int k = 0; k < size; ++k) {
            double aik = A[i * size + k];
            for (int j = 0; j < size; ++j) {
                C[i * size + j] += aik * B[k * size + j];
            }
        }
    }
}

void Benchmark::branchPredictionBenchmark(int iterations){
    volatile int sum = 0;

    for(int i = 0; i < iterations; ++i){
        if(i < iterations - 1) sum++;
    }

    for(int i = 0; i < iterations; ++i){
        if(rand() % 2 == 0) sum++;
    }
}

void Benchmark::nBodyBenchmark(int nBodies, int steps) {
    struct Body {
        double x, y, z;
        double vx, vy, vz;
        double mass;
    };

    std::vector<Body> bodies(nBodies);

    // Initialize bodies with deterministic values
    for (int i = 0; i < nBodies; ++i) {
        bodies[i].x = i * 0.01;
        bodies[i].y = i * 0.02;
        bodies[i].z = i * 0.03;
        bodies[i].vx = bodies[i].vy = bodies[i].vz = 0.0;
        bodies[i].mass = 1.0;
    }

    const double G = 6.67430e-11;
    const double dt = 0.01;

    for (int step = 0; step < steps; ++step) {
        for (int i = 0; i < nBodies; ++i) {
            double ax = 0.0, ay = 0.0, az = 0.0;

            for (int j = 0; j < nBodies; ++j) {
                if (i == j) continue;

                double dx = bodies[j].x - bodies[i].x;
                double dy = bodies[j].y - bodies[i].y;
                double dz = bodies[j].z - bodies[i].z;

                double distSqr = dx*dx + dy*dy + dz*dz + 1e-9;
                double invDist = 1.0 / std::sqrt(distSqr);
                double invDist3 = invDist * invDist * invDist;

                double force = G * bodies[j].mass * invDist3;

                ax += dx * force;
                ay += dy * force;
                az += dz * force;
            }

            bodies[i].vx += ax * dt;
            bodies[i].vy += ay * dt;
            bodies[i].vz += az * dt;
        }

        for (int i = 0; i < nBodies; ++i) {
            bodies[i].x += bodies[i].vx * dt;
            bodies[i].y += bodies[i].vy * dt;
            bodies[i].z += bodies[i].vz * dt;
        }
    }
}

void Benchmark::sortingBenchmark(int size){
    std::vector<int> data(size);

    for(int i = 0; i < size; ++i){
        data[i] = size - 1;
    }

    std::sort(data.begin(), data.end());
}

void Benchmark::dryRun(int iterations = 1000){
    floatingPointBenchmark(iterations);
    integerArithmeticBenchmark(iterations);
    branchPredictionBenchmark(iterations);
}

std::vector<Score>  Benchmark::runMultithreadedBenchmark(const int numThreads, int iterationsPerThread, const int intensityMultiplier, const int matrixMultiplySize, const int numRuns) {
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
    int matrixSize = matrixMultiplySize * std::sqrt(intensityMultiplier);
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
    auto endMatrix = std::chrono::high_resolution_clock::now();
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
    int bodies = std::sqrt(iterationsPerThread);

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
    int sortSize = 50000 * std::sqrt(intensityMultiplier);
    auto sortRunner = [&]() {
        std::vector<std::thread> sortThreads;
        for (int i = 0; i < numThreads; ++i) {
            sortThreads.emplace_back(Benchmark::sortingBenchmark, iterationsPerThread);
        }
        for (auto& thread : sortThreads) {
            thread.join();
        }
    };

    long long sortOps = 1LL * sortSize * std::log2(sortSize);
    double elapsedSort = Benchmark::runBenchmark(sortRunner, numRuns);
    std::cout << "Sorting benchmark completed in " << elapsedSort << " seconds.\n";
    scorer.addScore("Sorting", sortOps, elapsedSort);

    // Return total duration
    double totalTime = elapsedFloat + elapsedInt + elapsedMatrix + elapsedBranch + elapsedNbody;
    std::cout << "Total multi-threaded benchmark time: " << totalTime << " seconds.\n";

    return scorer.getScores();
}


std::vector<Score> Benchmark::runSingleThreadedBenchmark(int iterations, const int intensityMultiplier, const int matrixMultiplySize, const int numRuns){
    Scorer scorer;
    iterations *= intensityMultiplier;

    dryRun();

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
    int matrixSize = matrixMultiplySize * std::sqrt(intensityMultiplier);
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
    int bodies = std::sqrt(iterations);
    auto nbodyRunner = [&]() {
        Benchmark::nBodyBenchmark(bodies, steps);
    };
    
    double elapsedNbody = Benchmark::runBenchmark(nbodyRunner, numRuns);
    double nbodyOps = bodies * bodies * steps;
    std::cout << "Nbody benchmark completed in " << elapsedNbody << " seconds.\n";
    scorer.addScore("NBody", nbodyOps, elapsedNbody);
    
    //Sorting benchmark 
    int sortSize = 50000 * std::sqrt(intensityMultiplier);
    auto sortRunner = [&]() {
        Benchmark::sortingBenchmark(sortSize);
    };

    double elapsedSort = Benchmark::runBenchmark(sortRunner, numRuns);
    long long sortOps = 1LL * sortSize * std::log2(sortSize);
    std::cout << "Sorting benchmark completed in " << elapsedSort << " seconds.\n";
    scorer.addScore("Sorting", sortOps, elapsedSort);

    double totalTime = elapsedFloat + elapsedInt + elapsedMatrix + elapsedBranch + elapsedNbody;
    std::cout << "Total single-threaded benchmark time: " << totalTime << " seconds.\n";

    return scorer.getScores();
}