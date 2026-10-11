#include "benchmark_coordinator.hpp"
#include <iostream>
#include <memory>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <initializer_list>

//Threading
void benchmark_thread_pool::run_all(const std::function<void()>& fn) {
    run_all_indexed([&fn](std::size_t) { fn(); });
}

void benchmark_thread_pool::run_all_indexed(const std::function<void(std::size_t)>& fn) {
    {
        std::lock_guard<std::mutex> lock(worker_exception_mutex_);
        worker_exception_ = nullptr;
        current_task_ = fn;
    }

    start_barrier_.arrive_and_wait();

    done_barrier_.arrive_and_wait();

    std::lock_guard<std::mutex> lock(worker_exception_mutex_);
    if (worker_exception_) {
        std::rethrow_exception(worker_exception_);
    }
}

void benchmark_thread_pool::worker_loop(std::size_t worker_index) {
    (void)thread_affinity::pin_current_thread(worker_index);
    while (true) {
        start_barrier_.arrive_and_wait();

        if (stop_) {
            done_barrier_.arrive_and_wait();
            break;
        }

        try {
            current_task_(worker_index);
        } catch (...) {
            std::lock_guard<std::mutex> lock(worker_exception_mutex_);
            if (!worker_exception_) {
                worker_exception_ = std::current_exception();
            }
        }

        done_barrier_.arrive_and_wait();
    }
}

namespace {
std::uint64_t work_product(std::initializer_list<std::uint64_t> factors) {
    std::uint64_t product = 1;
    for (std::uint64_t factor : factors) {
        if (factor != 0 && product > std::numeric_limits<std::uint64_t>::max() / factor) {
            throw std::overflow_error("Benchmark work count is too large");
        }
        product *= factor;
    }
    return product;
}

template <typename Function>
void run_shards(benchmark_thread_pool& pool, std::size_t worker_count,
                std::size_t shard_count, Function&& fn) {
    pool.run_all_indexed([&](std::size_t worker_index) {
        for (std::size_t shard = worker_index; shard < shard_count; shard += worker_count) {
            fn(shard);
        }
    });
}

template <typename Workload, typename Factory>
std::vector<std::unique_ptr<Workload>> prepare_workloads(
    benchmark_thread_pool& pool, std::size_t worker_count, std::size_t shard_count,
    Factory&& make_workload) {
    std::vector<std::unique_ptr<Workload>> workloads(shard_count);
    run_shards(pool, worker_count, shard_count, [&](std::size_t shard) {
        workloads[shard] = make_workload(shard);
    });
    return workloads;
}
}  // namespace

void benchmark_coordinator::run_benchmark(runner_function runner)
{
    std::vector<score> scores = runner(*this);
    report_.add_score(scores);
}

std::vector<score> benchmark_coordinator::run_specific_benchmarks(
    const std::vector<std::string>& benchmark_names, int num_threads, int total_iterations) {
    if (num_threads <= 0 || settings_->thread_count <= 0 || total_iterations <= 0 ||
        settings_->intensity_multiplier <= 0) {
        throw std::invalid_argument("Benchmark needs positive thread, iteration, and intensity counts");
    }
    const std::size_t worker_count = static_cast<std::size_t>(num_threads);
    const std::size_t shard_count = static_cast<std::size_t>(
        std::min(settings_->thread_count, total_iterations));

    report_.set_run_configuration(*settings_, benchmark_names, num_threads);
    scorer scorer;
    scorer.load_weights(settings_->weights);
    scorer.load_baseline_times(settings_->baseline_times);

    const int intensity = settings_->intensity_multiplier;
    const int num_runs = settings_->num_runs;
    const std::uint64_t total_effective = work_product({
        static_cast<std::uint64_t>(total_iterations), static_cast<std::uint64_t>(intensity)});
    std::vector<int> iterations_by_shard(shard_count);
    for (std::size_t shard = 0; shard < shard_count; ++shard) {
        const auto iterations = static_cast<std::uint64_t>(total_iterations) / shard_count +
            (shard < static_cast<std::uint64_t>(total_iterations) % shard_count ? 1U : 0U);
        const auto effective = work_product({iterations, static_cast<std::uint64_t>(intensity)});
        if (effective > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
            throw std::invalid_argument("Iterations per shard exceed the supported range");
        }
        iterations_by_shard[shard] = static_cast<int>(effective);
    }

    const double matrix_size_value = static_cast<double>(settings_->matrix_multiply_size) *
                                     std::sqrt(static_cast<double>(intensity));
    if (!std::isfinite(matrix_size_value) ||
        matrix_size_value > static_cast<double>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("Matrix size exceeds the supported range");
    }
    const int matrix_size = static_cast<int>(matrix_size_value);
    const double sort_size_value = 50000.0 *
        static_cast<double>(static_cast<int>(std::sqrt(static_cast<double>(intensity))));
    if (!std::isfinite(sort_size_value) ||
        sort_size_value > static_cast<double>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("Sorting size exceeds the supported range");
    }
    const int sort_size = static_cast<int>(sort_size_value);

    benchmark_thread_pool pool(num_threads);
    double total_time = 0.0;

    for (const auto& benchmark_name : benchmark_names) {
        std::cout << "\n=== Running benchmark: " << benchmark_name << " ===\n";
        double elapsed = 0.0;

        if (benchmark_name == "floating_point") {
            auto runner = [&]() {
                run_shards(pool, worker_count, shard_count, [&](std::size_t shard) {
                    benchmark::floating_point_benchmark(iterations_by_shard[shard]);
                });
            };
            elapsed = scorer.add_score("floating_point", benchmark::run_benchmark(runner, num_runs),
                                       "loop iterations", total_effective);
        } else if (benchmark_name == "integer_arithmetic" || benchmark_name == "integer") {
            auto runner = [&]() {
                run_shards(pool, worker_count, shard_count, [&](std::size_t shard) {
                    benchmark::integer_arithmetic_benchmark(iterations_by_shard[shard]);
                });
            };
            elapsed = scorer.add_score("integer", benchmark::run_benchmark(runner, num_runs),
                                       "loop iterations", total_effective - shard_count);
        } else if (benchmark_name == "matrix_multiply") {
            auto runner = [&]() {
                run_shards(pool, worker_count, shard_count, [&](std::size_t) {
                    benchmark::matrix_multiply_benchmark(matrix_size);
                });
            };
            elapsed = scorer.add_score("matrix_multiply", benchmark::run_benchmark(runner, num_runs),
                                       "multiply-adds", work_product({shard_count,
                                           static_cast<std::uint64_t>(matrix_size),
                                           static_cast<std::uint64_t>(matrix_size),
                                           static_cast<std::uint64_t>(matrix_size)}));
        } else if (benchmark_name == "branch_prediction" || benchmark_name == "branch") {
            auto runner = [&]() {
                run_shards(pool, worker_count, shard_count, [&](std::size_t shard) {
                    benchmark::branch_prediction_benchmark(iterations_by_shard[shard]);
                });
            };
            elapsed = scorer.add_score("branch", benchmark::run_benchmark(runner, num_runs),
                                       "input elements", total_effective);
        } else if (benchmark_name == "n_body") {
            constexpr int steps = 10;
            std::vector<int> bodies_by_shard(shard_count);
            std::uint64_t interactions = 0;
            for (std::size_t shard = 0; shard < shard_count; ++shard) {
                const int bodies = static_cast<int>(std::sqrt(
                    static_cast<double>(iterations_by_shard[shard])));
                bodies_by_shard[shard] = bodies;
                const std::uint64_t shard_interactions = work_product({static_cast<std::uint64_t>(bodies),
                    static_cast<std::uint64_t>(bodies - 1), static_cast<std::uint64_t>(steps)});
                if (interactions > std::numeric_limits<std::uint64_t>::max() - shard_interactions) {
                    throw std::overflow_error("N-body work count is too large");
                }
                interactions += shard_interactions;
            }
            auto runner = [&]() {
                run_shards(pool, worker_count, shard_count, [&](std::size_t shard) {
                    benchmark::n_body_benchmark(bodies_by_shard[shard], steps);
                });
            };
            elapsed = scorer.add_score("n_body", benchmark::run_benchmark(runner, num_runs),
                                       "body-pair evaluations", interactions);
        } else if (benchmark_name == "sorting") {
            auto workloads = prepare_workloads<benchmark::sorting_workload>(
                pool, worker_count, shard_count,
                [&](std::size_t) { return std::make_unique<benchmark::sorting_workload>(sort_size); });
            auto setup = [&]() {
                run_shards(pool, worker_count, shard_count, [&](std::size_t shard) {
                    workloads[shard]->reset();
                });
            };
            auto runner = [&]() {
                run_shards(pool, worker_count, shard_count, [&](std::size_t shard) {
                    workloads[shard]->run();
                });
            };
            elapsed = scorer.add_score("sorting", benchmark::run_benchmark(runner, num_runs, setup),
                                       "input elements", work_product({shard_count,
                                           static_cast<std::uint64_t>(sort_size)}));
        } else if (benchmark_name == "memory_bandwidth") {
            auto workloads = prepare_workloads<benchmark::memory_bandwidth_workload>(
                pool, worker_count, shard_count, [&](std::size_t shard) {
                    return std::make_unique<benchmark::memory_bandwidth_workload>(
                        static_cast<std::size_t>(iterations_by_shard[shard]) * 8);
                });
            auto runner = [&]() {
                run_shards(pool, worker_count, shard_count, [&](std::size_t shard) {
                    workloads[shard]->run();
                });
            };
            // Each element reads a and b, writes c, then reads c for the checksum.
            constexpr std::uint64_t estimated_bytes_per_element = 4 * sizeof(double);
            elapsed = scorer.add_score("memory_bandwidth", benchmark::run_benchmark(runner, num_runs),
                                       "elements processed", work_product({total_effective, 8}),
                                       estimated_bytes_per_element);
        } else if (benchmark_name == "pointer_chase" || benchmark_name == "pointer") {
            auto workloads = prepare_workloads<benchmark::pointer_chase_workload>(
                pool, worker_count, shard_count, [&](std::size_t shard) {
                    return std::make_unique<benchmark::pointer_chase_workload>(
                        static_cast<std::size_t>(iterations_by_shard[shard]));
                });
            auto runner = [&]() {
                run_shards(pool, worker_count, shard_count, [&](std::size_t shard) {
                    workloads[shard]->run();
                });
            };
            elapsed = scorer.add_score("pointer_chase", benchmark::run_benchmark(runner, num_runs),
                                       "pointer hops", total_effective);
        } else if (benchmark_name == "cache_latency" || benchmark_name == "cache") {
            constexpr std::size_t stride = 16;
            auto runner = [&]() {
                run_shards(pool, worker_count, shard_count, [&](std::size_t shard) {
                    benchmark::cache_latency_benchmark(
                        static_cast<std::size_t>(iterations_by_shard[shard]), stride);
                });
            };
            elapsed = scorer.add_score("cache_latency", benchmark::run_benchmark(runner, num_runs),
                                       "pointer hops", work_product({total_effective, 16}));
        } else {
            std::cerr << "Unknown benchmark: " << benchmark_name << "\n";
            continue;
        }

        total_time += elapsed;
        std::cout << benchmark_name << " completed in " << elapsed << " seconds.\n";
        const auto result = scorer.get_scores().back();
        if (const auto rate = result.work_per_second()) {
            std::cout << "Throughput: " << *rate << " " << result.work_unit_ << "/s\n";
        }
        if (const auto bandwidth = result.estimated_gb_per_second()) {
            std::cout << "Estimated logical throughput: " << *bandwidth << " GB/s\n";
        }
        std::cout << "Benchmark score: " << scorer.compute_final_score() << "\n";
    }

    const std::string thread_label = (num_threads > 1) ? "multi-threaded" : "single-threaded";
    std::cout << "\nTotal " << thread_label << " benchmark time: " << total_time << " seconds.\n";
    std::vector<score> scores = scorer.get_scores();
    scores.emplace_back("Combined", scorer.compute_final_score(), 0.0);
    return scores;
}

void benchmark_coordinator::start(const mode& mode)
{
    for (int repeat = 0; repeat < settings_->repeat_count; ++repeat) {
        if (settings_->repeat_count > 1) {
            std::cout << "\n=== Suite repetition " << (repeat + 1) << " of "
                      << settings_->repeat_count << " ===\n";
        }

        switch (mode)
        {
            case mode::single_threaded:
            {
                run_benchmark([&](benchmark_coordinator& b) {
                    return b.run(1);
                });
                break;
            }

            case mode::multi_threaded:
            {
                run_benchmark([&](benchmark_coordinator& b) {
                    return b.run(settings_->thread_count);
                });
                break;
            }

            case mode::both:
            default:
            {
                run_benchmark([&](benchmark_coordinator& b) {
                    return b.run(1);
                });
                run_benchmark([&](benchmark_coordinator& b) {
                    return b.run(settings_->thread_count);
                });
                break;
            }
        }
    }
}

void benchmark_coordinator::start_with_profile(const std::vector<std::string>& benchmark_names, const mode& mode){
    if (benchmark_names.empty()) {
        std::cerr << "No benchmarks specified in profile.\n";
        return;
    }

    std::cout << "Running " << benchmark_names.size() << " benchmarks from profile...\n";

    for (int repeat = 0; repeat < settings_->repeat_count; ++repeat) {
        if (settings_->repeat_count > 1) {
            std::cout << "\n=== Profile repetition " << (repeat + 1) << " of "
                      << settings_->repeat_count << " ===\n";
        }

        switch (mode)
        {
            case mode::single_threaded:
            {
                report_.add_score(run_specific_benchmarks(benchmark_names, 1, settings_->iterations));
                break;
            }

            case mode::multi_threaded:
            {
                report_.add_score(run_specific_benchmarks(benchmark_names, settings_->thread_count,
                                                         settings_->iterations));
                break;
            }

            case mode::both:
            default:
            {
                std::cout << "\n=== Single-threaded Mode ===\n";
                report_.add_score(run_specific_benchmarks(benchmark_names, 1, settings_->iterations));

                std::cout << "\n=== Multi-threaded Mode ===\n";
                report_.add_score(run_specific_benchmarks(benchmark_names, settings_->thread_count,
                                                         settings_->iterations));
                break;
            }
        }
    }
}

std::vector<score> benchmark_coordinator::run(int num_threads) {
    static const std::vector<std::string> all_benchmarks = {
        "floating_point", "integer_arithmetic", "matrix_multiply",
        "branch_prediction", "n_body", "sorting", "memory_bandwidth",
        "pointer_chase", "cache_latency"
    };
    return run_specific_benchmarks(all_benchmarks, num_threads, settings_->iterations);
}
