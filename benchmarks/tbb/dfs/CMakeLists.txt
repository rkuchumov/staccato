cmake_minimum_required(VERSION 2.8)

set(target dfs-tbb)

add_executable(${target} main.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g")

find_library(TBB_LIB tbb)
find_path(TBB_INC tbb)

target_link_libraries(${target} "${TBB_LIB}")
link_directories(${target} "${TBB_INC}")

find_library(TBBMALLOC_LIB tbbmalloc_proxy)
target_link_libraries(${target} "${TBBMALLOC_LIB}")

find_path(TBBMALLOC_INC tbbmalloc_proxy)
link_directories(${target} "${TBBMALLOC_INC}")

