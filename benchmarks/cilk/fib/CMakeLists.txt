cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcilkplus -lcilkrts")

add_executable(fib-cilk main.cpp)
