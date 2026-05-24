#include "benchmark.hpp"
#include <chrono>
#include <cmath>
#include <random>
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

    //Optimization mitigation
    volatile double _ = sum;
}

void Benchmark::branchPredictionBenchmark(int iterations){
    std::vector<uint8_t> pattern(static_cast<size_t>(iterations));
    std::mt19937 rng(2026); //Random number allow this to be changed via cli later
    std::uniform_int_distribution<int> dist(0, 1);

    for (auto& v : pattern) v = static_cast<uint8_t>(dist(rng));

    volatile int sum = 0;

    for (std::size_t i = 0; i < static_cast<std::size_t>(iterations); ++i)
        if (pattern[i]) sum++;
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