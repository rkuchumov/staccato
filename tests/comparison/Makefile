SHELL = /bin/sh

CC = g++
CFLAGS = -g -O2 -Os
ALL_FLAGS = -std=c++11 -pthread

STACCATO_INC = /home/rk/Development/staccato/include
STACCATO_LIB = /home/rk/Development/staccato/build/release
STACCATO_FALGS = -I$(STACCATO_INC) -L$(STACCATO_LIB) -Wl,-rpath=$(STACCATO_LIB) -lstaccato -DSTACCATO=1

TBB_INC = /home/rk/Development/tbb/include/
TBB_LIB = /home/rk/Development/tbb/build/linux_intel64_gcc_cc5.3.1_libc2.23_kernel4.4.0_release/
TBB_FLAGS = -I$(TBB_INC) -L$(TBB_LIB) -ltbb -Wl,-rpath=$(TBB_LIB) -DTBB_USE_GCC_BUILTINS=1 -DTBB=1

CILK_FLAGS = -fcilkplus -lcilkrts -DCILK=1

OPENMP_FLAGS = -fopenmp -DOPENMP=1

LIBS = $(STACCATO_FALGS) $(TBB_FLAGS) $(CILK_FLAGS) $(OPENMP_FLAGS)

HEADERS = $(wildcard schedulers/*.h)
HEADERS += $(wildcard tasks/*.h)
HEADERS += test_suite.h 

SRCS = main.cpp

TARGET = test

.PHONY: all
all: $(TARGET) 

.PHONY: run
run: all
	./$(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(ALL_FLAGS) $(CFLAGS) $(VARS) $< $(LIBS) -o $@

.PHONY: clean
clean:
	rm $(TARGET) 2>/dev/null || true
