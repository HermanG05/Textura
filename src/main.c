#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <ncurses.h>
#include "buffer.h"
#include "utils.h"
#include "history.h"
#include "syntax.h"

#define CTRL(c) ((c) & 037)

static void search_text(Editor* editor, int backwards, int include_current) {
    if (!editor->search[0]) {
        snprintf(editor->message, sizeof(editor->message), "Use Ctrl+F to enter search text");
        return;
    }
    size_t start = editor->cursor_pos;
    if (!include_current) {
        if (backwards) start = start ? start - 1 : editor->buf->text_size;
        else start++;
    }
    size_t found = find_text(editor->buf, editor->search, start, backwards);
    if (found == SIZE_MAX) {
        snprintf(editor->message, sizeof(editor->message), "No matches for: %s", editor->search);
    } else {
        editor->cursor_pos = found;
        snprintf(editor->message, sizeof(editor->message), "Found: %s | Ctrl+N next, Ctrl+P previous, Esc clear", editor->search);
    }
}

static void go_to_line(Editor* editor) {
    char input[INPUT_SIZE] = {0};
    if (!prompt_input(editor, "Go to line: ", input, sizeof(input))) return;
    char* end;
    errno = 0;
    unsigned long long line = strtoull(input, &end, 10);
    if (input[0] < '0' || input[0] > '9' || *end || errno || !line || line > SIZE_MAX) {
        snprintf(editor->message, sizeof(editor->message), "Enter a positive line number");
        return;
    }
    size_t position = line_position(editor->buf, (size_t)line);
    size_t actual_line = 1;
    for (size_t i = 0; i < position; i++) {
        if (buffer_character(editor->buf, i) == '\n') actual_line++;
    }
    if (actual_line != line) {
        snprintf(editor->message, sizeof(editor->message), "Line does not exist");
        return;
    }
    editor->cursor_pos = position;
}

static void replace_text(Editor* editor) {
    char text[INPUT_SIZE];
    char replacement[INPUT_SIZE] = {0};
    snprintf(text, sizeof(text), "%s", editor->search);
    if (!prompt_input(editor, "Replace text: ", text, sizeof(text)) || !text[0]) return;
    if (!prompt_input(editor, "Replace with (empty deletes): ", replacement, sizeof(replacement))) return;
    size_t count;
    if (!replace_all(editor, text, replacement, &count)) {
        snprintf(editor->message, sizeof(editor->message), "Replace failed: not enough memory");
        return;
    }
    snprintf(editor->search, sizeof(editor->search), "%s", text);
    snprintf(editor->message, sizeof(editor->message), "%zu matches replaced | Ctrl+Z to undo", count);
}

int main(int argc, char** argv) {
    char filename[4096] = {0};
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
        return 1;
    }
    if (argc == 2) {
        if (strlen(argv[1]) >= sizeof(filename)) {
            fprintf(stderr, "Filename is too long\n");
            return 1;
        }
        strcpy(filename, argv[1]);
    } else {
        printf("Enter a file name: ");
        if (!fgets(filename, sizeof(filename), stdin)) return 1;
        filename[strcspn(filename, "\r\n")] = '\0';
        if (!filename[0]) return 1;
    }
    Buffer* buf = create_buffer();
    History* history = create_history(1000);
    if (!buf || !history || !load_file_into_buffer(filename, buf)) {
        fprintf(stderr, "Unable to open %s or allocate editor memory\n", filename);
        free_history(history);
        free_buffer(buf);
        return 1;
    }
    Editor editor = {0};
    editor.buf = buf;
    editor.history = history;
    editor.filename = filename;
    editor.auto_indent = 1;

    initscr();
    initialize_syntax_colors(editor.light_theme);
    raw();
    noecho();
    keypad(stdscr, TRUE);
    int running = 1;
    int quit_pending = 0;
    while (running) {
        redraw_window(&editor);
        int ch = getch();
        if (ch == KEY_RESIZE) continue;
        editor.message[0] = '\0';
        if (ch != CTRL('q')) quit_pending = 0;
        int success = 1;
        switch (ch) {
            case CTRL('q'):
                if (history->revision != history->saved_revision && !quit_pending) {
                    snprintf(editor.message, sizeof(editor.message), "Unsaved changes: Ctrl+S to save, Ctrl+Q again to discard");
                    quit_pending = 1;
                } else running = 0;
                break;
            case CTRL('s'):
                if (save_contents_to_file(filename, buf)) {
                    history->saved_revision = history->revision;
                    snprintf(editor.message, sizeof(editor.message), "File saved");
                } else snprintf(editor.message, sizeof(editor.message), "Save failed: %s", strerror(errno));
                break;
            case CTRL('z'):
                if (!undo(history, buf, &editor.cursor_pos)) snprintf(editor.message, sizeof(editor.message), "Nothing to undo");
                break;
            case CTRL('y'):
                if (!redo(history, buf, &editor.cursor_pos)) snprintf(editor.message, sizeof(editor.message), "Nothing to redo");
                break;
            case CTRL('f'): {
                char text[INPUT_SIZE];
                snprintf(text, sizeof(text), "%s", editor.search);
                if (prompt_input(&editor, "Find: ", text, sizeof(text))) {
                    snprintf(editor.search, sizeof(editor.search), "%s", text);
                    search_text(&editor, 0, 1);
                }
                break;
            }
            case CTRL('n'):
                search_text(&editor, 0, 0);
                break;
            case CTRL('p'):
                search_text(&editor, 1, 0);
                break;
            case CTRL('r'):
                replace_text(&editor);
                break;
            case CTRL('g'):
                go_to_line(&editor);
                break;
            case 27:
                editor.search[0] = '\0';
                break;
            case KEY_F(1):
                display_help(&editor);
                break;
            case KEY_F(2):
                editor.auto_indent = !editor.auto_indent;
                break;
            case KEY_F(3):
                editor.syntax_mode = (editor.syntax_mode + 1) % 6;
                snprintf(editor.message, sizeof(editor.message), "Syntax: %s",
                         editor.syntax_mode ? language_name((Language)(editor.syntax_mode - 1)) : "Auto");
                break;
            case KEY_F(4):
                editor.light_theme = !editor.light_theme;
                initialize_syntax_colors(editor.light_theme);
                snprintf(editor.message, sizeof(editor.message), "Syntax palette: %s", editor.light_theme ? "light" : "dark");
                break;
            case KEY_HOME:
            case CTRL('a'):
                editor.cursor_pos = line_start(buf, editor.cursor_pos);
                break;
            case KEY_END:
            case CTRL('e'):
                editor.cursor_pos = line_end(buf, editor.cursor_pos);
                break;
            case KEY_UP:
                move_vertical(&editor, -1, 1);
                break;
            case KEY_DOWN:
                move_vertical(&editor, 1, 1);
                break;
            case KEY_PPAGE:
            case KEY_NPAGE:
                move_vertical(&editor, ch == KEY_PPAGE ? -1 : 1, LINES > 2 ? (size_t)(LINES - 2) : 1);
                break;
            case KEY_LEFT:
                if (editor.cursor_pos) editor.cursor_pos--;
                break;
            case KEY_RIGHT:
                if (editor.cursor_pos < buf->text_size) editor.cursor_pos++;
                break;
            case KEY_BACKSPACE:
            case 127:
            case 8:
                if (editor.cursor_pos) {
                    success = record_edit(history, buf, editor.cursor_pos - 1, 1, "", 0,
                                          &editor.cursor_pos, editor.cursor_pos - 1);
                }
                break;
            case KEY_DC:
                if (editor.cursor_pos < buf->text_size) {
                    success = record_edit(history, buf, editor.cursor_pos, 1, "", 0,
                                          &editor.cursor_pos, editor.cursor_pos);
                }
                break;
            case '\n':
            case '\r':
            case KEY_ENTER:
                success = insert_newline(&editor);
                break;
            default:
                if ((ch >= 32 && ch <= 126) || ch == '\t') {
                    char character = (char)ch;
                    success = record_edit(history, buf, editor.cursor_pos, 0, &character, 1,
                                          &editor.cursor_pos, editor.cursor_pos + 1);
                }
                break;
        }
        if (!success) snprintf(editor.message, sizeof(editor.message), "Edit failed: not enough memory");
    }
    endwin();
    free_history(history);
    free_buffer(buf);
    return 0;
}
