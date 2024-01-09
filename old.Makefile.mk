# Makefile
# I have absolutely no clue what I am doing

CXX = g++
CXXFLAGS = -std=c++20
LDFLAGS = -lSDL2 -lGL -lGLEW

OPTIMIZE_FLAGS = -mavx2 -O3 #OPTIMIZE
DEBUG_FLAGS = -g -Wall -Wextra -pedantic -D_GLIBCXX_DEBUG -O0 #DEBUG
ASAN_FLAGS  = -g -Wall -Wextra -pedantic -D_GLIBCXX_DEBUG -fsanitize=address,undefined #ASAN

SRC_DIR = ./src
INCLUDE_DIR = ./include
BUILD_DIR = ./build

MAIN_SRC_NAME := $(notdir $(basename $(wildcard $(SRC_DIR)/*.cpp))) #all

ACTIVE_SRC_PATH ?= /dev/null
ACTIVE_SRC_NAME ?= /dev/null
ACTIVE_BUILD_VARIANT ?= DEBUG

SELECTED_FLAGS := $($(ACTIVE_BUILD_VARIANT)_FLAGS)
ifeq ($(SELECTED_FLAGS),)
  SELECTED_FLAGS := $(OPTIMIZE_FLAGS)
endif

DIRECTORIES := $(SRC_DIR) $(INCLUDE_DIR) $(BUILD_DIR) $(GEN_DIR)

# Create directories at the beginning of each invocation
create_dir:
	mkdir -p $(DIRECTORIES)


#functions
ifeq ($(filter all,$(MAKECMDGOALS)),all) #Ugly workaround. Otherwise make will build every target regardless which MAKECMDGOALS are specified
define build_every_src
$(BUILD_DIR)/%_$(1): $$(BUILD_DIR)/%_$(1).o
	$$(CXX) $$(CXXFLAGS) $$(LDFLAGS) $(2) $$^ -o $$@

$$(BUILD_DIR)/%_$(1).o: $$(SRC_DIR)/%.cpp $$(BUILD_DIR)/%.d
	$$(CXX) $$(CXXFLAGS) -I$$(INCLUDE_DIR) $(2) -c $$< -o $$@

$$(BUILD_DIR)/%.d: $$(SRC_DIR)/%.cpp $$(wildcard $$(INCLUDE_DIR)/*.hpp)
	$$(CXX) $$(CXXFLAGS) -I$$(INCLUDE_DIR) -MM -MP $$< -MF $$@
	$(info Building ALL $(MAIN_SRC_NAME)_$(ACTIVE_BUILD_VARIANT) with $(SELECTED_FLAGS))
endef
endif

ifeq ($(filter active,$(MAKECMDGOALS)),active)
define build_active_src
$(BUILD_DIR)/$(ACTIVE_SRC_NAME)_$(1): $$(BUILD_DIR)/$$(ACTIVE_SRC_NAME)_$(1).o $$(patsubst $(INCLUDE_DIR)/%.hpp,$(BUILD_DIR)/%.d,$(wildcard $$(INCLUDE_DIR)/*.hpp))
	$$(CXX) $$(CXXFLAGS) $$(LDFLAGS) $(2) $$^ -o $$@

$$(BUILD_DIR)/$$(ACTIVE_SRC_NAME)_$(1).o: $$(ACTIVE_SRC_PATH)/$$(ACTIVE_SRC_NAME).cpp $$(wildcard $$(INCLUDE_DIR)/*.hpp)
	$$(CXX) $$(CXXFLAGS) -I$$(INCLUDE_DIR) $(2) -c $$< -o $$@
	$(info Building ACTIVE $(ACTIVE_SRC_NAME)_$(ACTIVE_BUILD_VARIANT) with $(SELECTED_FLAGS))
endef
endif

#These are not allowed to have intendation that causes Makefile:66: *** prerequisites cannot be defined in recipes.  Stop.
# Build every program in ./src
all: $(foreach program,$(MAIN_SRC_NAME),$(BUILD_DIR)/$(program)_$(ACTIVE_BUILD_VARIANT))
$(eval $(call build_every_src,$(ACTIVE_BUILD_VARIANT),$(SELECTED_FLAGS)))

# Build vscode active file 
active: $(BUILD_DIR)/$(ACTIVE_SRC_NAME)_$(ACTIVE_BUILD_VARIANT)
$(eval $(call build_active_src,$(ACTIVE_BUILD_VARIANT),$(SELECTED_FLAGS)))

#somehow this is allowed to have intendation
test:
	$(info THIS SHOULD NOT BE BUILT) 

clean:
	rm -f $(BUILD_DIR)/*

.PHONY: all active clean
