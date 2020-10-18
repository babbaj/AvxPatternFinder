cmake_minimum_required(VERSION 3.13)
project(untitled)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -march=native")

set(CMAKE_CXX_STANDARD 20)
set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG)

add_executable(untitled main.cpp IncrementalAllocator.h ParallelExecutor.h)