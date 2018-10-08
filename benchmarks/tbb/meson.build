benchs = [
	['fib', ['fib.cpp']],
	['dfs', ['dfs.cpp']],
	['blkmul', ['blkmul.cpp']],
	['matmul', ['matmul.cpp']],
]

tbbdeps = [ dependency('tbb'), dependency('tbbmalloc_proxy') ]

foreach t : benchs
	exe = executable(
		t[0],
		t[1],
		dependencies : deps + tbbdeps,
		cpp_args : cxxflags
	)

	test(t[0], exe)
endforeach
