#include "benchmark.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

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
    std::vector<double> A(static_cast<std::size_t>(size) * static_cast<std::size_t>(size), 1.0);
    std::vector<double> B(static_cast<std::size_t>(size) * static_cast<std::size_t>(size), 1.0);
    std::vector<double> C(static_cast<std::size_t>(size) * static_cast<std::size_t>(size), 0.0);

    for (std::size_t i = 0; i < static_cast<std::size_t>(size); ++i) {
        for (std::size_t k = 0; k < static_cast<std::size_t>(size); ++k) {
            double aik = A[i * static_cast<std::size_t>(size) + k];
            for (std::size_t j = 0; j < static_cast<std::size_t>(size); ++j) {
                C[i * static_cast<std::size_t>(size) + j] += aik * B[k * static_cast<std::size_t>(size) + j];
            }
        }
    }

    double sum = 0.0;
    for(const auto& value : C){
        sum += value;
    }

    //Optimizaiton mitigation
    volatile double _ = sum;
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

    std::vector<Body> bodies(static_cast<std::size_t>(nBodies));

    // Initialize bodies with deterministic values
    for (std::size_t i = 0; i < static_cast<std::size_t>(nBodies); ++i) {
        bodies[i].x = static_cast<double>(i) * 0.01;
        bodies[i].y = static_cast<double>(i) * 0.02;
        bodies[i].z = static_cast<double>(i) * 0.03;
        bodies[i].vx = bodies[i].vy = bodies[i].vz = 0.0;
        bodies[i].mass = 1.0;
    }

    const double G = 6.67430e-11;
    const double dt = 0.01;

    for (int step = 0; step < steps; ++step) {
        for (std::size_t i = 0; i < static_cast<std::size_t>(nBodies); ++i) {
            double ax = 0.0, ay = 0.0, az = 0.0;

            for (std::size_t j = 0; j < static_cast<std::size_t>(nBodies); ++j) {
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

        for (std::size_t i = 0; i < static_cast<std::size_t>(nBodies); ++i) {
            bodies[i].x += bodies[i].vx * dt;
            bodies[i].y += bodies[i].vy * dt;
            bodies[i].z += bodies[i].vz * dt;
        }

        double totalEnergy = 0.0;
        for (const auto& body : bodies){
            double KE = 0.5 * body.mass * (body.vx*body.vx + body.vy*body.vy + body.vz*body.vz);
            totalEnergy += KE;
        }

        //Optimization Mitigation
        volatile double _ = totalEnergy; 

    }
}

void Benchmark::sortingBenchmark(int size){
    std::vector<int> data(static_cast<std::size_t>(size));

    for(std::size_t i = 0; i < static_cast<std::size_t>(size); ++i){
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