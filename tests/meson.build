tests = [
	# ['task_mailbox', ['task_mailbox.cc']],
]

add_languages('cpp')

gtest_dep = dependency('gtest', main : true, required : true)

if gtest_dep.found()
	deps += gtest_dep
	deps += dependency('gmock')

	foreach t : tests
		exe = executable(
			t[0],
			t[1],
			include_directories : incdir,
			dependencies : deps,
		)

		test(t[0], exe)
	endforeach
endif


