CXX ?= g++

COMMON_FLAGS += -fPIE -Wall -Wextra -Werror

CXXFLAGS += -std=c++17
LDFLAGS += -lglog -lstdc++fs

ifdef DEBUG
	CXXFLAGS += -g 
else
	CXXFLAGS += -O2 -DNDEBUG
endif

BIN = yamc
BUILD_DIR = ./build
INSTALL_DIR = /usr/local/bin

CPP = $(wildcard src/*.cpp)

OBJ = $(CPP:%.cpp=$(BUILD_DIR)/%.o)
DEP = $(OBJ:%.o=%.d)

$(BIN) : $(BUILD_DIR)/$(BIN)

$(BUILD_DIR)/$(BIN) : $(OBJ)
	mkdir -p $(@D)
	$(CXX) $(COMMON_FLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

-include $(DEP)

$(BUILD_DIR)/%.o : %.cpp
	mkdir -p $(@D)
	$(CXX) $(COMMON_FLAGS) $(CXXFLAGS) -MMD -c $< -o $@

debug:
	make DEBUG=1

install:
	cp $(BUILD_DIR)/$(BIN) $(INSTALL_DIR)/$(BIN)

.PHONY : clean
clean :
	-rm $(BUILD_DIR)/$(BIN) $(OBJ) $(DEP)