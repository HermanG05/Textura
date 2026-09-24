CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g -Iinclude
LDFLAGS = -lncurses

SRC = src/main.c src/buffer.c src/utils.c src/history.c src/syntax.c
TARGET = Textura
OBJ_DIR = .obj
OBJ = $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SRC))

all: $(TARGET)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(TARGET): $(OBJ)
	@echo "Building $(TARGET)..."
	@$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/test_textura: tests/test_textura.c $(filter-out $(OBJ_DIR)/main.o,$(OBJ)) $(wildcard include/*.h)
	@$(CC) $(CFLAGS) -o $@ tests/test_textura.c $(filter-out $(OBJ_DIR)/main.o,$(OBJ)) $(LDFLAGS)

$(OBJ_DIR)/test_syntax: tests/test_syntax.c $(OBJ_DIR)/buffer.o $(OBJ_DIR)/syntax.o $(wildcard include/*.h)
	@$(CC) $(CFLAGS) -o $@ tests/test_syntax.c $(OBJ_DIR)/buffer.o $(OBJ_DIR)/syntax.o $(LDFLAGS)

$(OBJ_DIR)/test_render: tests/test_render.c $(filter-out $(OBJ_DIR)/main.o,$(OBJ)) $(wildcard include/*.h)
	@$(CC) $(CFLAGS) -o $@ tests/test_render.c $(filter-out $(OBJ_DIR)/main.o,$(OBJ)) $(LDFLAGS)

test: $(OBJ_DIR)/test_textura $(OBJ_DIR)/test_syntax $(OBJ_DIR)/test_render
	@./$(OBJ_DIR)/test_textura
	@./$(OBJ_DIR)/test_syntax
	@./$(OBJ_DIR)/test_render

test-terminal: $(TARGET)
	@python3 tests/test_terminal.py

clean:
	@echo "Cleaning up..."
	@rm -rf $(TARGET) $(OBJ_DIR)

-include $(OBJ:.o=.d)

.PHONY: all clean test test-terminal
