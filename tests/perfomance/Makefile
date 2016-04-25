SHELL = /bin/sh

CC = g++
CFLAGS = -Wall -Wextra -pedantic -g -O0 -DSTACCATO_DEBUG=1

# TBB_SO = /home/rk/Development/tbb/build/linux_intel64_gcc_cc5.3.1_libc2.23_kernel4.4.0_release/
# TBB_INC = /home/rk/Development/tbb/include
# TBB_FLAGS = -I$(TBB_INC) -L$(TBB_SO) -Wl,-rpath=$(TBB_SO) -ltbb

SO_PATH = ../../build/release
LIBS = -I../../include/ -I. -L$(SO_PATH) -Wl,-rpath=$(SO_PATH) -lstaccato $(TBB_FLAGS)
ALL_FLAGS = -std=c++11 -pthread

SRCS = main.cpp
HEADERS = taskexec.h fib.h custom.h knapsack.h matmult.h mergesort.h
TARGET = test

.PHONY: all
all: lib $(TARGET) 

.PHONY: lib
lib:
	make -j 4 -C ../../build/ cfg=release

.PHONY: run
run: all
	./$(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(ALL_FLAGS) $(CFLAGS) $< $(LIBS) -o $@

.PHONY: clean
clean:
	rm $(TARGET) 2>/dev/null || true
