#pragma once
#include "parse_arguments.hpp"
#include "benchmark.hpp"
#include "benchmark_report.hpp"
#include "file_manager.hpp"
#include <functional>
#include <barrier>

//Move this to a threading util file in the future
class benchmark_thread_pool{
    private:
        int num_threads_;
        std::vector<std::thread> workers_;
        std::function<void()> current_task_;
        std::barrier<> start_barrier_;
        std::barrier<> done_barrier_;
        std::atomic<bool> stop_{false}; 

        void worker_loop();
    public:
        benchmark_thread_pool(int num_threads)
            :   num_threads_(num_threads),
                start_barrier_(num_threads + 1),   // +1 for the main thread
                done_barrier_(num_threads + 1)
            {
                workers_.reserve(static_cast<size_t>(num_threads));
                for (int i = 0; i < num_threads_; ++i) {
                    workers_.emplace_back([this]() { worker_loop(); });
                }
            }

        ~benchmark_thread_pool() {
            stop_.store(true, std::memory_order_release);

            start_barrier_.arrive_and_wait();

            done_barrier_.arrive_and_wait();

            for (auto& t : workers_)
                t.join();
        }

        void run_all(const std::function<void()>& fn);
};

class benchmark_coordinator{
    private:
        const parse_arguments& args_;
        benchmark_report& report_;

        using runner_function = std::function<std::vector<score>(const parse_arguments&, benchmark_coordinator&)>;
    
        //These should be changed to be more useful and customizable.
        static std::vector<score> run_multithreaded_benchmark(const int num_threads, int iterations_per_thread, const int intensity_multiplier = 50, const int matrix_multiply_size = 128, const int num_runs = 10);
        static std::vector<score> run_singlethreaded_benchmark(int iterations, const int intensity_multiplier = 50, const int matrix_multiply_size = 128, const int num_runs = 10);

        void run_benchmark(runner_function runner);

    public:
        benchmark_coordinator(const parse_arguments& args, benchmark_report& report)
            : args_(args), report_(report){}

        void run(const mode& mode);
};      