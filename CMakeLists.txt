cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(CPUBenchmark)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(CPUBenchmark ${SOURCES})

target_include_directories(CPUBenchmark
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
