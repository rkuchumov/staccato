SHELL = /bin/sh

CC = g++
CFLAGS = -Wall -Wextra -pedantic -g -O0
LIBS = -I../src/ -L../build/ -Wl,-rpath=../build/ -lsch
ALL_FLAGS = -std=c++11 -pthread

SRCS = $(wildcard *.cpp)
TARGETS = $(SRCS:%.cpp=%)

.PHONY: all
all: lib $(TARGETS) 

.PHONY: lib
lib:
	make -j 4 -C ../build/

%: %.cpp
	$(CC) $(ALL_FLAGS) $(CFLAGS) $< $(LIBS) -o $@

.PHONY: clean
clean:
	rm $(TARGETS) 2>/dev/null || true
