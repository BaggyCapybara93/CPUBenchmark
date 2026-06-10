#include "benchmark.hpp"
#include "optimization_utils.hpp"
#include <chrono>
#include <cmath>
#include <random>
#include <algorithm>
#include <iostream>
#include <thread>

void benchmark::floating_point_benchmark(int iterations) {
    double result = 0.0;
    for (int i = 0; i < iterations; ++i) {
        double x = static_cast<double>(i);
        result += std::sin(x) * std::cos(x) * std::tan(x + 0.5);
    }

    escape(result);
    clobber();

}

void benchmark::integer_arithmetic_benchmark(int iterations){
   uint32_t result = 0;

    for (int i = 1; i < iterations; ++i) {
        result = (result * 1664525u + 1013904223u) ^ static_cast<uint32_t>(i);
    }

    escape(result);
    clobber();
}

void benchmark::matrix_multiply_benchmark(int size) {
    const size_t N = static_cast<size_t>(size);

    std::vector<double> A(N * N, 1.0);
    std::vector<double> B(N * N, 1.0);
    std::vector<double> C(N * N, 0.0);

    const size_t block = 32; // Replace with auto-detected L1 block size util

    for (size_t ii = 0; ii < N; ii += block) {
        size_t i_end = std::min(ii + block, N);

        for (size_t kk = 0; kk < N; kk += block) {
            size_t k_end = std::min(kk + block, N);

            for (size_t jj = 0; jj < N; jj += block) {
                size_t j_end = std::min(jj + block, N);

                for (size_t i = ii; i < i_end; ++i) {
                    size_t i_row = i * N;

                    for (size_t k = kk; k < k_end; ++k) {
                        double aik = A[i_row + k];
                        size_t k_row = k * N;

                        for (size_t j = jj; j < j_end; ++j)
                            C[i_row + j] += aik * B[k_row + j];
                    }
                }
            }
        }
    }

    double sum = 0.0;
    for(const auto& value : C){
        sum += value;
    }

    //Optimization mitigation
    escape(sum);
    clobber();
}

void benchmark::branch_prediction_benchmark(int iterations){
    std::vector<uint8_t> pattern(static_cast<size_t>(iterations));
    std::mt19937 rng(2026); //Random number allow this to be changed via cli later
    std::uniform_int_distribution<int> dist(0, 1);

    for (auto& v : pattern) v = static_cast<uint8_t>(dist(rng));

    int sum = 0;

    for (std::size_t i = 0; i < static_cast<std::size_t>(iterations); ++i)
        if (pattern[i]) sum++;
    
    // Prevent optimization
    escape(sum);
    clobber();
}

void benchmark::n_body_benchmark(int n_bodies, int steps) {
    using std::size_t;

    const size_t N = static_cast<size_t>(n_bodies);

    std::vector<double> x(N), y(N), z(N);
    std::vector<double> vx(N, 0.0), vy(N, 0.0), vz(N, 0.0);
    std::vector<double> mass(N, 1.0);

    // Initialize positions deterministically
    for (size_t i = 0; i < N; ++i) {
        x[i] = static_cast<double>(i) * 0.01;
        y[i] = static_cast<double>(i) * 0.02;
        z[i] = static_cast<double>(i) * 0.03;
    }

    constexpr double G  = 6.67430e-11;
    constexpr double dt = 0.01;

    for (int step = 0; step < steps; ++step) {

        // --- Compute accelerations ---
        for (size_t i = 0; i < N; ++i) {
            double ax = 0.0, ay = 0.0, az = 0.0;

            for (size_t j = 0; j < N; ++j) {
                if (i == j) continue;

                double dx = x[j] - x[i];
                double dy = y[j] - y[i];
                double dz = z[j] - z[i];

                double distSqr = dx*dx + dy*dy + dz*dz + 1e-9;
                double invDist = 1.0 / std::sqrt(distSqr);
                double invDist3 = invDist * invDist * invDist;

                double force = G * mass[j] * invDist3;

                ax += dx * force;
                ay += dy * force;
                az += dz * force;
            }

            vx[i] += ax * dt;
            vy[i] += ay * dt;
            vz[i] += az * dt;
        }

        // --- Update positions ---
        for (size_t i = 0; i < N; ++i) {
            x[i] += vx[i] * dt;
            y[i] += vy[i] * dt;
            z[i] += vz[i] * dt;
        }

        // --- Compute kinetic energy ---
        double totalEnergy = 0.0;
        for (size_t i = 0; i < N; ++i) {
            double KE = 0.5 * mass[i] * (vx[i]*vx[i] + vy[i]*vy[i] + vz[i]*vz[i]);
            totalEnergy += KE;
        }

        escape(totalEnergy);
        clobber();
    }
}

void benchmark::sorting_benchmark(int size) {
    using std::size_t;

    // --- Preallocate ---
    std::vector<int> data(static_cast<size_t>(size));
    std::vector<int> backup(static_cast<size_t>(size));

    // --- Generate random data ---
    std::mt19937 rng(2026); 
    std::uniform_int_distribution<int> dist(0, size);

    for (auto& v : data)
        v = dist(rng);

    // Keep a copy so each timed run sorts identical input
    backup = data;

    // --- Warm-up run ---
    {
        std::vector<int> warm = data;
        std::sort(warm.begin(), warm.end());
    }

    data = backup;  // restore original unsorted data
    std::sort(data.begin(), data.end());

    // Prevent optimization
    escape(data[0]);
    clobber();
}

void benchmark::memory_bandwidth_benchmark(size_t size){
    std::vector<double> a(size, 1.0);
    std::vector<double> b(size, 2.0);
    std::vector<double> c(size, 0.0);

    const double scalar = 3.0;

    for (std::size_t i = 0; i < size; ++i) {
        c[i] = a[i] + scalar * b[i];
    }

    double sum = 0.0;
    for (double v : c) sum += v;

    escape(sum);
    clobber();
}

void benchmark::pointer_chase_benchmark(size_t size) {
    std::vector<size_t> next(size);
    for (size_t i = 0; i < size; ++i)
        next[i] = i;

    // Shuffle to destroy spatial locality
    std::mt19937 rng(2026);
    std::shuffle(next.begin(), next.end(), rng);

    // Pointer-chasing loop
    size_t idx = 0;
    for (size_t i = 0; i < size; ++i)
        idx = next[idx];

    escape(idx);
    clobber();
}
