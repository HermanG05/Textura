#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>
#include "utils.h"
#include "buffer.h"

#define LINE_NUMBER_WIDTH 4

void display_line_number(size_t line_number, size_t y_pos) {
    char line_num_str[10];
    sprintf(line_num_str, "%3zu", line_number + 1);
    
    attron(A_DIM);
    mvprintw(y_pos, 0, "%s", line_num_str);
    attroff(A_DIM);
}

cursor initial_buffer_render_on_window(Buffer* buf, size_t width, size_t height) {
    cursor coordinates;

    if (!buf) {
        coordinates.status = -1;
        return coordinates;
    }    

    size_t Y_POS = 0;
    size_t X_POS = 1;
    size_t end_of_screen = height * (width - 2);

    buf->first_character = 0;
    buf->last_character = 0;

    clear();

    display_line_number(Y_POS, Y_POS);

    for (size_t index = 0; index < buf->gap_start; index++) {
        if ((X_POS - 1) + Y_POS * (width - 2) >= end_of_screen) {
            buf->last_character = index;
            break;
        }

        if (X_POS >= width - 1) {
            X_POS = 1;
            Y_POS++;
            
            display_line_number(Y_POS, Y_POS);
            
            if (Y_POS >= height) {
                buf->last_character = index;
                break;
            }
        }
        
        mvaddch(Y_POS, X_POS + LINE_NUMBER_WIDTH, buf->buffer[index]);
        X_POS++;
    }

    if (Y_POS < height) {
        for (size_t i = 0; i < (buf->text_size - buf->gap_start); i++) {
            size_t index = buf->gap_end + i;
            
            if (index >= buf->buffer_size || 
                (X_POS - 1) + Y_POS * (width - 2) >= end_of_screen) {
                break;
            }

            if (X_POS >= width - 1) {
                X_POS = 1;
                Y_POS++;
                
                display_line_number(Y_POS, Y_POS);
                
                if (Y_POS >= height) {
                    break;
                }
            }
            
            mvaddch(Y_POS, X_POS + LINE_NUMBER_WIDTH, buf->buffer[index]);
            X_POS++;
        }
    }

    coordinates.initial_x_pos = X_POS;
    coordinates.initial_y_pos = Y_POS;
    coordinates.status = 0;

    refresh();
    
    return coordinates;
}

void redraw_window(Buffer* buf, size_t width) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    
    size_t edit_area_height = (size_t)(rows - 2);
    
    clear();
    size_t Y_POS = 0;
    size_t X_POS = 1;
    size_t displayed_chars = 0;
    
    display_line_number(Y_POS, Y_POS);
    
    for (size_t i = buf->first_character; i < buf->gap_start; i++) {
        if (X_POS >= width - 1) {
            X_POS = 1;
            Y_POS++;
            
            if (Y_POS >= edit_area_height) {
                break;
            }
            
            display_line_number(Y_POS, Y_POS);
        }
        
        int color = COLOR_PAIR(1);
        
        attron(color);
        mvaddch(Y_POS, X_POS + LINE_NUMBER_WIDTH, buf->buffer[i]);
        attroff(color);
        X_POS++;
        displayed_chars++;
    }

    size_t chars_after_gap = buf->text_size - buf->gap_start;
    for (size_t i = 0; i < chars_after_gap; i++) {
        if (X_POS >= width - 1) {
            X_POS = 1;
            Y_POS++;
            
            if (Y_POS >= edit_area_height) {
                break;
            }
            
            display_line_number(Y_POS, Y_POS);
        }

        size_t buffer_idx = buf->gap_end + i;
        if (buffer_idx >= buf->buffer_size) {
            break;
        }
        
        int color = COLOR_PAIR(1);
        
        attron(color);
        mvaddch(Y_POS, X_POS + LINE_NUMBER_WIDTH, buf->buffer[buffer_idx]);
        attroff(color);
        X_POS++;
        displayed_chars++;
    }

    if (Y_POS < (size_t)(rows - 3)) {
        buf->last_character = buf->first_character + displayed_chars;
    }

    refresh();
}

void render_backspace_on_window(Buffer* buf, size_t x_pos, size_t y_pos, size_t width) {
    if (x_pos <= 1 && y_pos <= 0 && buf->first_character == 0) {
        return;
    }
    
    size_t screen_index = (x_pos - 1) + y_pos * (width - 2);
    size_t buffer_index = screen_index + buf->first_character;
    
    if (buffer_index == 0) {
        return;
    }
    
    if (x_pos == 1 && y_pos > 0) {
        y_pos--;
        x_pos = width - 1;
        
        screen_index = (x_pos - 1) + y_pos * (width - 2);
        buffer_index = screen_index + buf->first_character;
    }
    
    move_buffer_cursor(buf, buffer_index);
    delete_buffer(buf);
    
    redraw_window(buf, width);
    
    if (x_pos > 1) {
        x_pos--;
    } else if (y_pos > 0) {
        y_pos--;
        x_pos = width - 1;
    }
    
    move(y_pos, x_pos + LINE_NUMBER_WIDTH);
    refresh();
}

void render_space_on_window(Buffer* buf, size_t *x_pos, size_t *y_pos, size_t width) {
    size_t screen_index = (*x_pos - 1 - LINE_NUMBER_WIDTH) + *y_pos * (width - 2);
    size_t buffer_index = screen_index + buf->first_character;
    
    if (buffer_index <= buf->text_size) {
        move_buffer_cursor(buf, buffer_index);
    } else {
        move_buffer_cursor(buf, buf->text_size);
        while (buf->text_size < buffer_index) {
            insert_buffer(buf, ' ');
        }
    }
    
    insert_buffer(buf, ' ');

    redraw_window(buf, width);

    (*x_pos)++;
    if (*x_pos >= width - 1 + LINE_NUMBER_WIDTH) {
        *x_pos = 1 + LINE_NUMBER_WIDTH;
        (*y_pos)++;
    }
    move(*y_pos, *x_pos);
    refresh();
}

void render_enter_on_window(Buffer* buf, size_t *x_pos, size_t *y_pos, size_t width) {
    size_t screen_index = (*x_pos - 1 - LINE_NUMBER_WIDTH) + *y_pos * (width - 2);
    size_t curr_index = screen_index + buf->first_character;
    
    if (curr_index > buf->text_size) {
        curr_index = buf->text_size;
    }
    
    move_buffer_cursor(buf, curr_index);
    
    size_t content_after_size = buf->text_size - curr_index;
    char* content_after = NULL;
    
    if (content_after_size > 0) {
        content_after = malloc(content_after_size + 1);
        if (!content_after) {
            perror("Failed to allocate memory");
            return;
        }
        
        size_t idx = 0;
        for (size_t i = curr_index; i < buf->text_size; i++) {
            char ch;
            if (i < buf->gap_start) {
                ch = buf->buffer[i];
            } else {
                size_t adjusted_pos = buf->gap_end + (i - buf->gap_start);
                ch = buf->buffer[adjusted_pos];
            }
            content_after[idx++] = ch;
        }
        content_after[idx] = '\0';
        
        for (size_t i = 0; i < content_after_size; i++) {
            move_buffer_cursor(buf, buf->text_size - 1);
            delete_buffer(buf);
        }
    }
    
    size_t remain_on_line = (width - 2) - ((*x_pos - 1 - LINE_NUMBER_WIDTH));
    for (size_t i = 0; i < remain_on_line; i++) {
        insert_buffer(buf, ' ');
    }
    
    (*y_pos)++;
    *x_pos = 1 + LINE_NUMBER_WIDTH;
    
    display_line_number(*y_pos, *y_pos);
    
    if (content_after) {
        for (size_t i = 0; i < content_after_size; i++) {
            insert_buffer(buf, content_after[i]);
        }
        free(content_after);
    }
    
    redraw_window(buf, width);
    move(*y_pos, *x_pos);
    refresh();
}

void update_general_window(Buffer* buf, size_t* x_pos, size_t* y_pos, int ch, size_t width) {
    size_t screen_index = (*x_pos - 1 - LINE_NUMBER_WIDTH) + *y_pos * (width - 2);
    size_t buffer_index = screen_index + buf->first_character;

    if (*x_pos >= width - 1 + LINE_NUMBER_WIDTH) {
        *x_pos = 1 + LINE_NUMBER_WIDTH;
        (*y_pos)++;
    }
    
    if (buffer_index <= buf->text_size) {
        move_buffer_cursor(buf, buffer_index);
    } else {
        move_buffer_cursor(buf, buf->text_size);
        while (buf->text_size < buffer_index) {
            insert_buffer(buf, ' ');
        }
    }
    
    insert_buffer(buf, ch);
    
    redraw_window(buf, width);
    
    (*x_pos)++;
    if (*x_pos >= width - 1 + LINE_NUMBER_WIDTH) {
        *x_pos = 1 + LINE_NUMBER_WIDTH;
        (*y_pos)++;
    }
    move(*y_pos, *x_pos);
    refresh();
}

void render_delete_on_window(Buffer* buf, size_t x_pos, size_t y_pos, size_t width) {
    size_t screen_index = (x_pos - 1 - LINE_NUMBER_WIDTH) + y_pos * (width - 2);
    size_t buffer_index = screen_index + buf->first_character;
    
    if (buffer_index >= buf->text_size) {
        return;
    }
    
    move_buffer_cursor(buf, buffer_index + 1);
    delete_buffer(buf);
    
    redraw_window(buf, width);
    
    move(y_pos, x_pos);
    refresh();
}

size_t count_lines(Buffer* buf) {
    if (!buf || buf->text_size == 0) return 0;
    
    size_t line_count = 1;
    
    for (size_t i = 0; i < buf->gap_start; i++) {
        if (buf->buffer[i] == '\n') {
            line_count++;
        }
    }
    
    for (size_t i = buf->gap_end; i < buf->buffer_size && buf->buffer[i] != '\0'; i++) {
        if (buf->buffer[i] == '\n') {
            line_count++;
        }
    }
    
    return line_count;
}

size_t count_words(Buffer* buf) {
    if (!buf || buf->text_size == 0) return 0;
    
    size_t word_count = 0;
    int in_word = 0;
    
    for (size_t i = 0; i < buf->gap_start; i++) {
        char c = buf->buffer[i];
        if (isspace(c) || c == '\n') {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            word_count++;
        }
    }
    
    in_word = 0;
    
    for (size_t i = buf->gap_end; i < buf->buffer_size && buf->buffer[i] != '\0'; i++) {
        char c = buf->buffer[i];
        if (isspace(c) || c == '\n') {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            word_count++;
        }
    }
    
    return word_count;
}

size_t count_non_space_chars(Buffer* buf) {
    if (!buf || buf->text_size == 0) return 0;
    
    size_t char_count = 0;
    
    for (size_t i = 0; i < buf->gap_start; i++) {
        if (buf->buffer[i] != ' ' && buf->buffer[i] != '\n' && buf->buffer[i] != '\t') {
            char_count++;
        }
    }
    
    for (size_t i = buf->gap_end; i < buf->buffer_size && buf->buffer[i] != '\0'; i++) {
        if (buf->buffer[i] != ' ' && buf->buffer[i] != '\n' && buf->buffer[i] != '\t') {
            char_count++;
        }
    }
    
    return char_count;
}

void display_status_bar(Buffer* buf, const char* filename, size_t x_pos, size_t y_pos) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    
    int cur_y, cur_x;
    getyx(stdscr, cur_y, cur_x);
    
    size_t line_count = count_lines(buf);
    size_t char_count = count_non_space_chars(buf);
    size_t word_count = count_words(buf);
    
    const char* short_filename = filename;
    const char* last_slash = strrchr(filename, '/');
    if (last_slash) {
        short_filename = last_slash + 1;
    }
    
    char status_left[64];
    char status_right[192];
    
    snprintf(status_left, sizeof(status_left), " %s ", 
             short_filename ? short_filename : "Untitled");
             
    snprintf(status_right, sizeof(status_right), " UTF-8 | L: %zu | Ch: %zu | W: %zu | %zu:%zu ", 
             line_count, 
             char_count, 
             word_count,
             y_pos + 1, 
             x_pos - LINE_NUMBER_WIDTH + 1);
    
    move(rows - 2, 0);
    
    size_t right_text_len = strlen(status_right);
    size_t left_text_len = strlen(status_left);
    int right_start = cols - right_text_len;
    
    if (right_start < (int)left_text_len) {
        right_start = left_text_len;
    }
    
    for (int i = 0; i < cols; i++) {
        mvaddch(rows - 2, i, ' ');
    }
    
    static int colors_initialized = 0;
    if (!colors_initialized) {
        start_color();
        init_pair(20, COLOR_BLACK, COLOR_WHITE);
        init_pair(21, COLOR_BLACK, COLOR_WHITE);
        colors_initialized = 1;
    }
    
    attron(A_REVERSE);
    mvprintw(rows - 2, 0, "%s", status_left);
    
    for (int i = left_text_len; i < right_start; i++) {
        mvaddch(rows - 2, i, ' ');
    }
    
    mvprintw(rows - 2, right_start, "%s", status_right);
    attroff(A_REVERSE);
    
    move(cur_y, cur_x);
    refresh();
}
