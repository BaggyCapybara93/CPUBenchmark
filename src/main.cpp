#include "benchmark.hpp"
#include "parseArguments.hpp"
#include "benchmarkReport.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    ParseArguments args(argc, argv);
    Benchmark benchmark;

    switch (args.getMode()) {
        case CommandMode::RunSingleThreaded: {
            BenchmarkReport report("./scores_singlethreaded");

            for (size_t i = 0; i < args.getRepeatCount(); i++) {
                const auto& results = benchmark.runSingleThreadedBenchmark(
                    args.getIterations(),
                    args.getIntensityMultiplier(),
                    args.getMatrixMultiplySize()
                );

                report.addScore(results);
            }

            // Print results
            std::cout << "\n=== Single-Threaded Results ===\n";
            for (const auto& s : report.getBenchmarkScores()) {
                std::cout << s.benchmarkName
                          << " | Score: " << s.score
                          << " | Time: " << s.time << "s\n";
            }

            if (args.shouldSaveResults())
                report.saveBenchmark();

            return 0;
        }
        case CommandMode::RunMultiThreaded: {
            BenchmarkReport report("./scores_multithreaded");

            for (size_t i = 0; i < args.getRepeatCount(); i++) {
                const auto& results = benchmark.runMultithreadedBenchmark(
                    args.getThreadCount(),
                    args.getIterations(),
                    args.getIntensityMultiplier(),
                    args.getMatrixMultiplySize()
                );

                report.addScore(results);
            }

            std::cout << "\n=== Multi-Threaded Results ===\n";
            for (const auto& s : report.getBenchmarkScores()) {
                std::cout << s.benchmarkName
                          << " | Score: " << s.score
                          << " | Time: " << s.time << "s\n";
            }
            std::cout << "Combined Score: "
                << report.getCombinedScore()
                << "\n";

            if (args.shouldSaveResults())
                report.saveBenchmark();

            return 0;
        }
        case CommandMode::RunBoth: {

            // --- Single-threaded ---
            BenchmarkReport singleReport("./scores");

            for (size_t i = 0; i < args.getRepeatCount(); i++) {
                const auto& results = benchmark.runSingleThreadedBenchmark(
                    args.getIterations(),
                    args.getIntensityMultiplier(),
                    args.getMatrixMultiplySize()
                );

                singleReport.addScore(results);
            }

            std::cout << "\n=== Single-Threaded Results ===\n";
            for (const auto& s : singleReport.getBenchmarkScores()) {
                std::cout << s.benchmarkName
                          << " | Score: " << s.score
                          << " | Time: " << s.time << "s\n";
            }
            std::cout << "Combined Score: "
                << singleReport.getCombinedScore()
                << "\n";

            // --- Multi-threaded ---
            BenchmarkReport multiReport("./scores");

            for (size_t i = 0; i < args.getRepeatCount(); i++) {
                const auto& results = benchmark.runMultithreadedBenchmark(
                    args.getThreadCount(),
                    args.getIterations(),
                    args.getIntensityMultiplier(),
                    args.getMatrixMultiplySize()
                );

                multiReport.addScore(results);
            }

            std::cout << "\n=== Multi-Threaded Results ===\n";
            for (const auto& s : multiReport.getBenchmarkScores()) {
                std::cout << s.benchmarkName
                          << " | Score: " << s.score
                          << " | Time: " << s.time << "s\n";
            }

            std::cout << "Combined Score: "
                << multiReport.getCombinedScore()
                << "\n";

            if (args.shouldSaveResults()) {
                singleReport.saveBenchmark();
                multiReport.saveBenchmark();
            }

            return 0;
        }

        case CommandMode::Invalid:
            return 1;
    }

    return 0;
}