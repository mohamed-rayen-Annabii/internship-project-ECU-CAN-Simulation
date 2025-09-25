# Paths
SRC_DIR=src
BUILD_DIR=build

ECU1_SRC=$(SRC_DIR)/ecu1.cpp
ECU2_SRC=$(SRC_DIR)/ecu2.cpp

ECU1_BIN=$(BUILD_DIR)/ecu1
ECU2_BIN=$(BUILD_DIR)/ecu2

CXX=g++
CXXFLAGS=-Wall -Wextra -std=c++17

.PHONY: all clean

all: $(ECU1_BIN) $(ECU2_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(ECU1_BIN): $(ECU1_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(ECU2_BIN): $(ECU2_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR) logs test/*.flag test/*.log

