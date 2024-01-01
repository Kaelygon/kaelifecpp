# Makefile

CXX = g++
CXXFLAGS = -std=c++20
LDFLAGS = -lSDL2 -lGL -lGLEW

OPTIMIZE_FLAGS = -mavx2 -O3 -Os
DEBUG_FLAGS = -g -Wall -Wextra -pedantic -D_GLIBCXX_DEBUG -Og 
ASAN_FLAGS  = -g -Wall -Wextra -pedantic -D_GLIBCXX_DEBUG -Og -fsanitize=address #otherwise fsanitize and valgrind clash 

SRC_DIR = ./src
INCLUDE_DIR = ./include
BUILD_DIR = ./build
GEN_DIR = ./generated

SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))
MAIN_SRC_NAME := $(notdir $(basename $(wildcard $(SRC_DIR)/*.cpp)))


# Build executable with debug flags
alldebug: $(BUILD_DIR)/$(MAIN_SRC_NAME)_debug

$(BUILD_DIR)/$(MAIN_SRC_NAME)_debug: $(patsubst $(BUILD_DIR)/%.o, $(BUILD_DIR)/%_debug.o, $(OBJ_FILES))
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEBUG_FLAGS) $^ -o $@

# Build object files from source files with optimization flgas
$(BUILD_DIR)/%_debug.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) $(DEBUG_FLAGS) -c $< -o $@


# Build executable with asan flags
allasan: $(BUILD_DIR)/$(MAIN_SRC_NAME)_asan
$(BUILD_DIR)/$(MAIN_SRC_NAME)_asan: $(patsubst $(BUILD_DIR)/%.o, $(BUILD_DIR)/%_asan.o, $(OBJ_FILES))
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(ASAN_FLAGS) $^ -o $@

# Build object files from source files with optimization flgas
$(BUILD_DIR)/%_asan.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) $(ASAN_FLAGS) -c $< -o $@


# Build executable with optimization flags
alloptimized: $(BUILD_DIR)/$(MAIN_SRC_NAME)_optimized

$(BUILD_DIR)/$(MAIN_SRC_NAME)_optimized: $(patsubst $(BUILD_DIR)/%.o, $(BUILD_DIR)/%_optimized.o, $(OBJ_FILES))
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OPTIMIZE_FLAGS) $^ -o $@

# Build object files from source files with optimization flgas
$(BUILD_DIR)/%_optimized.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) $(OPTIMIZE_FLAGS) -c $< -o $@


clean:
	rm -f $(BUILD_DIR)/*

.PHONY: alldebug alloptimized clean
