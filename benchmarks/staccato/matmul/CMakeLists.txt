cmake_minimum_required(VERSION 3.0)

set(target matmul-staccato)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g")

add_executable(${target} main.cpp)

find_path(STACCATO_INC staccato)

target_link_libraries(${target} pthread)
link_directories(${target} "${STACCATO_INC}")

