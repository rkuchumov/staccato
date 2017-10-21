cmake_minimum_required(VERSION 3.0)

add_executable(dfs-cilk main.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcilkplus -lcilkrts")

target_link_libraries(dfs-cilk "${STACCATO_LIB}")
link_directories(dfs-cilk "${STACCATO_INC}")

