cmake_minimum_required(VERSION 3.0)

add_executable(fib-dfs main.cpp)

find_library(TBB_LIB tbb)
find_path(TBB_INC tbb)

target_link_libraries(fib-dfs "${TBB_LIB}")
link_directories(fib-dfs "${TBB_INC}")

