CXX = g++
CXXFLAGS = -O2 -Wall -Wextra -std=c++17

BUILD_DIR = build
SEQ_SRC = src/sequencial/forca_bruta.cpp
SEQ_BIN = $(BUILD_DIR)/forca_bruta

.PHONY: all sequencial clean

all: sequencial

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

sequencial: $(SEQ_BIN)

$(SEQ_BIN): $(SEQ_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SEQ_SRC) -o $(SEQ_BIN)

clean:
	rm -rf $(BUILD_DIR)