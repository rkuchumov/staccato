SHELL = /bin/sh

CC = g++
CFLAGS = -O0 -g
ALL_FLAGS = -std=c++11 -pthread

BUILD_PATH = ../build

ifeq ($(cfg), debug)
	MODE = debug
	SO_PATH = $(BUILD_PATH)/debug
	CFLAGS += -Wall -Wextra -Wshadow -Wcast-align -pedantic
	VARS += -DSTACCATO_DEBUG=1 -DSTACCATO_STATISTICS=1
else ifeq ($(cfg), statistics)
	MODE = statistics
	SO_PATH = $(BUILD_PATH)/statistics
	VARS += -DSTACCATO_STATISTICS=1 -DSTACCATO_SAMPLE_DEQUES_SIZES=0
else
	MODE = release
	SO_PATH = $(BUILD_PATH)/release
endif

LIBS = -I../include/ -L$(SO_PATH) -Wl,-rpath=$(SO_PATH) -lstaccato


SRCS = fib.cpp
TARGET = fib

.PHONY: all
all: lib $(TARGET) 

.PHONY: lib
lib:
	make -C $(BUILD_PATH) cfg=$(MODE)

.PHONY: run
run: all
	./$(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(ALL_FLAGS) $(CFLAGS) $(VARS) $< $(LIBS) -o $@

.PHONY: clean
clean:
	rm $(TARGET) 2>/dev/null || true
