cmake_minimum_required(VERSION 3.0)

project("Staccato C++ task scheduler")

option(STACCATO_BUILD_EXAMPLES "Build example programs" OFF)
option(STACCATO_BUILD_TESTS "Build all tests" OFF)

if (NOT CMAKE_BUILD_TYPE)
	set (CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_BUILD_TYPE MATCHES Debug)
	message("Debug build.")

	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

	add_definitions(-DSTACCATO_DEBUG=1)
	set(CMAKE_VERBOSE_MAKEFILE TRUE)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
	message("Release build.")
endif ()

set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(HEADERS
	include/constants.hpp
	include/scheduler.hpp
	include/task.hpp
	include/task_deque.hpp
	include/lifo_allocator.hpp
	include/worker.hpp
	include/utils.hpp
)

install(
	FILES ${HEADERS}
	DESTINATION include/staccato
)

enable_testing()
add_subdirectory(examples/01-class)
add_subdirectory(tests)

