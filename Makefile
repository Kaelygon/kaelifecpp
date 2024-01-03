# Makefile

CXX = g++
CXXFLAGS = -std=c++20
LDFLAGS = -lSDL2 -lGL -lGLEW

OPTIMIZE_FLAGS = -mavx2 -O3 #everyoptimized
DEBUG_FLAGS = -g -Wall -Wextra -pedantic -D_GLIBCXX_DEBUG -ggdb #everydebug
ASAN_FLAGS  = -g -Wall -Wextra -pedantic -D_GLIBCXX_DEBUG -ggdb -fsanitize=address #everyasan

SRC_DIR = ./src
INCLUDE_DIR = ./include
BUILD_DIR = ./build
GEN_DIR = ./generated

SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))
MAIN_SRC_NAME := $(notdir $(basename $(wildcard $(SRC_DIR)/*.cpp)))


DIRECTORIES := $(SRC_DIR) $(INCLUDE_DIR) $(BUILD_DIR) $(GEN_DIR)
${DIRECTORIES}:
	mkdir -p ${DIRECTORIES}


define build_every_src
$(BUILD_DIR)/%_$(1): $$(BUILD_DIR)/%_$(1).o
	$$(CXX) $$(CXXFLAGS) $$(LDFLAGS) $(2) $$^ -o $$@

$$(BUILD_DIR)/%_$(1).o: $$(SRC_DIR)/%.cpp
	$$(CXX) $$(CXXFLAGS) -I$$(INCLUDE_DIR) $(2) -c $$< -o $$@
endef


define build_active_src
$(BUILD_DIR)/$(ACTIVE_SRC_NAME)_$(1): $$(BUILD_DIR)/$$(ACTIVE_SRC_NAME).o
	$$(CXX) $$(CXXFLAGS) $$(LDFLAGS) $$(ACTIVE_SRC_FLAGS) $$^ -o $$@

$$(BUILD_DIR)/$$(ACTIVE_SRC_NAME).o: $$(ACTIVE_SRC_PATH)/$$(ACTIVE_SRC_NAME).cpp
	$$(CXX) $$(CXXFLAGS) -I$$(INCLUDE_DIR) $$(ACTIVE_SRC_FLAGS) -c $$< -o $$@
endef


# Build executable with debug flags
everydebug: create_directories $(foreach program,$(MAIN_SRC_NAME),$(BUILD_DIR)/$(program)_debug)
$(eval $(call build_every_src,debug,$(DEBUG_FLAGS)))

# Build executable with asan flags
everyasan: create_directories $(foreach program,$(MAIN_SRC_NAME),$(BUILD_DIR)/$(program)_asan)
$(eval $(call build_every_src,asan,$(ASAN_FLAGS)))

# Build executable with optimization flags
everyoptimized: create_directories $(foreach program,$(MAIN_SRC_NAME),$(BUILD_DIR)/$(program)_optimized)
$(eval $(call build_every_src,optimized,$(OPTIMIZE_FLAGS)))


# Build vscode active file
activedebug: create_directories $(BUILD_DIR)/$(ACTIVE_SRC_NAME)_active
$(eval $(call build_active_src,active,$(DEBUG_FLAGS)))

#all: alloptimized allasan alldebug

create_directories:
	@mkdir -p $(DIRECTORIES)

clean:
	rm -f $(BUILD_DIR)/*

.PHONY: everyoptimized activedebug everyasan everydebug all create_directories