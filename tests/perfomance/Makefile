SHELL = /bin/sh

CC = g++
CFLAGS = -g -O0
ALL_FLAGS = -std=c++11 -pthread

BUILD_PATH = ../../build
INCLUDE_PATH = ../../include

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

ifeq ($(with), tbb)
	# TBB_SO = <path_to_tbb_so_dir>
	# TBB_INC = <path_to_tbb_include>
	# TBB_FLAGS = -I$(TBB_INC) -L$(TBB_SO) -Wl,-rpath=$(TBB_SO) -ltbb
	TBB_FLAGS = -ltbb
endif

LIBS = -I$(INCLUDE_PATH) -L$(SO_PATH) -Wl,-rpath=$(SO_PATH) -lstaccato $(TBB_FLAGS)

SRCS = main.cpp
HEADERS = taskexec.h fib.h custom.h knapsack.h matmult.h mergesort.h
TARGET = test

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
