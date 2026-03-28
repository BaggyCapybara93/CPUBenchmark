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

## Requirements
- C++17 compatible compiler
- CMake 3.10+

## License
MIT License - see LICENSE file for details

*Note: This project is in the early stages of development*