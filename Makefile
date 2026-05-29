# =========================
# compiler settings
# =========================

CXX       := g++
CXXFLAGS  := -std=c++14 -O2 -Wall -pthread -DNDEBUG
INCLUDES  := -Isrc/common

# sanitizer/debug
DEBUGFLAGS  := -g -O0 -fsanitize=address, undefined

# =========================
# directories
# =========================

COMMON_DIR        := src/common
ANALYSIS_DIR      := src/analysis
VALIDATION_DIR    := src/validation
LOOKUP_DIR        := src/lookup
REACHABILITY_DIR  := src/reachability

BIN_DIR  := bin
OBJ_DIR  := obj

# =========================
# source files
# =========================

COMMON_BASE_SRC  := \
	src/common/node.cpp \
	src/common/zdd_yonoku.cpp \
	src/common/posi_yonoku.cpp
COMMON_TABLE_SRC  := \
	src/common/table.cpp
COMMON_TABLE_2BIT_SRC  := \
	src/common/table_reachability.cpp

ANALYSIS_COMMON_SRC  := \
	$(COMMON_BASE_SRC) \
	$(COMMON_TABLE_SRC)
VALIDATION_COMMON_SRC  := \
	$(COMMON_BASE_SRC) \
	$(COMMON_TABLE_SRC)
LOOKUP_COMMON_SRC  := \
	$(COMMON_BASE_SRC) \
	$(COMMON_TABLE_SRC)
REACHABILITY_COMMON_SRC  := \
	$(COMMON_BASE_SRC) \
	$(COMMON_TABLE_2BIT_SRC)

ANALYSIS_SRC      := $(wildcard $(ANALYSIS_DIR)/*.cpp)
VALIDATION_SRC    := $(wildcard $(VALIDATION_DIR)/*.cpp)
LOOKUP_SRC        := $(wildcard $(LOOKUP_DIR)/*.cpp)
REACHABILITY_SRC  := $(wildcard $(REACHABILITY_DIR)/*.cpp)

# =========================
# executable names
# =========================

ANALYSIS_TARGETS      := $(patsubst $(ANALYSIS_DIR)/%.cpp,$(BIN_DIR)/%,$(ANALYSIS_SRC))
VALIDATION_TARGETS    := $(patsubst $(VALIDATION_DIR)/%.cpp,$(BIN_DIR)/%,$(VALIDATION_SRC))
LOOKUP_TARGETS        := $(patsubst $(LOOKUP_DIR)/%.cpp,$(BIN_DIR)/%,$(LOOKUP_SRC))
REACHABILITY_TARGETS  := $(patsubst $(REACHABILITY_DIR)/%.cpp,$(BIN_DIR)/%,$(REACHABILITY_SRC))

TARGETS  := \
	$(ANALYSIS_TARGETS) \
	$(VALIDATION_TARGETS) \
	$(LOOKUP_TARGETS) \
	$(REACHABILITY_TARGETS)

# =========================
# default target
# =========================

all: $(TARGETS)

# =========================
# build rules
# =========================

# analysis
$(BIN_DIR)/%: $(ANALYSIS_DIR)/%.cpp $(ANALYSIS_COMMON_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# validation
$(BIN_DIR)/%: $(VALIDATION_DIR)/%.cpp $(VALIDATION_COMMON_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# lookup
$(BIN_DIR)/%: $(LOOKUP_DIR)/%.cpp $(LOOKUP_COMMON_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# reachability
$(BIN_DIR)/%: $(REACHABILITY_DIR)/%.cpp $(REACHABILITY_COMMON_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# =========================
# debug build
# =========================

debug: CXXFLAGS += $(DEBUGFLAGS)
debug: clean all

# =========================
# clean
# =========================

clean:
	rm -rf $(BIN_DIR)/*

# =========================
# phony
# =========================

.PHONY: all debug clean
