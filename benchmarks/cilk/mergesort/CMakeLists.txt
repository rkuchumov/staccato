cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcilkplus -lcilkrts -g -O3")

add_executable(mergesort-cilk main.cpp)
