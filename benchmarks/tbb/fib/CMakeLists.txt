cmake_minimum_required(VERSION 3.0)

add_executable(fib-tbb main.cpp)

find_library(TBB_LIB tbb)
find_path(TBB_INC tbb)

target_link_libraries(fib-tbb "${TBB_LIB}")
link_directories(fib-tbb "${TBB_INC}")

