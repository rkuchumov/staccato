cmake_minimum_required(VERSION 3.0)

add_executable(fib-cilk main.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcilkplus -lcilkrts")

target_link_libraries(fib-cilk "${STACCATO_LIB}")
link_directories(fib-cilk "${STACCATO_INC}")

