#!/bin/bash

runs=6
threads=({1..56})

args_fib="42"
args_dfs="9 9"
args_mergesort="50000000"
args_matmul="3000"
args_blkmul="8"

runs=1
threads=(4)

args_fib="35"
args_dfs="7 7"
args_mergesort="100000"
args_matmul="800"
args_blkmul="6"

benchmarks=(
	"staccato fib _threads_ $args_fib"
	"staccato dfs _threads_ $args_dfs"
	"staccato mergesort _threads_ $args_mergesort"
	"staccato matmul _threads_ $args_matmul"
	"staccato blkmul _threads_ $args_blkmul"
	"cilk fib _threads_ $args_fib"
	"cilk dfs _threads_ $args_dfs"
	"cilk mergesort _threads_ $args_mergesort"
	"cilk matmul _threads_ $args_matmul"
	"cilk blkmul _threads_ $args_blkmul"
)

# export CXXFLAGS=-I\ ~/.local/include/\ -DSTACCATO_DEBUG=1
export CXXFLAGS=-I\ ~/.local/include/

function get_integer() {
	echo "$1" | grep "$2" | grep -o "[0-9].*"
}

function get_string() {
	echo "$1" | grep "$2" | cut -f2 -d':'
}

function print_header() {
	echo sched name threads time input output
}

function show_results() {
	output=$(cat /dev/stdin)

	sched=$(get_string "$output" "Scheduler")
	name=$(get_string "$output" "Benchmark")
	threads=$(get_integer "$output" "Threads")
	time=$(get_integer "$output" "Time(us)")
	input=$(get_integer "$output" "Input")
	output=$(get_integer "$output" "Output")

	echo $sched $name $threads $time \"$input\" \"$output\"
}

function clean() {
	dir=$1/$2/build

	if [ -d $dir ]; then
		rm -rf $dir
	fi
}

function build() {
	dir=$1/$2

	pushd .

	cd $dir

	if [ -d build ]; then
		cd build
	else
		mkdir build
		cd build
	fi

	cmake $cmake_args ..
	make

	popd
}

function bench() {
	runs=$1
	dir=$2/$3/build/
	bin=${3}-${2}
	shift; shift; shift
	args=$@

	pushd . >/dev/null

	cd $dir

	for (( i = 0; i < $runs; i++ )); do
		./$bin $args | show_results
	done

	popd >/dev/null
}

function clean_all() {
	for b in "${benchmarks[@]}" ; do
		clean $b
	done
}

function build_all() {
	for b in "${benchmarks[@]}" ; do
		build $b
	done
}

function bench_all() {
	print_header
	for id in "${!benchmarks[@]}" ; do
		benchmark="${benchmarks[$id]}"

		for t in ${threads[@]} ; do
			b=${benchmark/_threads_/$t}
			bench $runs $b 
		done
	done
}

function install_staccato() {
	pushd .
	cd ..

	rm -rf build || true
	mkdir build
	cd build
	cmake -DCMAKE_INSTALL_PREFIX=~/.local/ ..
	make
	make install

	popd
}

mode=$1
if [[ z$mode == "zupdate" ]]; then
	install_staccato
	clean_all
	build_all
elif [[ z$mode == "zrebuild" ]]; then
	clean_all
	build_all
elif [[ z$mode == "zbuild" ]]; then
	build_all
elif [[ z$mode == "zclean" ]]; then
	clean_all
else
	bench_all
fi

