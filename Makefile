CXX ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -pthread -Iinclude $(shell pkg-config --cflags libpq)
LDLIBS := $(shell pkg-config --libs libpq) -lcrypt -pthread
TARGET := ftp_server
SOURCES := $(shell find src -name '*.cpp' -type f)
HEADERS := $(shell find include -name '*.hpp' -type f)

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDLIBS)

clean:
	rm -f $(TARGET)
