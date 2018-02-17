cmake_minimum_required(VERSION 3.0)

set(target dfs-tbb)

add_executable(${target} main.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g")

find_library(TBB_LIB tbb)
find_path(TBB_INC tbb)

target_link_libraries(${target} "${TBB_LIB}")
link_directories(${target} "${TBB_INC}")

