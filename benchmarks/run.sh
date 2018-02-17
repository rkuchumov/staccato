#!/bin/bash

function get_integer() {
	echo "$1" | grep "$2" | grep -o "[0-9].*"
}

function get_string() {
	echo "$1" | grep "$2" | cut -f2 -d':'
}

function show_results() {
	output=$(cat /dev/stdin)

	sched=$(get_string "$output" "Scheduler")
	name=$(get_string "$output" "Benchmark")
	threads=$(get_integer "$output" "Threads")
	time=$(get_integer "$output" "Time(us)")
	time2=$(get_integer "$output" "Time(us)")
	input=$(get_integer "$output" "Input")

	echo $sched $name $threads $time $input
}

function bench() {
	runs=$1
	dir=$2/$3
	bin=${3}-${2}
	args=$4

	pushd . >/dev/null

	cd $dir

	if [ -d build ]; then
		cd build
	else
		mkdir build
		cd build
	fi
	cmake $cmake_args .. >/dev/null
	make >/dev/null

	for (( i = 0; i < $runs; i++ )); do
		./$bin $args | show_results
	done

	popd >/dev/null
}

runs=1
threads=4

bench $runs cilk fib "$threads 30"
bench $runs cilk dfs "$threads 7 7"
bench $runs cilk mergesort "$threads 10000000"
bench $runs cilk matmul "$threads 1000"
bench $runs cilk blkmul "$threads 6"

bench $runs tbb fib "$threads 30"
bench $runs tbb dfs "$threads 7 7"
bench $runs tbb mergesort "$threads 10000000"
bench $runs tbb matmul "$threads 1"
bench $runs tbb blkmul "$threads 6"

export CXXFLAGS=-I\ ~/.local/include/

bench $runs staccato fib "$threads 30"
bench $runs staccato dfs "$threads 7 7"
bench $runs staccato mergesort "$threads 10000000"
bench $runs staccato matmul "$threads 1"
bench $runs staccato blkmul "$threads 6"
