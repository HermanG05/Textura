#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include "buffer.h"

#define LINE_NUMBER_WIDTH 4

typedef enum {
    BACKSPACE = 0x7F,
    ENTER = 0x0A,
    ESC = 0x1B,
    ARROW_UP = 0x41,
    ARROW_DOWN = 0x42,
    ARROW_LEFT = 0x44,
    ARROW_RIGHT = 0x43,
    CTRL_C = 0x03,
    CTRL_S = 0x13,
    CTRL_Q = 0x11,
    DEL_KEY = 0x7E,
} keyPress;

typedef struct {
    size_t initial_x_pos;
    size_t initial_y_pos;
    int status;
} cursor;

void display_line_number(size_t line_number, size_t y_pos);
cursor initial_buffer_render_on_window(Buffer* buf, size_t width, size_t height);
void redraw_window(Buffer* buf, size_t width);
void render_backspace_on_window(Buffer* buf, size_t x_pos, size_t y_pos, size_t width);
void render_delete_on_window(Buffer* buf, size_t x_pos, size_t y_pos, size_t width);
void render_space_on_window(Buffer* buf, size_t *x_pos, size_t *y_pos, size_t width);
void render_enter_on_window(Buffer* buf, size_t *x_pos, size_t *y_pos, size_t width);
void update_general_window(Buffer* buf, size_t* x_pos, size_t* y_pos, int ch, size_t width);
void display_status_bar(Buffer* buf, const char* filename, size_t x_pos, size_t y_pos);

