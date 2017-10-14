cmake_minimum_required(VERSION 3.0)

add_executable(dfs-staccato main.cpp)

find_library(STACCATO_LIB staccato)
find_path(STACCATO_INC staccato)

target_link_libraries(dfs-staccato "${STACCATO_LIB}")
link_directories(dfs-staccato "${STACCATO_INC}")

