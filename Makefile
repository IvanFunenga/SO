# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -pthread
LDFLAGS = -lpthread

# Source and object directories
SRC_DIR = src
OBJ_DIR = obj

# Target executables
TARGETS = dei_chain miner

# Source files
CONTROLLER_SRCS = $(SRC_DIR)/controller.c $(SRC_DIR)/shared_memory.c $(SRC_DIR)/ipc_utils.c $(SRC_DIR)/logging.c $(SRC_DIR)/miner.c
MINER_SRCS = $(SRC_DIR)/miner.c $(SRC_DIR)/shared_memory.c $(SRC_DIR)/ipc_utils.c $(SRC_DIR)/logging.c

# Object files
CONTROLLER_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(CONTROLLER_SRCS))
MINER_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(MINER_SRCS))

# Default target
all: $(TARGETS)

# Main controller executable
dei_chain: $(CONTROLLER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Miner executable
miner: $(MINER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Pattern rule for object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create object directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean up
clean:
	rm -rf $(OBJ_DIR) $(TARGETS) DEIChain_log.txt

.PHONY: all clean