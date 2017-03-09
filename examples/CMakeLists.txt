cmake_minimum_required(VERSION 2.8)

project("Staccato C++ task scheduler: examples")

if (CMAKE_BUILD_TYPE EQUAL Debug)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

	add_definitions(-DSTACCATO_DEBUG=1)
endif()

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../build/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include/)

add_executable(fib fib.cpp)

target_link_libraries(fib libstaccato.so)
