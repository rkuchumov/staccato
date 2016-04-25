SHELL = /bin/sh

CC = g++
CFLAGS = -Wall -Wextra -pedantic -g -O0 -DSTACCATO_DEBUG=1

SO_PATH = ../../build/release
LIBS = -I../../include/ -I. -L$(SO_PATH) -Wl,-rpath=$(SO_PATH) -lstaccato
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
run:
	./$(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(ALL_FLAGS) $(CFLAGS) $< $(LIBS) -o $@

.PHONY: clean
clean:
	rm $(TARGET) 2>/dev/null || true
