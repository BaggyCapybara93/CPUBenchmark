#include "benchmark_coordinator.hpp"
#include <iostream>
#include <memory>

//Threading
void benchmark_thread_pool::run_all(const std::function<void()>& fn) {
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

void benchmark_thread_pool::worker_loop() {
    while (true) {
        start_barrier_.arrive_and_wait();

        if (stop_) {
            done_barrier_.arrive_and_wait();
            break;
        }

        try {
            current_task_();
        } catch (...) {
            std::lock_guard<std::mutex> lock(worker_exception_mutex_);
            if (!worker_exception_) {
                worker_exception_ = std::current_exception();
            }
        }

        done_barrier_.arrive_and_wait();
    }
}

void benchmark_coordinator::run_benchmark(runner_function runner)
{
    std::vector<score> scores = runner(*this);
    report_.add_score(scores);
}

std::vector<score> benchmark_coordinator::run_specific_benchmarks(
    const std::vector<std::string>& benchmark_names, int num_threads, int iterations_per_thread) {
    scorer scorer;
    scorer.load_weights(std::make_shared<std::unordered_map<std::string, double>>(settings_.get()->weights));
    scorer.load_baseline_times(std::make_shared<std::unordered_map<std::string, double>>(settings_.get()->baseline_times));
    
    int intensity_multiplier = settings_.get()->intensity_multiplier;
    int matrix_multiply_size = settings_.get()->matrix_multiply_size;
    int num_runs = settings_.get()->num_runs;

    iterations_per_thread *= intensity_multiplier;
    benchmark_thread_pool pool(num_threads);
    double total_time = 0.0;

    for (const auto& benchmark_name : benchmark_names) {
        std::cout << "\n=== Running benchmark: " << benchmark_name << " ===\n";
        
        double elapsed = 0.0;

        if (benchmark_name == "floating_point") {
            auto float_runner = [&]() {
                pool.run_all([&]() {
                    benchmark::floating_point_benchmark(iterations_per_thread);
                });
            };
            elapsed = benchmark::run_benchmark(float_runner, num_runs);
            std::cout << "Floating-point benchmark completed in " << elapsed << " seconds.\n";
            scorer.add_score("floating_point", elapsed, elapsed);
        }
        else if (benchmark_name == "integer_arithmetic" || benchmark_name == "integer") {
            auto int_runner = [&]() {
                pool.run_all([&]() {
                    benchmark::integer_arithmetic_benchmark(iterations_per_thread);
                });
            };
            elapsed = benchmark::run_benchmark(int_runner, num_runs);
            std::cout << "Integer arithmetic benchmark completed in " << elapsed << " seconds.\n";
            scorer.add_score("integer", elapsed, elapsed);
        }
        else if (benchmark_name == "matrix_multiply") {
            int actual_matrix_size = static_cast<int>(matrix_multiply_size * std::sqrt(static_cast<double>(intensity_multiplier)));
            auto matrix_runner = [&]() {
                pool.run_all([&]() {
                    benchmark::matrix_multiply_benchmark(actual_matrix_size);
                });
            };
            elapsed = benchmark::run_benchmark(matrix_runner, num_runs);
            long long matrix_ops = 1LL * actual_matrix_size * actual_matrix_size * actual_matrix_size * num_threads;
            std::cout << "Matrix multiply benchmark completed in " << elapsed << " seconds.\n";
            scorer.add_score("matrix_multiply", static_cast<double>(matrix_ops), elapsed);
        }
        else if (benchmark_name == "branch_prediction" || benchmark_name == "branch") {
            auto branch_runner = [&]() {
                pool.run_all([&]() {
                    benchmark::branch_prediction_benchmark(iterations_per_thread);
                });
            };
            elapsed = benchmark::run_benchmark(branch_runner, num_runs);
            std::cout << "Branch prediction benchmark completed in " << elapsed << " seconds.\n";
            scorer.add_score("branch", elapsed, elapsed);
        }
        else if (benchmark_name == "n_body") {
            int steps = 10;
            int bodies = static_cast<int>(std::sqrt(iterations_per_thread));
            auto nbody_runner = [&]() {
                pool.run_all([&]() {
                    benchmark::n_body_benchmark(bodies, steps);
                });
            };
            double nbody_ops = static_cast<double>(bodies) * bodies * steps;
            elapsed = benchmark::run_benchmark(nbody_runner, num_runs);
            std::cout << "NBody benchmark completed in " << elapsed << " seconds.\n";
            scorer.add_score("n_body", nbody_ops, elapsed);
        }
        else if (benchmark_name == "sorting") {
            int sort_size = 50000 * static_cast<int>(std::sqrt(intensity_multiplier));
            auto sort_runner = [&]() {
                pool.run_all([&]() {
                    benchmark::sorting_benchmark(sort_size);
                });
            };
            double sort_ops = 1.0 * sort_size * std::log2(static_cast<double>(sort_size));
            elapsed = benchmark::run_benchmark(sort_runner, num_runs);
            std::cout << "Sorting benchmark completed in " << elapsed << " seconds.\n";
            scorer.add_score("sorting", sort_ops, elapsed);
        }
        else if (benchmark_name == "memory_bandwidth") {
            std::size_t mem_size = static_cast<std::size_t>(iterations_per_thread) * 8;
            auto memory_runner = [&]() {
                pool.run_all([&]() {
                    benchmark::memory_bandwidth_benchmark(mem_size);
                });
            };
            double bytes_processed = static_cast<double>(mem_size) * static_cast<double>(sizeof(double)) * 3.0 * static_cast<double>(num_threads);
            elapsed = benchmark::run_benchmark(memory_runner, num_runs);
            std::cout << "Memory bandwidth benchmark completed in " << elapsed << " seconds.\n";
            scorer.add_score("memory_bandwidth", bytes_processed, elapsed);
        }
        else if (benchmark_name == "pointer_chase" || benchmark_name == "pointer") {
            std::size_t chase_size = static_cast<std::size_t>(iterations_per_thread);
            auto pointer_chase_runner = [&]() {
                pool.run_all([&]() {
                    benchmark::pointer_chase_benchmark(chase_size);
                });
            };
            double ops_processed_ptr = static_cast<double>(chase_size) * static_cast<double>(num_threads);
            elapsed = benchmark::run_benchmark(pointer_chase_runner, num_runs);
            std::cout << "Pointer chase benchmark completed in " << elapsed << " seconds.\n";
            scorer.add_score("pointer_chase", ops_processed_ptr, elapsed);
        }
        else if (benchmark_name == "cache_latency" || benchmark_name == "cache") {
            std::size_t cache_size = static_cast<std::size_t>(iterations_per_thread);
            std::size_t cache_stride = 16;
            auto cache_latency_runner = [&]() {
                pool.run_all([&]() {
                    benchmark::cache_latency_benchmark(cache_size, cache_stride);
                });
            };
            double cache_ops_processed = static_cast<double>(cache_size) * static_cast<double>(num_threads);
            elapsed = benchmark::run_benchmark(cache_latency_runner, num_runs);
            std::cout << "Cache latency benchmark completed in " << elapsed << " seconds.\n";
            scorer.add_score("cache_latency", cache_ops_processed, elapsed);
        }
        else {
            std::cerr << "Unknown benchmark: " << benchmark_name << "\n";
            continue;
        }

        total_time += elapsed;
        std::cout << "Benchmark score: " << scorer.compute_final_score() << "\n";
    }

    const std::string thread_label = (num_threads > 1) ? "multi-threaded" : "single-threaded";
    std::cout << "\nTotal " << thread_label << " benchmark time: " << total_time << " seconds.\n";
    return scorer.get_scores();
}

void benchmark_coordinator::start(const mode& mode)
{
    switch (mode)
    {
        case mode::single_threaded:
        {
            run_benchmark([&](benchmark_coordinator& b) {
                return b.run(
                    1,  // num_threads
                    settings_.get()->iterations,
                    settings_.get()->intensity_multiplier,
                    settings_.get()->matrix_multiply_size,
                    settings_.get()->num_runs
                );
            });
            break;
        }

        case mode::multi_threaded:
        {
            run_benchmark([&](benchmark_coordinator& b) {
                int iterations_per_thread = settings_.get()->iterations / settings_.get()->thread_count;
                return b.run(
                    settings_.get()->thread_count,
                    iterations_per_thread,
                    settings_.get()->intensity_multiplier,
                    settings_.get()->matrix_multiply_size,
                    settings_.get()->num_runs
                );
            });
            break;
        }

        case mode::both:
        default:
        {
            // Run single-threaded
            run_benchmark([&](benchmark_coordinator& b) {
                return b.run(
                    1,  // num_threads
                    settings_.get()->iterations,
                    settings_.get()->intensity_multiplier,
                    settings_.get()->matrix_multiply_size,
                    settings_.get()->num_runs
                );
            });

            // Run multi-threaded
            run_benchmark([&](benchmark_coordinator& b) {
                int iterations_per_thread = settings_.get()->iterations / settings_.get()->thread_count;
                return b.run(
                    settings_.get()->thread_count,
                    iterations_per_thread,
                    settings_.get()->intensity_multiplier,
                    settings_.get()->matrix_multiply_size,
                    settings_.get()->num_runs
                );
            });

            break;
        }
    }
}

void benchmark_coordinator::start_with_profile(const std::vector<std::string>& benchmark_names, const mode& mode){
    if (benchmark_names.empty()) {
        std::cerr << "No benchmarks specified in profile.\n";
        return;
    }

    std::cout << "Running " << benchmark_names.size() << " benchmarks from profile...\n";

    switch (mode)
    {
        case mode::single_threaded:
        {
            report_.add_score(run_specific_benchmarks(benchmark_names, 1, settings_->iterations));
            break;
        }

        case mode::multi_threaded:
        {
            const int iterations_per_thread = settings_->iterations / settings_->thread_count;
            report_.add_score(run_specific_benchmarks(benchmark_names, settings_->thread_count, iterations_per_thread));
            break;
        }

        case mode::both:
        default:
        {
            // Run single-threaded first
            std::cout << "\n=== Single-threaded Mode ===\n";
            report_.add_score(run_specific_benchmarks(benchmark_names, 1, settings_->iterations));

            // Run multi-threaded
            std::cout << "\n=== Multi-threaded Mode ===\n";
            const int iterations_per_thread = settings_->iterations / settings_->thread_count;
            report_.add_score(run_specific_benchmarks(benchmark_names, settings_->thread_count, iterations_per_thread));
            break;
        }
    }
}

std::vector<score> benchmark_coordinator::run(const int num_threads, int iterations_per_thread, const int intensity_multiplier, const int matrix_multiply_size, const int num_runs) {
    scorer scorer;
    scorer.load_weights(std::make_shared<std::unordered_map<std::string, double>>(settings_.get()->weights));
    scorer.load_baseline_times(std::make_shared<std::unordered_map<std::string, double>>(settings_.get()->baseline_times));
    iterations_per_thread *= intensity_multiplier;

    benchmark_thread_pool pool(num_threads);

    // Run floating-point benchmark first
    auto float_runner = [&]() {
        pool.run_all([&]() {
            benchmark::floating_point_benchmark(iterations_per_thread);
        });
    };

    double elapsed_float = benchmark::run_benchmark(float_runner, num_runs);
    std::cout << "Floating-point benchmark completed in " << elapsed_float << " seconds.\n";
    scorer.add_score("floating_point", elapsed_float, elapsed_float);

    // Integer arithmetic benchmark 
    auto int_runner = [&]() {
        pool.run_all([&]() {
            benchmark::integer_arithmetic_benchmark(iterations_per_thread);
        });
    };

    double elapsed_int = benchmark::run_benchmark(int_runner, num_runs);
    std::cout << "Integer arithmetic benchmark completed in " << elapsed_int << " seconds.\n";
    scorer.add_score("integer", elapsed_int, elapsed_int);

    //Matrix Multiplication Benchmark
    int matrix_size = static_cast<int>(matrix_multiply_size * std::sqrt(static_cast<double>(intensity_multiplier)));
    auto matrix_runner = [&]() {
        pool.run_all([&]() {
            benchmark::matrix_multiply_benchmark(matrix_size);
        });
    };

    double elapsed_matrix = benchmark::run_benchmark(matrix_runner, num_runs);
    long long matrix_ops = 1LL * matrix_size * matrix_size * matrix_size * num_threads; // actual number of operations
    std::cout << "Matrix multiply benchmark completed in " << elapsed_matrix << " seconds.\n";
    scorer.add_score("matrix_multiply", static_cast<double>(matrix_ops), elapsed_matrix);

    //Branch Prediction Benchmark
    auto branch_runner = [&]() {
        pool.run_all([&]() {
            benchmark::branch_prediction_benchmark(iterations_per_thread);
        });
    };

    double elapsed_branch = benchmark::run_benchmark(branch_runner, num_runs);
    std::cout << "Branch prediction benchmark completed in " << elapsed_branch << " seconds.\n";
    scorer.add_score("branch", elapsed_branch, elapsed_branch);

    //Nbody Benchmark
    int steps = 10;
    int bodies = static_cast<int>(std::sqrt(iterations_per_thread));

    auto nbody_runner = [&]() {
        pool.run_all([&]() {
            benchmark::n_body_benchmark(bodies, steps);
        });
    };

    double nbody_ops = static_cast<double>(bodies) * static_cast<double>(bodies) * static_cast<double>(steps);
    double elapsed_nbody = benchmark::run_benchmark(nbody_runner, num_runs);
    std::cout << "NBody benchmark completed in " << elapsed_nbody << " seconds.\n";
    scorer.add_score("n_body", nbody_ops, elapsed_nbody);

    //Sorting Benchmark
    int sort_size = 50000 * static_cast<int>(std::sqrt(intensity_multiplier));
    auto sort_runner = [&]() {
        pool.run_all([&]() {
            benchmark::sorting_benchmark(sort_size);
        });
    };

    double sort_ops = 1.0 * sort_size * std::log2(static_cast<double>(sort_size));
    double elapsed_sort = benchmark::run_benchmark(sort_runner, num_runs);
    std::cout << "Sorting benchmark completed in " << elapsed_sort << " seconds.\n";
    scorer.add_score("sorting", sort_ops, elapsed_sort);

    // Memory Bandwidth Benchmark 
    std::size_t mem_size = static_cast<std::size_t>(iterations_per_thread) * 8; 

    auto memory_runner = [&]() {
        pool.run_all([&]() {
            benchmark::memory_bandwidth_benchmark(mem_size);
        });
    };
    double bytes_processed = static_cast<double>(mem_size) * static_cast<double>(sizeof(double)) * 3.0 * static_cast<double>(num_threads);

    double elapsed_memory = benchmark::run_benchmark(memory_runner, num_runs);
    std::cout << "Memory bandwidth benchmark completed in " << elapsed_memory << " seconds.\n";

    scorer.add_score("memory_bandwidth", bytes_processed, elapsed_memory);

    // Pointer Chasing Benchmark
    std::size_t chase_size = static_cast<std::size_t>(iterations_per_thread);

    auto pointer_chase_runner = [&]() {
        pool.run_all([&]() {
            benchmark::pointer_chase_benchmark(chase_size);
        });
    };

    // Each iteration performs exactly 1 dependent pointer dereference
    double ops_processed = static_cast<double>(chase_size) * static_cast<double>(num_threads);

    double elapsed_chase = benchmark::run_benchmark(pointer_chase_runner, num_runs);
    std::cout << "Pointer chase benchmark completed in " << elapsed_chase << " seconds.\n";

    scorer.add_score("pointer_chase", ops_processed, elapsed_chase);

    // Cache Latency Benchmark
    std::size_t cache_size  = static_cast<std::size_t>(iterations_per_thread);
    std::size_t cache_stride = 16; 

    auto cache_latency_runner = [&]() {
        pool.run_all([&]() {
            benchmark::cache_latency_benchmark(cache_size, cache_stride);
        });
    };

    // Each iteration performs exactly 1 dependent load
    double cache_ops_processed =
        static_cast<double>(cache_size) * static_cast<double>(num_threads);

    double elapsed_cache = benchmark::run_benchmark(cache_latency_runner, num_runs);

    std::cout << "Cache latency benchmark completed in "
            << elapsed_cache << " seconds.\n";

    scorer.add_score("cache_latency", cache_ops_processed, elapsed_cache);

    // Return total duration
    double total_time = elapsed_float + elapsed_int + elapsed_matrix + elapsed_branch + elapsed_nbody + elapsed_sort + elapsed_memory + elapsed_chase + elapsed_cache;
    const std::string thread_label = (num_threads > 1) ? "multi-threaded" : "single-threaded";
    std::cout << "Total " << thread_label << " benchmark time: " << total_time << " seconds.\n";

    return scorer.get_scores();
}
