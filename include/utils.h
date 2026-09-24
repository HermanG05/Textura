#pragma once

#include "buffer.h"
#include "history.h"

#define LINE_NUMBER_WIDTH 6
#define TAB_WIDTH 4
#define INPUT_SIZE 256

typedef struct {
    Buffer* buf;
    History* history;
    size_t cursor_pos;
    size_t top_line;
    size_t left_column;
    char search[INPUT_SIZE];
    char message[512];
    const char* filename;
    int auto_indent;
    int syntax_mode;
    int light_theme;
} Editor;

size_t line_start(const Buffer* buf, size_t position);
size_t line_end(const Buffer* buf, size_t position);
size_t line_position(const Buffer* buf, size_t line);
size_t find_text(const Buffer* buf, const char* text, size_t start, int backwards);
int matches_text(const Buffer* buf, const char* text, size_t position);
int replace_all(Editor* editor, const char* text, const char* replacement, size_t* count);
int insert_newline(Editor* editor);
void move_vertical(Editor* editor, int direction, size_t count);
void redraw_window(Editor* editor);
int prompt_input(Editor* editor, const char* label, char* input, size_t capacity);
void display_help(Editor* editor);
