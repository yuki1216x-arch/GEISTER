# =========================
# compiler settings
# =========================

CXX       := g++
CXXFLAGS  := -std=c++14 -O2 -Wall -pthread #-DNDEBUG
INCLUDES  := -Isrc/common

# sanitizer/debug
DEBUGFLAGS  := -g -O0 -fsanitize=address, undefined

# =========================
# directories
# =========================

COMMON_DIR        := src/common
ANALYSIS_DIR      := src/analysis
VALIDATION_DIR    := src/validation

BIN_DIR  := bin
OBJ_DIR  := obj

# =========================
# source files
# =========================

COMMON_BASE_SRC  := \
	src/common/zdd_geister.cpp \
	src/common/posi_geister.cpp

ANALYSIS_COMMON_SRC  := \
	$(COMMON_BASE_SRC)
VALIDATION_COMMON_SRC  := \
	$(COMMON_BASE_SRC)

ANALYSIS_SRC      := $(wildcard $(ANALYSIS_DIR)/*.cpp)
VALIDATION_SRC    := $(wildcard $(VALIDATION_DIR)/*.cpp)

# =========================
# executable names
# =========================

ANALYSIS_TARGETS      := $(patsubst $(ANALYSIS_DIR)/%.cpp,$(BIN_DIR)/%,$(ANALYSIS_SRC))
VALIDATION_TARGETS    := $(patsubst $(VALIDATION_DIR)/%.cpp,$(BIN_DIR)/%,$(VALIDATION_SRC))

TARGETS  := \
	$(ANALYSIS_TARGETS) \
	$(VALIDATION_TARGETS)

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
# $(BIN_DIR)/%: $(LOOKUP_DIR)/%.cpp $(LOOKUP_COMMON_SRC)
#	@mkdir -p $(BIN_DIR)
#	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# reachability
# $(BIN_DIR)/%: $(REACHABILITY_DIR)/%.cpp $(REACHABILITY_COMMON_SRC)
#	@mkdir -p $(BIN_DIR)
#	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

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
