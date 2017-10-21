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
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
	message("Release build.")
endif ()

set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(sources
	"src/scheduler.cpp"
	"src/task.cpp"
	"src/task_deque.cpp"
	"src/root_task.cpp"
	"src/lambda_task.cpp"
	"src/worker.cpp"
)

add_library(staccato SHARED ${sources})
target_link_libraries(staccato pthread)

install(TARGETS staccato DESTINATION lib)

set(HEADERS
	include/constants.hpp
	include/scheduler.hpp
	include/task_meta.hpp
	include/task_deque.hpp
	include/worker.hpp
)

install(
	FILES ${HEADERS}
	DESTINATION include/staccato
)

# if (STACCATO_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
# endif()

	add_subdirectory(examples/01-class)
	add_subdirectory(examples/02-lambda)
if (STACCATO_BUILD_TESTS OR STACCATO_BUILD_EXAMPLES)
	add_subdirectory(examples/02-lambda)
endif()

