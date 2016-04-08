SHELL = /bin/sh

CC = g++
LDFLAGS = -std=c++11 -shared -pthread 

CFLAGS = -std=c++11 -Wall -Wextra -pedantic -fpic -c -O2 -DNDEBUG=1 -DNSTAT=1

# debug version:
# CFLAGS = -std=c++11 -Wall -Wextra -pedantic -fpic -c -g -O0 

TARGET = libsch.so
SRCS = $(wildcard ../src/*.cpp)
OBJS = $(SRCS:../src/%.cpp=%.o)

.PHONY: all
all: $(TARGET) 

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

%.o: ../src/%.cpp
	$(CC) $(CFLAGS) $< -o $@

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
