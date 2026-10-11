# CPUBenchmark

A simple C++ CPU benchmark tool for measuring system performance.

## Features
- Multi-threaded and single-threaded benchmark modes
- CPU-intensive benchmarks including:
  - Floating-point arithmetic
  - Integer arithmetic
  - Matrix multiplication
  - Branch prediction
  - N-body simulation
  - Sorting algorithms
- Configurable parameters (threads, iterations, intensity)
- Score calculation and reporting (relative performance metrics)

## Building
```bash
mkdir build && cd build
cmake ..
make
```

## Usage

Sorting, memory bandwidth, and pointer chasing prepare their inputs before timing. Sorting restores the same input before each warmup and measured run. Timings use a monotonic clock and include worker dispatch overhead. Reports carry a measurement version; comparisons skip older results when the measurement method changes.

`numRuns` controls the number of timed samples per benchmark and suite repetition. Reports show their mean, median, min, max, standard deviation, P95, and P99; JSON reports also include every raw run time. Warmup runs are excluded from these statistics.

Single-threaded and multi-threaded modes process the same logical work shards, based on `--threads`. Reports record each benchmark's total work per measured sample and the full run configuration. A saved text run is compared with the newest earlier run only when the workload, active thread count, profile, scoring settings, build settings, and hardware match. Results from different configurations remain available in their reports without a percentage comparison.

### Basic Usage
```bash
# Run both single-threaded and multi-threaded benchmarks
./CPUBenchmark

# Run single-threaded only
./CPUBenchmark --singleThreaded

# Run multi-threaded with custom parameters
./CPUBenchmark --multiThreaded --threads 4 --iterations 10000

# Show help
./CPUBenchmark --help
```

### Benchmark Profiles

Profiles allow you to create custom benchmark configurations that define which benchmarks to run and their execution parameters.

#### Creating a Profile

Create a `.txt` file in the `profiles/` directory:

```
name=quick_test
benchmarks=floating_point,integer_arithmetic,branch_prediction
iterations=100000
repeat_count=1
intensity_multiplier=10
matrix_multiply_size=64
num_runs=3
```

**Profile Fields:**
| Field | Description | Default |
|-------|-------------|---------|
| `name` | Profile name | "default" |
| `benchmarks` | Comma-separated benchmark names | All benchmarks |
| `iterations` | Iterations per thread | 999999 |
| `repeat_count` | Repeat count | 1 |
| `intensity_multiplier` | Intensity multiplier | 50 |
| `matrix_multiply_size` | Matrix size | 128 |
| `num_runs` | Number of runs | 5 |

**Available Benchmarks:**
- `floating_point` - Floating-point operations
- `integer_arithmetic` - Integer arithmetic
- `matrix_multiply` - Matrix multiplication
- `branch_prediction` - Branch prediction
- `n_body` - N-body simulation
- `sorting` - Sorting algorithms
- `memory_bandwidth` - Memory bandwidth
- `pointer_chase` - Pointer chasing
- `cache_latency` - Cache latency

#### Using Profiles

```bash
# Run with a profile (both modes)
./CPUBenchmark --profile=profiles/quick_test.txt

# Run with specific mode
./CPUBenchmark --profile=profiles/quick_test.txt --singleThreaded
./CPUBenchmark --profile=profiles/quick_test.txt --multiThreaded

# Example: Quick test profile
name=quick_test
benchmarks=floating_point,integer_arithmetic,branch_prediction
iterations=100000
repeat_count=1
intensity_multiplier=10
matrix_multiply_size=64
num_runs=3

# Example: Full suite profile
name=full_suite
benchmarks=floating_point,integer_arithmetic,matrix_multiply,branch_prediction,n_body,sorting,memory_bandwidth,pointer_chase,cache_latency
iterations=999999
repeat_count=1
intensity_multiplier=50
matrix_multiply_size=128
num_runs=5
```

## Requirements
- C++20 compatible compiler
- CMake 3.10+

## License
MIT License - see LICENSE file for details

*Note: This project is in the early stages of development*
