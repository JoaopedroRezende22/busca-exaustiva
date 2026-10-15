
CXX = g++
CXXFLAGS = -O2 -Wall -Wextra -std=c++17
OMPFLAGS = -fopenmp

BUILD_DIR = build

SEQ_SRC = src/sequencial/forca_bruta.cpp
SEQ_BIN = $(BUILD_DIR)/forca_bruta

OMP_SRC = src/openmp/forca_bruta_openmp.cpp
OMP_BIN = $(BUILD_DIR)/forca_bruta_openmp

.PHONY: all sequencial openmp clean

all: sequencial openmp

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

sequencial: $(SEQ_BIN)

$(SEQ_BIN): $(SEQ_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SEQ_SRC) -o $(SEQ_BIN)

openmp: $(OMP_BIN)

$(OMP_BIN): $(OMP_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(OMPFLAGS) $(OMP_SRC) -o $(OMP_BIN)

clean:
	rm -rf $(BUILD_DIR)
