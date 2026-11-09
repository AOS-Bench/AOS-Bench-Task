# libcompute — small C library exercised by an 8-suite test harness.
#
# `make` builds bin/run_tests; `make test` runs it and reports pass/fail.
# `make clean` scrubs build artifacts.

CC       := gcc
CFLAGS   := -O2 -Wall -Wextra -Werror -Wno-unused-result -Iinclude -std=c11 -MMD -MP
LDFLAGS  :=
LDLIBS   := -lm -lpthread

SRC_DIR  := src
OBJ_DIR  := build
BIN_DIR  := bin

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

TARGET := $(BIN_DIR)/run_tests

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

test: $(TARGET)
	@$(TARGET)

clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)

-include $(DEPS)
