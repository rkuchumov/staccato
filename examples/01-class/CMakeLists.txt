cmake_minimum_required(VERSION 3.0)

add_executable(fib-class fib-class.cpp)

target_link_libraries(fib-class pthread)

