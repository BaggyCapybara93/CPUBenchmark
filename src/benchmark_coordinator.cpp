#include "benchmark_coordinator.hpp"
#include <iostream>

//Threading
void benchmark_thread_pool::run_all(const std::function<void()>& fn) {
    current_task_ = fn;

    start_barrier_.arrive_and_wait();

    done_barrier_.arrive_and_wait();
}

void benchmark_thread_pool::worker_loop() {
    while (true) {
        start_barrier_.arrive_and_wait();

        if (stop_) {
            done_barrier_.arrive_and_wait();
            break;
        }

        current_task_();

        done_barrier_.arrive_and_wait();
    }
}



void benchmark_coordinator::run_benchmark(runner_function runner)
{
    std::vector<score> scores = runner(args_, *this);
    report_.add_score(scores);
}

void benchmark_coordinator::run(const mode& mode)
{
    switch (mode)
    {
        case mode::single_threaded:
        {
            run_benchmark([&](const parse_arguments& a, benchmark_coordinator& b) {
                return b.run_singlethreaded_benchmark(
                    a.get_iterations(),
                    a.get_intensity_multiplier(),
                    a.get_matrix_multiply_size(),
                    a.get_num_runs()
                );
            });
            break;
        }

        case mode::multi_threaded:
        {
            run_benchmark([&](const parse_arguments& a, benchmark_coordinator& b) {
                int iterations_per_thread = a.get_iterations() / a.get_thread_count();
                return b.run_multithreaded_benchmark(
                    a.get_thread_count(),
                    iterations_per_thread,
                    a.get_intensity_multiplier(),
                    a.get_matrix_multiply_size(),
                    a.get_num_runs()
                );
            });
            break;
        }

        case mode::both:
        default:
        {
            // Run single-threaded
            run_benchmark([&](const parse_arguments& a, benchmark_coordinator& b) {
                return b.run_singlethreaded_benchmark(
                    a.get_iterations(),
                    a.get_intensity_multiplier(),
                    a.get_matrix_multiply_size(),
                    a.get_num_runs()
                );
            });

            // Run multi-threaded
            run_benchmark([&](const parse_arguments& a, benchmark_coordinator& b) {
                int iterations_per_thread = a.get_iterations() / a.get_thread_count();
                return b.run_multithreaded_benchmark(
                    a.get_thread_count(),
                    iterations_per_thread,
                    a.get_intensity_multiplier(),
                    a.get_matrix_multiply_size(),
                    a.get_num_runs()
                );
            });

            break;
        }
    }
}

std::vector<score>  benchmark_coordinator::run_multithreaded_benchmark(const int num_threads, int iterations_per_thread, const int intensity_multiplier, const int matrix_multiply_size, const int num_runs) {
    scorer scorer;
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

    double nbody_ops = bodies * bodies * steps;
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


    // Return total duration
    double total_time = elapsed_float + elapsed_int + elapsed_matrix + elapsed_branch + elapsed_nbody + elapsed_memory + elapsed_chase;
    std::cout << "Total multi-threaded benchmark time: " << total_time << " seconds.\n";

    return scorer.get_scores();
}

std::vector<score> benchmark_coordinator::run_singlethreaded_benchmark(int iterations, const int intensity_multiplier, const int matrix_multiply_size, const int num_runs){
    scorer scorer;
    iterations *= intensity_multiplier;

    //Floating-point benchmark
    auto float_runner = [&]() {
        benchmark::floating_point_benchmark(iterations);
    };
    double elapsed_float = benchmark::run_benchmark(float_runner, num_runs);
    std::cout << "Floating-point benchmark completed in " << elapsed_float << " seconds.\n";
    scorer.add_score("floating_point", elapsed_float, elapsed_float);

    //Integer arithmetic benchmark
    auto int_runner = [&]() {
        benchmark::integer_arithmetic_benchmark(iterations);
    };

    double elapsed_int = benchmark::run_benchmark(int_runner, num_runs);
    std::cout << "Integer arithmetic benchmark completed in " << elapsed_int << " seconds.\n";
    scorer.add_score("integer", elapsed_int, elapsed_int);

    //Matrix multiply benchmark
    int matrix_size = static_cast<int>(matrix_multiply_size * std::sqrt(static_cast<double>(intensity_multiplier)));
    auto matrix_runner = [&]() {
        benchmark::matrix_multiply_benchmark(matrix_size);
    };

    double elapsed_matrix = benchmark::run_benchmark(matrix_runner, num_runs);
    std::cout << "Matrix multiply benchmark completed in " << elapsed_matrix << " seconds.\n";
    long long matrix_ops = 1LL * matrix_size * matrix_size * matrix_size;
    scorer.add_score("matrix_multiply", static_cast<double>(matrix_ops), elapsed_matrix);

    //Branch prediction benchmark
    auto branch_runner = [&]() {
        benchmark::branch_prediction_benchmark(iterations);
    };
    
    double elapsed_branch = benchmark::run_benchmark(branch_runner, num_runs);
    std::cout << "Branch prediction benchmark completed in " << elapsed_branch << " seconds.\n";
    scorer.add_score("branch", elapsed_branch, elapsed_branch);

    //NBodies Benchmark
    int steps = 10;
    int bodies = static_cast<int>(std::sqrt(iterations));
    auto nbody_runner = [&]() {
        benchmark::n_body_benchmark(bodies, steps);
    };
    
    double elapsed_nbody = benchmark::run_benchmark(nbody_runner, num_runs);
    double nbody_ops = bodies * bodies * steps;
    std::cout << "Nbody benchmark completed in " << elapsed_nbody << " seconds.\n";
    scorer.add_score("n_body", nbody_ops, elapsed_nbody);
    
    //Sorting benchmark 
    int sort_size = static_cast<int>(50000 * std::sqrt(static_cast<double>(intensity_multiplier)));
    auto sort_runner = [&]() {
        benchmark::sorting_benchmark(sort_size);
    };

    double elapsed_sort = benchmark::run_benchmark(sort_runner, num_runs);
    double sort_ops = 1.0 * sort_size * std::log2(static_cast<double>(sort_size));
    std::cout << "Sorting benchmark completed in " << elapsed_sort << " seconds.\n";
    scorer.add_score("sorting", sort_ops, elapsed_sort);

    // Memory Bandwidth Benchmark (STREAM-like)
    std::size_t mem_size = static_cast<std::size_t>(iterations) * 8; 

    auto memory_runner = [&]() {
        benchmark::memory_bandwidth_benchmark(mem_size);
    };

    double bytes_processed = 3.0 * static_cast<double>(mem_size) * sizeof(double);

    double elapsed_memory = benchmark::run_benchmark(memory_runner, num_runs);
    std::cout << "Memory bandwidth benchmark completed in " << elapsed_memory << " seconds.\n";

    scorer.add_score("memory_bandwidth", bytes_processed, elapsed_memory);

    //Pointer Chase benchmark
    std::size_t chase_size = static_cast<std::size_t>(iterations);

    auto pointer_chase_runner = [&]() {
        benchmark::pointer_chase_benchmark(chase_size);
    };

    // Each iteration performs exactly 1 dependent pointer dereference
    double ops_processed = static_cast<double>(chase_size);

    double elapsed_chase = benchmark::run_benchmark(pointer_chase_runner, num_runs);
    std::cout << "Pointer chase benchmark completed in " << elapsed_chase << " seconds.\n";

    scorer.add_score("pointer_chase", ops_processed, elapsed_chase);


    double total_time = elapsed_float + elapsed_int + elapsed_matrix + elapsed_branch + elapsed_nbody + elapsed_memory + elapsed_chase;
    std::cout << "Total single-threaded benchmark time: " << total_time << " seconds.\n";

    return scorer.get_scores();
}