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
- Throughput in recorded work units per second, plus estimated logical GB/s for memory bandwidth
- RAM capacity, type, and speed in text and JSON reports when available from the OS
- Optional integer and floating-point add, multiply, and divide kernels
- Optional predictable and unpredictable branch kernels with matched taken counts
- Optional cache-size sweep with working-set size and effective pointer-hop latency in reports

## Building
```bash
mkdir build && cd build
cmake ..
make
```

## Usage

Sorting, memory bandwidth, pointer chasing, and the optional branch patterns prepare their inputs before timing. Sorting restores the same input before each warmup and measured run. Timings use a monotonic clock and include worker dispatch overhead. Reports carry a measurement version; comparisons skip older results when the measurement method changes.

`numRuns` controls the number of timed samples per benchmark and suite repetition. Reports show their mean, median, min, max, standard deviation, P95, and P99; JSON reports also include every raw run time. Warmup runs are excluded from these statistics.

Throughput divides the work performed in one measured sample by its mean elapsed time, including worker dispatch overhead. Each benchmark keeps its own work unit, such as loop iterations, pointer hops, or elements processed, so rates with different units are not directly comparable. The memory bandwidth workload also reports estimated logical GB/s using 32 bytes per element: two `double` reads, one `double` write, and one checksum read. This counts operations in the workload, not measured traffic to DRAM; GB means 1,000,000,000 bytes.

Single-threaded and multi-threaded modes process the same logical work shards, based on `--threads`. Reports record each benchmark's total work per measured sample and the full run configuration. A saved text run is compared with the newest earlier run only when the workload, active thread count, profile, scoring settings, build settings, and hardware match. Results from different configurations remain available in their reports without a percentage comparison.

RAM type and speed are reported as `Unavailable` if the operating system does not expose them. Linux reads SMBIOS memory data through `dmidecode`, which may require elevated access. Text reports saved before these fields were added are not considered comparable with new runs.

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
| `memory_bandwidth_max_mib` | Maximum total memory bandwidth working set (MiB) | 256 |
| `cache_sweep_min_kib` | Smallest cache sweep working set (power of two, KiB) | 4 |
| `cache_sweep_max_kib` | Largest cache sweep working set (power of two, KiB; up to 65536) | 32768 |
| `cache_sweep_hops` | Pointer hops per active worker at each size | 262144 |

**Available Benchmarks:**
- `floating_point` - Floating-point operations
- `integer_arithmetic` - Integer arithmetic
- `matrix_multiply` - Matrix multiplication
- `branch_prediction` - Branch prediction
- `branch_predictable`, `branch_unpredictable` - Matched branch patterns for comparison
- `n_body` - N-body simulation
- `sorting` - Sorting algorithms
- `memory_bandwidth` - Memory bandwidth
- `pointer_chase` - Pointer chasing
- `cache_latency` - Cache latency
- `cache_size_sweep` - Random cache-line pointer chasing from 4 KiB through 32 MiB by default
- `integer_add`, `integer_multiply`, `integer_divide` - Basic unsigned 32-bit integer kernels
- `floating_add`, `floating_multiply`, `floating_divide` - Basic double-precision kernels
- `floating_fma` - Double-precision fused multiply-add kernel

The two branch kernels are profile-only and use the same number of decisions and taken branches per shard. `branch_predictable` repeats a short alternating pattern; `branch_unpredictable` shuffles the same values with a fixed seed. Pattern generation happens before timing, and a conditional volatile update keeps the branch in optimized builds. Compare their mean time or throughput with the same build, thread count, and profile. The timings also include input loads and the conditional update, so they are a workload comparison rather than a direct branch-misprediction counter. The existing `branch_prediction` benchmark remains in the default suite with its original measurement method.

```text
name=branch_patterns
benchmarks=branch_predictable,branch_unpredictable
iterations=10000000
repeat_count=1
intensity_multiplier=1
num_runs=5
```

The cache-size sweep is also profile-only. It allocates one randomized, dependent pointer cycle per active worker, with one 64-byte node per cache line, before timing. Each size uses the same hop count and receives five warmups plus `num_runs` measured samples. Reports list the working set **per worker** and effective nanoseconds per hop per active worker; the latter includes thread dispatch and scheduling overhead. In multi-threaded mode, other workers can contend for shared caches and memory. The sweep is diagnostic and does not enter the combined score; a sweep-only report has no combined score. Cache-level boundaries are inferred from changes in the latency curve, not detected directly.

For example:

```text
name=cache_sweep
benchmarks=cache_size_sweep
iterations=3
repeat_count=1
intensity_multiplier=1
num_runs=5
cache_sweep_min_kib=4
cache_sweep_max_kib=32768
cache_sweep_hops=262144
```

The sweep allocates up to `cache_sweep_max_kib` per active worker at once. `iterations` must be at least the desired multi-threaded worker count; `--threads` controls the maximum worker count.

For example, a profile can select only these kernels:

```text
name=arithmetic
benchmarks=integer_add,integer_multiply,integer_divide,floating_add,floating_multiply,floating_divide,floating_fma
iterations=100000000
repeat_count=1
intensity_multiplier=1
matrix_multiply_size=128
num_runs=5
```

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
