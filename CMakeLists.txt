cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
project(CPUBenchmark)

# Strict compile rules
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion -Wsign-conversion -Wcast-qual")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wshadow")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat=2 -Wformat-security")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")

# Optimization flags for CPU benchmarks
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")

set(SOURCE_FILES
    src/benchmark.cpp
    src/benchmarkCoordinator.cpp
    src/benchmarkReport.cpp
    src/main.cpp
    src/parseArguments.cpp
    src/scorer.cpp
)

add_executable(CPUBenchmark ${SOURCE_FILES})

find_package(Boost 1.71.0 CONFIG COMPONENTS program_options)

target_include_directories(CPUBenchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(CPUBenchmark PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(CPUBenchmark PRIVATE Boost::program_options)