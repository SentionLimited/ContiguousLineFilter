#
#  Makefile
#

TARGET 	   := contiguous_line_filter
BUILD      := ./build
OBJ_DIR    := $(BUILD)/objects
BIN_DIR    := $(BUILD)/bin
OUT_DIR    := $(BUILD)/output

CC         := /usr/bin/g++
CCFLAGS    := -std=c++14 -O3

#
# Includes
#
INCLUDES   := `pkg-config opencv --cflags --libs` # opencv

#
# Sources
#
SRC        :=                           		\
   $(wildcard contiguous_line_filter.cpp)      	\
   $(wildcard main.cpp)             			\

OBJECTS    := $(SRC:%.cpp=$(OBJ_DIR)/%.o)

#
# Compile commands
#
COLOR=\033[0;35m # purple
COLOR_RESET=\033[0m

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	@echo "Building$(COLOR)$@$(COLOR_RESET)"
	$(CC) $(CCFLAGS) $(INCLUDES) -c $< -o $@
	@echo ""

$(BIN_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	@echo "Building$(COLOR)$(BIN_DIR)/$(TARGET)$(COLOR_RESET)"
	$(CC) -o $(BIN_DIR)/$(TARGET) $^ $(INCLUDES)

#
# Make commands
#
.PHONY: all build clean 

all: build $(BIN_DIR)/$(TARGET)

build:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OUT_DIR)

clean:
	-@rm -rvf $(BIN_DIR)/*
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(OUT_DIR)/*
	-@echo ""
