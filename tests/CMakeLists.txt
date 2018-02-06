cmake_minimum_required(VERSION 3.0)
project(StaccatoUnitTests)

option(STACCATO_MEMCHECK "Enable memory checks with valgrind" ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")

set(GTEST_DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/gtest_download)

# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in ${GTEST_DOWNLOAD_DIR}/CMakeLists.txt)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
	RESULT_VARIABLE result
	WORKING_DIRECTORY ${GTEST_DOWNLOAD_DIR})
if(result)
	message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} --build .
	RESULT_VARIABLE result
	WORKING_DIRECTORY ${GTEST_DOWNLOAD_DIR})
if(result)
	message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(
	${CMAKE_BINARY_DIR}/googletest-src
	${CMAKE_BINARY_DIR}/googletest-build
)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
	include_directories("${gtest_SOURCE_DIR}/include")
endif()

enable_testing()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

function(my_add_test name sources)
	add_executable(${name} ${sources})
	target_link_libraries(${name} staccato gtest_main gtest gmock)
	add_test(NAME ${name} COMMAND ${name})

	find_program(VALGRIND "valgrind")
	if (TRAIN_MEMCHECK AND VALGRIND)
		add_test(NAME ${name}_memcheck COMMAND ${VALGRIND} ${valgrind_args} ./${name})
	endif()

endfunction()

my_add_test(test_task_deque task_deque.cpp)
my_add_test(test_lifo_allocator lifo_allocator.cpp)
