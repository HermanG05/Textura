# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g
LDFLAGS = -lncurses

# Source files and target executable
SRC = main.c buffer.c utils.c history.c
TARGET = Textura

# Build directories
OBJ_DIR = .obj
OBJ = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))

# Build the target
all: setup $(TARGET)

# Create object directory
setup:
	@mkdir -p $(OBJ_DIR)

# Linking the object files to create the final executable
$(TARGET): $(OBJ)
	@echo "Building $(TARGET)..."
	@$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Rule for compiling source files to object files
$(OBJ_DIR)/%.o: %.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Clean rule to remove the generated files
clean:
	@echo "Cleaning up..."
	@rm -rf $(TARGET) $(OBJ_DIR)

.PHONY: all clean setup
