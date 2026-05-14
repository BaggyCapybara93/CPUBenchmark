cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(CPUBenchmark)


set(SOURCE_FILES
    src/benchmark.cpp
    src/benchmarkCoordinator.cpp
    src/benchmarkReport.cpp
    src/main.cpp
    src/parseArguments.cpp
    src/scorer.cpp
)

add_executable(CPUBenchmark ${SOURCE_FILES})


target_include_directories(CPUBenchmark
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
