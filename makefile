CXX      := g++-8
CXXFLAGS := -pedantic-errors -Wall -Wextra -Werror -Wno-psabi -std=c++17 -march=native
LDFLAGS  := -pthread -lboost_system
BUILD    := build
OBJ_DIR  := $(BUILD)/objects
DEP_DIR  := $(BUILD)/deps
APP_DIR  := $(BUILD)/apps
TARGET   := script_bot
INCLUDE  := -Iinclude/
SRC      :=                      \
   $(wildcard src/*.cpp)         \
   $(wildcard src/*/*.cpp)       \

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
	@mkdir -p $(@D)
	@set -e; rm -f $@; \
	$(CXX) -MM $(INCLUDE) $< > $@.$$$$; \
	sed 's,\([a-z_]*\)\.o[ :]*,$(@D:$(DEP_DIR)/%=$(OBJ_DIR)/%)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

.PHONY: all build clean debug release

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(DEP_DIR)

debug: CXXFLAGS += -DDEBUG -DDEBUG_OUT -O0 -g -D_GLIBCXX_DEBUG -ggdb -fno-omit-frame-pointer -fsanitize=undefined -fsanitize=address -fsanitize=leak
debug: all

release: CXXFLAGS += -O2
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*
	-@rm -rvf $(DEP_DIR)/*

include $(wildcard $(DEP_DIR)/src/*.d) $(wildcard $(DEP_DIR)/src/*/*.d)


