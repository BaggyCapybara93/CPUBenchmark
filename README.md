# CPUBenchmark

A C++20 tool for comparing CPU performance across single-threaded and multi-threaded runs. It reports timing, throughput, and a relative score for each benchmark.

## Build

Requires CMake 3.10+, Boost.Program_options 1.71+, and nlohmann_json. Use a Release build for meaningful timings:

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

## Run

From the build directory:

```bash
./CPUBenchmark                         # Run both modes
./CPUBenchmark --singleThreaded        # Single-threaded only
./CPUBenchmark --multiThreaded --threads 4 --iterations 10000
./CPUBenchmark --saveMethod json       # Save JSON instead of text
./CPUBenchmark --help                  # Show all options
```

Results are saved in `scores/` beside the executable. `--numRuns` sets the measured samples per benchmark, and `--repeat` repeats the suite. The default memory bandwidth working set is capped at 256 MiB across all shards; change it with `--memoryBandwidthMaxMiB N`.

## Stress testing

```bash
./CPUBenchmark --infinite                         # Repeat using all available threads
./CPUBenchmark --stress --threads 4               # Alias, with four worker threads
./CPUBenchmark --infinite --singleThreaded         # Single-threaded stress test
./CPUBenchmark --infinite --profile=quick.txt      # Repeat only the profile's workloads
```

Stress mode repeats until Ctrl+C (or SIGTERM), then prints the completed cycle count
and elapsed time. Shutdown waits for the current workload/setup call to finish;
large workloads can delay it. Profiles, workload settings, and `--numRuns` still
apply; `--repeat` and a profile's `repeat_count` are ignored. Stress mode defaults
to multi-threaded execution. Results appear in the terminal, with no report files
saved and no accumulation of samples across cycles. This generates sustained
load; it does not add hardware-error detection.

## Profiles

A profile selects benchmarks and their settings. Create a text file such as `quick.txt`:

```text
name=quick
benchmarks=floating_point,integer_arithmetic
iterations=100000
intensity_multiplier=1
num_runs=3
```

Run it with `./CPUBenchmark --profile=quick.txt`. Add `--singleThreaded` or `--multiThreaded` to select one mode. A profile needs a `benchmarks` line; other settings are optional.

The default suite includes `floating_point`, `integer_arithmetic`, `matrix_multiply`, `branch_prediction`, `n_body`, `sorting`, `memory_bandwidth`, `pointer_chase`, and `cache_latency`.

Profiles can also select:

- `integer_add`, `integer_multiply`, `integer_divide`, `floating_add`, `floating_multiply`, `floating_divide`, and `floating_fma`
- `branch_predictable` and `branch_unpredictable` for matched branch patterns
- `cache_size_sweep` for a diagnostic cache latency sweep

Profile settings include `repeat_count`, `matrix_multiply_size`, `memory_bandwidth_max_mib`, `cache_sweep_min_kib`, `cache_sweep_max_kib`, and `cache_sweep_hops`. The default cache sweep spans 4 KiB to 32 MiB per active worker; set `iterations` at least as high as the desired worker count. The sweep does not affect the combined score.

## Reading results

- Each benchmark has five warmups before its measured runs. Reports include mean, median, range, standard deviation, P95, and P99; JSON also includes raw sample times.
- Timings include worker dispatch overhead. Throughput uses each benchmark's own work unit, so rates from different benchmarks are not directly comparable.
- Memory bandwidth makes repeated passes over its bounded arrays when needed. Its reported GB/s is estimated logical traffic, not measured DRAM traffic.
- Text reports compare with earlier runs only when the workload, build, hardware, and scoring settings match.

## License

0BSD (Zero-Clause BSD); see [LICENSE](LICENSE).
