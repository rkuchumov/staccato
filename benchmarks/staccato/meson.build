benchs = [
	['fib', ['fib.cpp']],
	['nfib', ['nfib.cpp']],
	['dfs', ['dfs.cpp']],
	['blkmul', ['blkmul.cpp']],
	['matmul', ['matmul.cpp']],
]

foreach t : benchs
	exe = executable(
		t[0],
		t[1],
		include_directories : incdir,
		dependencies : deps,
	)

	test(t[0], exe)
endforeach
