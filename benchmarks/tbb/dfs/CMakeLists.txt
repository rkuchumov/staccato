cmake_minimum_required(VERSION 3.0)

add_executable(dfs-tbb main.cpp)

find_library(TBB_LIB tbb)
find_path(TBB_INC tbb)

target_link_libraries(dfs-tbb "${TBB_LIB}")
link_directories(dfs-tbb "${TBB_INC}")

