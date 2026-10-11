#include "benchmark.hpp"
#include "optimization_utils.hpp"
#include <chrono>
#include <cmath>
#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <thread>
#include <limits>

namespace {

template <typename Value, typename Operation>
Value run_arithmetic_kernel(
    int iterations, const std::array<Value, 256>& operands,
    const std::array<Value, 256>& numerators, Operation operation) {
    std::array<Value, benchmark::arithmetic_lanes> accumulators{};
    for (std::size_t lane = 0; lane < accumulators.size(); ++lane) {
        accumulators[lane] = static_cast<Value>(lane + 1);
    }

    for (int i = 0; i < iterations; ++i) {
        const std::size_t offset =
            (static_cast<std::size_t>(i) % (operands.size() / benchmark::arithmetic_lanes)) *
            benchmark::arithmetic_lanes;
        for (std::size_t lane = 0; lane < accumulators.size(); ++lane) {
            const std::size_t index = offset + lane;
            accumulators[lane] = operation(accumulators[lane], operands[index], numerators[index]);
        }
    }

    Value checksum{};
    for (Value value : accumulators) checksum += value;
    escape(checksum);
    clobber();
    return checksum;
}

}  // namespace

benchmark::integer_kernel_workload::integer_kernel_workload(
    int iterations, arithmetic_operation operation)
    : iterations_(iterations), operation_(operation) {
    if (iterations <= 0) throw std::invalid_argument("Arithmetic iterations must be positive");
    for (std::size_t i = 0; i < operands_.size(); ++i) {
        operands_[i] = 3U + 2U * static_cast<std::uint32_t>(i % 127U);
        numerators_[i] = 100'003U + 977U * static_cast<std::uint32_t>(i);
    }
}

std::uint32_t benchmark::integer_kernel_workload::run() const {
    switch (operation_) {
        case arithmetic_operation::add:
            return run_arithmetic_kernel(iterations_, operands_, numerators_,
                [](std::uint32_t value, std::uint32_t operand, std::uint32_t) {
                    return value + operand;
                });
        case arithmetic_operation::multiply:
            return run_arithmetic_kernel(iterations_, operands_, numerators_,
                [](std::uint32_t value, std::uint32_t operand, std::uint32_t) {
                    return value * operand;
                });
        case arithmetic_operation::divide:
            return run_arithmetic_kernel(iterations_, operands_, numerators_,
                [](std::uint32_t value, std::uint32_t operand, std::uint32_t numerator) {
                    return value + (numerator + (value & 1023U)) / operand;
                });
    }
    throw std::invalid_argument("Unknown arithmetic operation");
}

benchmark::floating_kernel_workload::floating_kernel_workload(
    int iterations, arithmetic_operation operation)
    : iterations_(iterations), operation_(operation) {
    if (iterations <= 0) throw std::invalid_argument("Arithmetic iterations must be positive");
    for (std::size_t block = 0; block < operands_.size() / arithmetic_lanes; block += 2) {
        for (std::size_t lane = 0; lane < arithmetic_lanes; ++lane) {
            const double factor = 1.0 + static_cast<double>((block / 2 + lane) % 127U + 1U) * 1e-7;
            operands_[block * arithmetic_lanes + lane] = factor;
            operands_[(block + 1) * arithmetic_lanes + lane] = 1.0 / factor;
        }
    }
}

double benchmark::floating_kernel_workload::run() const {
    switch (operation_) {
        case arithmetic_operation::add:
            return run_arithmetic_kernel(iterations_, operands_, operands_,
                [](double value, double operand, double) { return value + operand; });
        case arithmetic_operation::multiply:
            return run_arithmetic_kernel(iterations_, operands_, operands_,
                [](double value, double operand, double) { return value * operand; });
        case arithmetic_operation::divide:
            return run_arithmetic_kernel(iterations_, operands_, operands_,
                [](double value, double operand, double) { return value / operand; });
    }
    throw std::invalid_argument("Unknown arithmetic operation");
}

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

benchmark::branch_pattern_workload::branch_pattern_workload(
    int iterations, branch_pattern pattern)
    : pattern_(iterations > 0 ? static_cast<std::size_t>(iterations) : 0U) {
    if (iterations <= 0) {
        throw std::invalid_argument("Branch pattern iterations must be positive");
    }
    for (std::size_t i = 0; i < pattern_.size(); ++i) {
        pattern_[i] = static_cast<std::uint8_t>((i & 1U) == 0U);
        taken_count_ += pattern_[i];
    }
    if (pattern == branch_pattern::unpredictable) {
        std::mt19937 rng(2026);
        std::shuffle(pattern_.begin(), pattern_.end(), rng);
    }
}

std::uint64_t benchmark::branch_pattern_workload::run() const {
    volatile std::uint64_t taken = 0;
    for (std::uint8_t condition : pattern_) {
        if (condition != 0U) {
            // The conditional volatile write preserves a real branch in optimized builds.
            taken = taken + 1U;
        }
    }
    const std::uint64_t result = taken;
    escape(result);
    clobber();
    return result;
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

benchmark::sorting_workload::sorting_workload(int size)
    : input_(static_cast<std::size_t>(size)), working_(static_cast<std::size_t>(size)) {
    std::mt19937 rng(2026); 
    std::uniform_int_distribution<int> dist(0, size);
    for (auto& v : input_)
        v = dist(rng);
}

void benchmark::sorting_workload::reset() {
    std::copy(input_.begin(), input_.end(), working_.begin());
}

void benchmark::sorting_workload::run() {
    std::sort(working_.begin(), working_.end());

    if (!working_.empty()) {
        escape(working_.front());
    }
    clobber();
}

void benchmark::sorting_benchmark(int size) {
    sorting_workload workload(size);
    workload.reset();
    workload.run();
}

benchmark::memory_bandwidth_workload::memory_bandwidth_workload(std::size_t size)
    : a_(size, 1.0), b_(size, 2.0), c_(size, 0.0) {}

void benchmark::memory_bandwidth_workload::run() {
    const double scalar = 3.0;

    for (std::size_t i = 0; i < c_.size(); ++i) {
        c_[i] = a_[i] + scalar * b_[i];
    }

    double sum = 0.0;
    for (double v : c_) sum += v;

    escape(sum);
    clobber();
}

void benchmark::memory_bandwidth_benchmark(size_t size) {
    memory_bandwidth_workload workload(size);
    workload.run();
}

benchmark::pointer_chase_workload::pointer_chase_workload(std::size_t size)
    : next_(size) {
    std::vector<size_t> order(size);
    std::iota(order.begin(), order.end(), 0);

    std::mt19937 rng(2026);
    std::shuffle(order.begin(), order.end(), rng);

    for (size_t i = 0; i < size; ++i) {
        next_[order[i]] = order[(i + 1) % size];
    }
}

void benchmark::pointer_chase_workload::run() const {
    size_t idx = 0;
    for (size_t i = 0; i < next_.size(); ++i)
        idx = next_[idx];

    escape(idx);
    clobber();
}

void benchmark::pointer_chase_benchmark(size_t size) {
    pointer_chase_workload workload(size);
    workload.run();
}

benchmark::cache_sweep_workload::cache_sweep_workload(std::size_t working_set_bytes)
    : nodes_(working_set_bytes / sizeof(node)) {
    if (working_set_bytes < 2 * sizeof(node) ||
        working_set_bytes % sizeof(node) != 0 ||
        nodes_.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument("Cache sweep size must contain whole cache lines");
    }
    std::vector<std::uint32_t> order(nodes_.size());
    std::iota(order.begin(), order.end(), 0U);
    std::mt19937 rng(2026);
    std::shuffle(order.begin(), order.end(), rng);
    for (std::size_t i = 0; i < order.size(); ++i) {
        nodes_[order[i]].next = order[(i + 1) % order.size()];
    }
}

void benchmark::cache_sweep_workload::run(std::size_t hops) const {
    std::uint32_t index = 0;
    for (std::size_t i = 0; i < hops; ++i) {
        index = nodes_[index].next;
    }
    escape(index);
    clobber();
}

void benchmark::cache_latency_benchmark(size_t size, size_t stride) {
    if (size == 0) {
        return;
    }

    std::vector<size_t> data(size);

    size_t effective_stride = stride % size;
    if (effective_stride == 0) {
        effective_stride = 1;
    }
    while (std::gcd(effective_stride, size) != 1) {
        ++effective_stride;
    }

    for (size_t i = 0; i < size; ++i)
        data[i] = (i + effective_stride) % size;

    size_t idx = 0;
    volatile size_t sink = 0;

    // Walk through the array many times
    for (size_t i = 0; i < size * 16; ++i)
        idx = data[idx];

    sink = idx;
    escape(sink);
    clobber();
}
