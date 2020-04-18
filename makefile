CXX      := g++-9.1
CXXFLAGS := -pedantic-errors -Wall -Wextra -Werror -Wno-psabi -std=c++17
LDFLAGS  := -pthread -lboost_system
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
DEP_DIR  := $(BUILD)/deps
APP_DIR  := $(BUILD)/apps
TARGET   := script_bot
INCLUDE  := -Iinclude/
SRC      :=                      \
   $(wildcard src/*.cpp)         \

OBJECTS  := $(SRC:%.cpp=$(OBJ_DIR)/%.o)
DEPS  := $(SRC:%.cpp=$(DEP_DIR)/%.d)

all: build $(APP_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@ $(LDFLAGS)

$(APP_DIR)/$(TARGET): $(OBJECTS) $(DEPS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $(APP_DIR)/$(TARGET) $(OBJECTS) $(LDFLAGS)

$(DEP_DIR)/%.d: %.cpp
	@set -e; rm -f $@; \
	$(CXX) -MM $(INCLUDE) $< > $@.$$$$; \
	sed 's,\([a-z_]*\)\.o[ :]*,$(OBJ_DIR)/src/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

.PHONY: all build clean debug release

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(DEP_DIR)
	@mkdir -p $(DEP_DIR)/src

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -O2
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*
	-@rm -rvf $(DEP_DIR)/*

include $(wildcard $(DEP_DIR)/src/*.d)

