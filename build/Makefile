SHELL = /bin/sh

CC = g++
LDFLAGS = -std=c++11 -shared -pthread 
ALL_FLAGS = -std=c++11 -fpic -c 
INCLUDE = -I../include -I../src

ifeq ($(cfg), debug)
	OBJS_DIR = debug
	CFLAGS = -Wall -Wextra -Wshadow -Wcast-align -pedantic
	CFLAGS += -O0 -g
	VARS += -DSTACCATO_DEBUG=1 -DSTACCATO_STATISTICS=1
else ifeq ($(cfg), statistics)
	OBJS_DIR = statistics
	CFLAGS = -O2
	VARS += -DSTACCATO_STATISTICS=1
else
	OBJS_DIR = release
	CFLAGS = -O2
endif

SRCS = $(wildcard ../src/*.cpp)
TARGET = $(OBJS_DIR)/libstaccato.so
OBJS = $(SRCS:../src/%.cpp=$(OBJS_DIR)/%.o)

CACHE_SIZE := $(shell getconf LEVEL1_DCACHE_LINESIZE 2>/dev/null || false)
ifdef CACHE_SIZE
	VARS += -DLEVEL1_DCACHE_LINESIZE=$(CACHE_SIZE)
endif

.PHONY: all
all: objs_dir $(TARGET) dep

.PHONY: objs_dir
objs_dir:
	mkdir -p $(OBJS_DIR)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(OBJS_DIR)/%.o: ../src/%.cpp
	$(CC) $(ALL_FLAGS) $(VARS) $(CFLAGS) $(INCLUDE) $< -o $@

.PHONY: clean
clean:
	rm -rfv $(OBJS_DIR) 2>/dev/null || true

.PHONY: dep
dep: $(OBJS_DIR)/.depend

$(OBJS_DIR)/.depend: $(SRCS)
	mkdir -p $(OBJS_DIR)
	$(CC) $(ALL_FLAGS) $(CFLAGS) $(INCLUDE) -MM $(SRCS) | sed -e  's/.*:/$(OBJS_DIR)\/&/' > $(OBJS_DIR)/.depend

-include $(OBJS_DIR)/.depend
