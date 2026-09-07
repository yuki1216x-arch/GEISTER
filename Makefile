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

BIN_DIR  := bin
OBJ_DIR  := obj

# =========================
# source files
# =========================

COMMON_BASE_SRC  := \
	src/common/node.cpp \
	src/common/zdd_geister.cpp \
	src/common/posi_geister.cpp \
	src/common/table.cpp

ANALYSIS_COMMON_SRC  := \
	$(COMMON_BASE_SRC)
VALIDATION_COMMON_SRC  := \
	$(COMMON_BASE_SRC)
LOOKUP_COMMON_SRC  := \
	$(COMMON_BASE_SRC)

ANALYSIS_SRC      := $(wildcard $(ANALYSIS_DIR)/*.cpp)
VALIDATION_SRC    := $(wildcard $(VALIDATION_DIR)/*.cpp)
LOOKUP_SRC        := $(wildcard $(LOOKUP_DIR)/*.cpp)

# =========================
# executable names
# =========================

ANALYSIS_TARGETS      := $(patsubst $(ANALYSIS_DIR)/%.cpp,$(BIN_DIR)/%,$(ANALYSIS_SRC))
VALIDATION_TARGETS    := $(patsubst $(VALIDATION_DIR)/%.cpp,$(BIN_DIR)/%,$(VALIDATION_SRC))
LOOKUP_TARGETS        := $(patsubst $(LOOKUP_DIR)/%.cpp,$(BIN_DIR)/%,$(LOOKUP_SRC))

TARGETS  := \
	$(ANALYSIS_TARGETS) \
	$(VALIDATION_TARGETS) \
	$(LOOKUP_TARGETS)

# =========================
# default target
# =========================

all: $(TARGETS)

# =========================
# build rules
# =========================

# analysis
# Purple version
$(BIN_DIR)/main_purple: $(ANALYSIS_DIR)/main_purple.cpp $(ANALYSIS_COMMON_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -DUSE_PURPLE $(INCLUDES) $^ -o $@

# Other analysis programs
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
# clean
# =========================

test: $(BIN_DIR)/main
	@echo "=== Building test databases ==="
	@mkdir -p data/db data/output

	@./bin/main 1 1 1 1 1 s > data/output/test1-1-1-1.txt 2>&1
	@./bin/main 1 1 1 1 2 s > data/output/test1-1-1-2.txt 2>&1
	@./bin/main 1 1 1 2 1 s > data/output/test1-1-2-1.txt 2>&1

	@echo "=== Comparing databases ==="
	@failed=0; \
	for file in \
		self_table_1-1-1-1.bin \
		enemy_table_1-1-1-1.bin \
		self_table_1-1-1-2.bin \
		enemy_table_1-1-1-2.bin \
		self_table_1-1-2-1.bin \
		enemy_table_1-1-2-1.bin \
		self_table_1-2-1-1.bin \
		enemy_table_1-2-1-1.bin \
		self_table_2-1-1-1.bin \
		enemy_table_2-1-1-1.bin; \
	do \
		if ! diff -q data/$$file data/db/$$file > /dev/null; then \
			echo "DIFF: $$file"; \
			failed=1; \
		fi; \
	done; \
	if [ $$failed -ne 0 ]; then \
		echo "buildに失敗した"; \
		exit 1; \
	else \
		echo "buildに成功した"; \
	fi

# =========================
# phony
# =========================

.PHONY: all debug clean
