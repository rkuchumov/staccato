project(
	'libstaccato',
	'cpp',
	default_options : [
		'buildtype=release'
	]
)

cpp = meson.get_compiler('cpp')

incdir = [
	include_directories('.'),
	include_directories('include')
]

deps = [
	cpp.find_library('pthread'),
]

cxxflags = [
	'-D_GNU_SOURCE=1'
]

if get_option('buildtype').startswith('debug')
	cxxflags += '-DSTACCATO_DEBUG=1'
endif

test_flags = [
	'-g',
	'-march=native',
	'-ffast-math',
	'-Wno-missing-field-initializers',
	'-Wno-missing-braces'
]

foreach flag : test_flags
	if cpp.has_argument(flag)
		cxxflags += flag
	endif
endforeach

subdir('benchmarks/staccato')
subdir('benchmarks/sequential')
#subdir('benchmarks/cilk')
#subdir('benchmarks/tbb')

if get_option('buildtype').startswith('debug')
	subdir('tests/')
endif

