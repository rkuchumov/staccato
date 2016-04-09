SHELL = /bin/sh

CC = g++
LDFLAGS = -std=c++11 -shared -pthread 
ALL_FLAGS = -std=c++11 -Wall -Wextra -pedantic -fpic -c
VARS = -DLEVEL1_DCACHE_LINESIZE=`getconf LEVEL1_DCACHE_LINESIZE`

# debug version:
CFLAGS = -g -00

# reliase version:
CFLAGS = -O2 
VARS += -DNDEBUG=1 -DNSTAT=1

TARGET = libsch.so
SRCS = $(wildcard ../src/*.cpp)
OBJS = $(SRCS:../src/%.cpp=%.o)

.PHONY: all
all: $(TARGET) 

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

%.o: ../src/%.cpp
	$(CC) $(ALL_FLAGS) $(CFLAGS) $(VARS) $< -o $@

.PHONY: clean
clean:
	rm $(TARGET) 2>/dev/null || true
	rm $(OBJS) 2>/dev/null || true

.PHONY: run
run: $(TARGET)
	./$(TARGET)

.PHONY: dep
dep: $(SRCS)
	$(CC) $(CFLAGS) -MM $^  > .depend

-include .depend
