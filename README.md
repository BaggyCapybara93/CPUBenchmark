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

## Building
```bash
mkdir build && cd build
cmake ..
make
```

## Usage

Sorting, memory bandwidth, and pointer chasing prepare their inputs before timing. Sorting restores the same input before each warmup and measured run. Timings use a monotonic clock and include worker dispatch overhead. Reports carry a measurement version; comparisons skip older results when the measurement method changes.

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
- `integer_add`, `integer_multiply`, `integer_divide` - Basic unsigned 32-bit integer kernels
- `floating_add`, `floating_multiply`, `floating_divide` - Basic double-precision kernels

The six arithmetic kernels are available in profiles and are not part of the default full suite, so existing default scores retain their benchmark mix. Each uses eight independent accumulator lanes and cache-resident, deterministic operands. The integer divide kernel varies its dividend from the accumulator to keep the division in each loop iteration; its reported work count includes divisions, while the accumulator update is overhead. Floating-point factors are paired with reciprocals so long multiply and divide runs remain finite. Work counts are rounded up to whole groups of eight operations per shard. Results include loop, input-load, and thread-dispatch overhead, and Release builds may auto-vectorize them. Use them for relative comparisons with the same build and profile rather than as isolated instruction latency measurements.

For example, a profile can select only these kernels:

```text
name=arithmetic
benchmarks=integer_add,integer_multiply,integer_divide,floating_add,floating_multiply,floating_divide
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
