cmake_minimum_required(VERSION 2.8)

project("Staccato C++ task scheduler")

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
	message("Release build.")
else ()
	message("Some other build type.")
endif ()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(staccato SHARED 
	./src/scheduler.cpp
	./src/task.cpp
	./src/task_deque.cpp
	./src/worker.cpp
)

target_link_libraries(staccato pthread)
