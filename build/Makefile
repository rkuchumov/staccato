SHELL = /bin/sh

CC = g++
ALLFLAGS = -std=c++11 -pthread
CFLAGS = -Wall -Wextra -pedantic -g -O0 -fpic -c
LDFLAGS = -shared

TARGET = libsch.so

SRCS = $(wildcard ../src/*.cpp)
OBJS = $(SRCS:../src/%.cpp=%.o)

.PHONY: all
all: $(TARGET) 

$(TARGET): $(OBJS)
	$(CC) $(ALLFLAGS) $(LDFLAGS) $(OBJS) -o $@

%.o: ../src/%.cpp
	$(CC) $(ALLFLAGS) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm $(TARGET) 2>/dev/null || true
	rm $(OBJS) 2>/dev/null || true
	rm .depend 2>/dev/null || true

.PHONY: run
run: $(TARGET)
	./$(TARGET)

.PHONY: dep
dep: $(SRCS)
	$(CC) $(ALLFLAGS) $(CFLAGS) -MM $^  > .depend

-include .depend
