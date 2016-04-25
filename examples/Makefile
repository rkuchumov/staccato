CC = g++
CFLAGS = -Wall -Wextra

BUILD_PATH = ../build/
SO_PATH = $(BUILD_PATH)/release
LIBS = -I../include/ -L$(SO_PATH) -Wl,-rpath=$(SO_PATH) -lstaccato
ALL_FLAGS = -std=c++11 -pthread

SRCS = fib.cpp
TARGET = fib

.PHONY: all
all: lib $(TARGET) 

.PHONY: lib
lib:
	make -C $(BUILD_PATH) cfg=release

.PHONY: run
run:
	./$(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(ALL_FLAGS) $(CFLAGS) $< $(LIBS) -o $@

.PHONY: clean
clean:
	rm $(TARGET) 2>/dev/null || true
