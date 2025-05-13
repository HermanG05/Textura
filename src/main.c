#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include "buffer.h"
#include "utils.h"
#include "history.h"

#define CTRL(c) ((c) & 037)
#define LINE_NUMBER_WIDTH 4

size_t get_buffer_position(size_t x_pos, size_t y_pos, size_t width, size_t first_character) {
    size_t screen_index = (x_pos - 1) + y_pos * (width - 2);
    return screen_index + first_character;
}

void display_status_message(const char* message) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    
    int cur_y, cur_x;
    getyx(stdscr, cur_y, cur_x);
    
    move(rows - 1, 0);
    clrtoeol();
    
    attron(A_REVERSE);
    mvprintw(rows - 1, 0, "%s", message);
    attroff(A_REVERSE);
    
    for (int i = strlen(message); i < cols; i++) {
        addch(' ');
    }
    
    move(cur_y, cur_x);
    refresh();
}

int main(int argc __attribute__((unused)), char** argv) {
    char filename[256] = {0};
    
    if (!argv[1]) {
        char ch;
        printf("No file Specified, would you like to create a file? Y/N: ");
        scanf("%c", &ch);
        if (ch == 'Y' || ch == 'y') {
            printf("Enter a file name: ");
            scanf("%255s", filename);
            create_new_file(filename);
        } else {
            printf("No file specified. Exiting.\n");
            return 1;
        }
    } else {
        strncpy(filename, argv[1], sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';
    }
    
    Buffer* buf = create_buffer();
    
    History* history = create_history(100);
    
    initscr();
    raw();
    noecho();        
    keypad(stdscr, TRUE);  
    refresh();
    
    size_t width, height;
    size_t X_POS = 1 + LINE_NUMBER_WIDTH, Y_POS = 0;
    getmaxyx(stdscr, height, width);
    load_file_into_buffer(filename, buf, width);
    int ch;  
    cursor initial_coordinates = initial_buffer_render_on_window(buf, width, height);

    if (initial_coordinates.status == 0) {
        X_POS = initial_coordinates.initial_x_pos + LINE_NUMBER_WIDTH;
        Y_POS = initial_coordinates.initial_y_pos;
    }
    
    display_status_bar(buf, filename, X_POS, Y_POS);
    
    while ((ch = getch()) != CTRL_Q) {
        size_t pre_x = X_POS;
        size_t pre_y = Y_POS;
        size_t buffer_pos = get_buffer_position(X_POS - LINE_NUMBER_WIDTH, Y_POS, width, buf->first_character);
        
        if (ch == CTRL('z')) {
            if (undo(history, buf, &X_POS, &Y_POS, width)) {
                redraw_window(buf, width);
                move(Y_POS, X_POS);
            }
            display_status_bar(buf, filename, X_POS, Y_POS);
            continue;
        } else if (ch == CTRL('y')) {
            if (redo(history, buf, &X_POS, &Y_POS, width)) {
                redraw_window(buf, width);
                move(Y_POS, X_POS);
            }
            display_status_bar(buf, filename, X_POS, Y_POS);
            continue;
        } else if (ch == CTRL('s')) {
            save_contents_to_file(filename, buf, width);
            display_status_message("File saved");
            display_status_bar(buf, filename, X_POS, Y_POS);
            move(Y_POS, X_POS);
            continue;
        }
        
        switch (ch) {
            case KEY_BACKSPACE:
                if (X_POS > 1 + LINE_NUMBER_WIDTH) {
                    size_t del_pos = (X_POS - 2 - LINE_NUMBER_WIDTH) + Y_POS * (width - 2) + buf->first_character;
                    char del_char = ' ';
                    
                    if (del_pos < buf->text_size) {
                        if (del_pos < buf->gap_start) {
                            del_char = buf->buffer[del_pos];
                        } else {
                            del_char = buf->buffer[buf->gap_end + (del_pos - buf->gap_start)];
                        }
                    }
                    
                    X_POS--;
                    record_delete(history, del_pos, del_char, pre_x, pre_y);
                    render_backspace_on_window(buf, X_POS - LINE_NUMBER_WIDTH, Y_POS, width);
                    display_status_bar(buf, filename, X_POS, Y_POS);
                } else if (X_POS == 1 + LINE_NUMBER_WIDTH && Y_POS > 0) {
                    size_t prev_line_end = ((Y_POS - 1) * (width - 2) + (width - 2) - 1) + buf->first_character;
                    char del_char = ' ';
                    
                    if (prev_line_end < buf->text_size) {
                        if (prev_line_end < buf->gap_start) {
                            del_char = buf->buffer[prev_line_end];
                        } else {
                            del_char = buf->buffer[buf->gap_end + (prev_line_end - buf->gap_start)];
                        }
                    }
                    
                    X_POS = width - 1;
                    Y_POS--;
                    record_delete(history, prev_line_end, del_char, pre_x, pre_y);
                    move(Y_POS, X_POS);
                    display_status_bar(buf, filename, X_POS, Y_POS);
                }
                break;
            case KEY_DC:
                if (X_POS > 1 + LINE_NUMBER_WIDTH) {
                    size_t del_pos = (X_POS - 2 - LINE_NUMBER_WIDTH) + Y_POS * (width - 2) + buf->first_character;
                    char del_char = ' ';
                    
                    if (del_pos < buf->text_size) {
                        if (del_pos < buf->gap_start) {
                            del_char = buf->buffer[del_pos];
                        } else {
                            del_char = buf->buffer[buf->gap_end + (del_pos - buf->gap_start)];
                        }
                    }
                    
                    X_POS--;
                    record_delete(history, del_pos, del_char, pre_x, pre_y);
                    render_backspace_on_window(buf, X_POS - LINE_NUMBER_WIDTH, Y_POS, width);
                    display_status_bar(buf, filename, X_POS, Y_POS);
                } else if (X_POS == 1 + LINE_NUMBER_WIDTH && Y_POS > 0) {
                    X_POS = width - 1;
                    Y_POS--;
                    move(Y_POS, X_POS);
                    display_status_bar(buf, filename, X_POS, Y_POS);
                }
                break;
            case KEY_UP:
                if (Y_POS == 0 && buf->first_character > 0) {
                    buf->first_character -= (width - 2);
                    buf->last_character -= (width - 2);
                    redraw_window(buf, width);
                    
                    move(Y_POS, X_POS);
                }
                if (Y_POS > 0) {
                    Y_POS--;
                    move(Y_POS, X_POS);
                }
                display_status_bar(buf, filename, X_POS, Y_POS);
                break;
            case KEY_DOWN:
                if (Y_POS >= height - 1) {
                    size_t max_first = buf->text_size > (width - 2) ? buf->text_size - (width - 2) : 0;
                    
                    if (buf->first_character < max_first) {
                        buf->first_character += (width - 2);
                        buf->last_character += (width - 2);
                        
                        if (buf->first_character > max_first) {
                            buf->first_character = max_first;
                        }
                        
                        size_t buffer_index = (X_POS - 1) + Y_POS * (width - 2) + buf->first_character;
                        
                        if (buffer_index > buf->text_size) {
                            move_buffer_cursor(buf, buf->text_size);
                            
                            while (buf->text_size < buffer_index) {
                                if ((buf->text_size % (width - 2)) == 0 && buf->text_size > 0) {
                                    insert_buffer(buf, '\n');
                                } else {
                                    insert_buffer(buf, ' ');
                                }
                            }
                        }
                        
                        move_buffer_cursor(buf, buffer_index);
                        redraw_window(buf, width);
                        
                        move(Y_POS, X_POS);
                    }
                } else {
                    Y_POS++;
                    move(Y_POS, X_POS);
                }
                display_status_bar(buf, filename, X_POS, Y_POS);
                break;
            case KEY_LEFT:
                if (X_POS > 1 + LINE_NUMBER_WIDTH) {
                    X_POS--;
                    move(Y_POS, X_POS);
                } else if (X_POS == 1 + LINE_NUMBER_WIDTH && Y_POS > 0) {
                    X_POS = width - 1;
                    Y_POS--;
                    
                    if (Y_POS == 0 && buf->first_character > 0) {
                        buf->first_character -= (width - 2);
                        buf->last_character -= (width - 2);
                        redraw_window(buf, width);
                        Y_POS = 0;
                    }
                    
                    move(Y_POS, X_POS);
                }
                display_status_bar(buf, filename, X_POS, Y_POS);
                break;
            case KEY_RIGHT:
                if (X_POS < width - 1) {
                    X_POS++;
                    move(Y_POS, X_POS);
                } else if (X_POS == width - 1) {
                    X_POS = 1;
                    Y_POS++;
                    
                    if (Y_POS >= height - 1) {
                        size_t max_first = buf->text_size > (width - 2) ? buf->text_size - (width - 2) : 0;
                        
                        if (buf->first_character < max_first) {
                            buf->first_character += (width - 2);
                            buf->last_character += (width - 2);
                            
                            if (buf->first_character > max_first) {
                                buf->first_character = max_first;
                            }
                            
                            redraw_window(buf, width);
                            Y_POS = height - 2;
                        }
                    }
                    
                    move(Y_POS, X_POS);
                }
                display_status_bar(buf, filename, X_POS, Y_POS);
                break;
            case ' ':
                record_insert(history, buffer_pos, ' ', pre_x, pre_y);
                render_space_on_window(buf, &X_POS, &Y_POS, width);
                display_status_bar(buf, filename, X_POS, Y_POS);
                break;
            case 0x0A:
                start_batch(history);
                
                record_enter(history, buffer_pos, pre_x, pre_y);
                
                render_enter_on_window(buf, &X_POS, &Y_POS, width);
                
                end_batch(history);
                display_status_bar(buf, filename, X_POS, Y_POS);
                break;
            default:
                if (ch >= 32 && ch <= 126) {
                    record_insert(history, buffer_pos, ch, pre_x, pre_y);
                }
                update_general_window(buf, &X_POS, &Y_POS, ch, width);
                display_status_bar(buf, filename, X_POS, Y_POS);
        }
        
        refresh();
    }
    save_contents_to_file(filename, buf, width);
    endwin();
    
    free_history(history);
    free_buffer(buf);
    return 0;
}
