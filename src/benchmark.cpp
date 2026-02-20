#include "benchmark.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

//Time
template <typename Function, typename... Args>
double Benchmark::time(Function&& function, Args&&... args) {
    auto start = std::chrono::high_resolution_clock::now();
    std::invoke(std::forward<Function>(function), std::forward<Args>(args)...);
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(end - start).count();
}

void Benchmark::floatingPointBenchmark(int iterations) {
    volatile double result = 0.0;
    for (int i = 0; i < iterations; ++i) {
        result += std::sin(i) * std::cos(i) / std::tan(i + 1);
    }
}

void Benchmark::integerArithmeticBenchmark(int iterations){
    volatile int result = 0;
    for(int i = 0; i < iterations; ++i){
        result += i * (i + 1)/(i + 2);
    }
}

void Benchmark::matrixMultiplyBenchmark(int size) {
    std::vector<std::vector<double>> matrix(size, std::vector<double>(size, 1.0));
    for (int i = 0; i < size; ++i)
        for (int j = 0; j < size; ++j)
            matrix[i][j] *= std::sin(i) * std::cos(j);
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

void Benchmark::dryRun(int iterations = 1000){
    floatingPointBenchmark(iterations);
    integerArithmeticBenchmark(iterations);
    branchPredictionBenchmark(iterations);
}

std::unordered_map<std::string, std::vector<double>> Benchmark::runMultithreadedBenchmark(int numThreads, int iterationsPerThread, const int intensityMultiplier, const int matrixMultiplySize) {
    iterationsPerThread *= intensityMultiplier;
    std::unordered_map<std::string, std::vector<double>> scores;

    dryRun();

    // Run floating-point benchmark first
    double elapsedFloat = Benchmark::time([&]() {
        std::vector<std::thread> floatThreads;
        for (int i = 0; i < numThreads; ++i) {
            floatThreads.emplace_back(Benchmark::floatingPointBenchmark, iterationsPerThread);
        }
        for (auto& thread : floatThreads) {
            thread.join();
        }
    });

    std::cout << "Floating-point benchmark completed in " << elapsedFloat << " seconds.\n";
    scores["Floating-point"].push_back(getScore(elapsedFloat, iterationsPerThread));

    // Run integer arithmetic benchmark after floating-point benchmark
    double elapsedInt = Benchmark::time([&]() {
        std::vector<std::thread> intThreads;
        for (int i = 0; i < numThreads; ++i) {
            intThreads.emplace_back(Benchmark::integerArithmeticBenchmark, iterationsPerThread);
        }
        for (auto& thread : intThreads) {
            thread.join();
        }
    });
    std::cout << "Integer arithmetic benchmark completed in " << elapsedInt << " seconds.\n";
    scores["Integer Arithmetic"].push_back(getScore(elapsedInt, iterationsPerThread));

    //Matrix Multiplication Benchmark
    int matrixSize = matrixMultiplySize * std::sqrt(intensityMultiplier);
    double elapsedMatrix = Benchmark::time([&]() {
        std::vector<std::thread> matrixThreads;
        for (int i = 0; i < numThreads; ++i) {
            matrixThreads.emplace_back(Benchmark::matrixMultiplyBenchmark, matrixSize);
        }
        for (auto& thread : matrixThreads) {
            thread.join();
        }
    });

    auto endMatrix = std::chrono::high_resolution_clock::now();
    int matrixOps = matrixSize * matrixSize * numThreads; // actual number of operations
    std::cout << "Matrix multiply benchmark completed in " << elapsedMatrix << " seconds.\n";

    scores["Matrix Multiply"].push_back(getScore(elapsedMatrix, matrixOps));

    //Branch Prediction Benchmark
    double elapsedBranch = Benchmark::time([&]() {
        std::vector<std::thread> branchThreads;
        for (int i = 0; i < numThreads; ++i) {
            branchThreads.emplace_back(Benchmark::branchPredictionBenchmark, iterationsPerThread);
        }
        for (auto& thread : branchThreads) {
            thread.join();
        }
    });

    std::cout << "Branch prediction benchmark completed in " << elapsedBranch << " seconds.\n";
    scores["Branch Prediction"].push_back(getScore(elapsedBranch, iterationsPerThread));

    //Nbody Benchmark
    int steps = 10;
    int bodies = std::sqrt(iterationsPerThread);

    double elapsedNbody = Benchmark::time([&]() {
        std::vector<std::thread> nbodyThreads;
        for (int i = 0; i < numThreads; ++i) {
            nbodyThreads.emplace_back(Benchmark::nBodyBenchmark, bodies, steps);
        }
        for (auto& thread : nbodyThreads) {
            thread.join();
        }
    });
    double nbodyOps = bodies * bodies * steps;
    std::cout << "NBody benchmark completed in " << elapsedNbody << " seconds.\n";
    scores["NBody"].push_back(getScore(elapsedNbody, nbodyOps));


    // Return total duration
    double totalTime = elapsedFloat + elapsedInt + elapsedMatrix + elapsedBranch + elapsedNbody;
    std::cout << "Total multi-threaded benchmark time: " << totalTime << " seconds.\n";
    scores["Combined"].push_back(getScore(totalTime, iterationsPerThread));

    return scores;
}


std::unordered_map<std::string, std::vector<double>> Benchmark::runSingleThreadedBenchmark(int iterations, const int intensityMultiplier, const int matrixMultiplySize){
    iterations *= intensityMultiplier;
    std::unordered_map<std::string, std::vector<double>> scores;

    dryRun();

    //Floating-point benchmark
    double elapsedFloat = Benchmark::time(Benchmark::floatingPointBenchmark, iterations);
    std::cout << "Floating-point benchmark completed in " << elapsedFloat << " seconds.\n";

    scores["Floating-point"].push_back(getScore(elapsedFloat, iterations));

    //Integer arithmetic benchmark
    double elapsedInt = Benchmark::time(Benchmark::integerArithmeticBenchmark, iterations);
    std::cout << "Integer arithmetic benchmark completed in " << elapsedInt << " seconds.\n";
    scores["Integer Arithmetic"].push_back(getScore(elapsedInt, iterations));

    //Matrix multiply benchmark
    int matrixSize = matrixMultiplySize * std::sqrt(intensityMultiplier);
    double elapsedMatrix = Benchmark::time(Benchmark::matrixMultiplyBenchmark, matrixSize);
    std::cout << "Matrix multiply benchmark completed in " << elapsedMatrix << " seconds.\n";
    int matrixOps = matrixSize * matrixSize; // actual number of operations
    scores["Matrix Multiply"].push_back(getScore(elapsedMatrix, matrixOps));

    //Branch prediction benchmark
    double elapsedBranch = Benchmark::time(Benchmark::branchPredictionBenchmark, iterations);
    std::cout << "Branch prediction benchmark completed in " << elapsedBranch << " seconds.\n";
    scores["Branch Prediction"].push_back(getScore(elapsedBranch, iterations));

    //NBodies Benchmark
    int steps = 10;
    int bodies = std::sqrt(iterations);
    double elapsedNbody = Benchmark::time(Benchmark::nBodyBenchmark, bodies, steps);
    double nbodyOps = bodies * bodies * steps;
    std::cout << "Nbody benchmark completed in " << elapsedNbody << " seconds.\n";
    scores["Nbody"].push_back(getScore(elapsedNbody, nbodyOps));

    double totalTime = elapsedFloat + elapsedInt + elapsedMatrix + elapsedBranch + elapsedNbody;
    std::cout << "Total single-threaded benchmark time: " << totalTime << " seconds.\n";
    
    scores["Combined"].push_back(getScore(totalTime, iterations));

    return scores;
}


//Memory Benchmarks
void Benchmark::runMemoryBandwidthTest(size_t size) {
    std::vector<int> data(size, 1);  // Allocate memory

    auto start = std::chrono::high_resolution_clock::now();

    // Sequential read/write test
    for (size_t i = 0; i < size; ++i) {
        data[i] += 1;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    double bandwidth = (size * sizeof(int)) / (elapsed.count() * 1024 * 1024); // MB/s

    std::cout << "Memory Bandwidth Test Completed\n";
    std::cout << "Elapsed Time: " << elapsed.count() << " seconds\n";
    std::cout << "Approximate Bandwidth: " << bandwidth << " MB/s\n";
}

//Score Calculator
double Benchmark::getScore(double intake, int iterations) {
    constexpr double referenceTime = 1.0; // seconds for baseline run
    constexpr int referenceIterations = 1000000;

    double score = (iterations / intake) / (referenceIterations / referenceTime);
    return score * 1000.0; // scaled nicely
}