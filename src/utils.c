#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <ncurses.h>
#include "utils.h"
#include "syntax.h"

size_t line_start(const Buffer* buf, size_t position) {
    while (position > 0 && buffer_character(buf, position - 1) != '\n') position--;
    return position;
}

size_t line_end(const Buffer* buf, size_t position) {
    while (position < buf->text_size && buffer_character(buf, position) != '\n') position++;
    return position;
}

size_t line_position(const Buffer* buf, size_t line) {
    size_t position = 0;
    while (line > 1 && position < buf->text_size) {
        if (buffer_character(buf, position++) == '\n') line--;
    }
    return position;
}

int matches_text(const Buffer* buf, const char* text, size_t position) {
    size_t length = strlen(text);
    if (!length || position > buf->text_size || length > buf->text_size - position) return 0;
    for (size_t i = 0; i < length; i++) {
        if (buffer_character(buf, position + i) != text[i]) return 0;
    }
    return 1;
}

size_t find_text(const Buffer* buf, const char* text, size_t start, int backwards) {
    if (!text[0] || !buf->text_size) return SIZE_MAX;
    if (start >= buf->text_size) start = backwards ? buf->text_size - 1 : 0;
    size_t position = start;
    do {
        if (matches_text(buf, text, position)) return position;
        if (backwards) position = position ? position - 1 : buf->text_size - 1;
        else position = position + 1 == buf->text_size ? 0 : position + 1;
    } while (position != start);
    return SIZE_MAX;
}

int replace_all(Editor* editor, const char* text, const char* replacement, size_t* count) {
    Buffer* buf = editor->buf;
    size_t length = strlen(text);
    size_t replacement_size = strlen(replacement);
    *count = 0;
    if (!length) return 1;
    for (size_t i = 0; i < buf->text_size;) {
        if (matches_text(buf, text, i)) {
            (*count)++;
            i += length;
        } else i++;
    }
    if (!*count || strcmp(text, replacement) == 0) return 1;
    size_t new_size = buf->text_size - *count * length;
    if (replacement_size && *count > (SIZE_MAX - new_size) / replacement_size) return 0;
    new_size += *count * replacement_size;
    char* result = (char*)malloc(new_size ? new_size : 1);
    if (!result) return 0;
    size_t output = 0;
    size_t cursor_after = 0;
    int first_match = 1;
    for (size_t i = 0; i < buf->text_size;) {
        if (matches_text(buf, text, i)) {
            if (replacement_size) memcpy(result + output, replacement, replacement_size);
            if (first_match) {
                cursor_after = output;
                first_match = 0;
            }
            output += replacement_size;
            i += length;
        } else result[output++] = buffer_character(buf, i++);
    }
    int success = record_edit(editor->history, buf, 0, buf->text_size, result, new_size,
                              &editor->cursor_pos, cursor_after);
    free(result);
    return success;
}

int insert_newline(Editor* editor) {
    size_t start = line_start(editor->buf, editor->cursor_pos);
    size_t indent = 0;
    if (editor->auto_indent) {
        while (start + indent < editor->cursor_pos) {
            char ch = buffer_character(editor->buf, start + indent);
            if (ch != ' ' && ch != '\t') break;
            indent++;
        }
    }
    char* text = (char*)malloc(indent + 1);
    if (!text) return 0;
    text[0] = '\n';
    for (size_t i = 0; i < indent; i++) text[i + 1] = buffer_character(editor->buf, start + i);
    int success = record_edit(editor->history, editor->buf, editor->cursor_pos, 0, text, indent + 1,
                              &editor->cursor_pos, editor->cursor_pos + indent + 1);
    free(text);
    return success;
}

static size_t display_column(const Buffer* buf, size_t start, size_t end) {
    size_t column = 0;
    for (size_t i = start; i < end; i++) {
        column += buffer_character(buf, i) == '\t' ? TAB_WIDTH - column % TAB_WIDTH : 1;
    }
    return column;
}

void move_vertical(Editor* editor, int direction, size_t count) {
    Buffer* buf = editor->buf;
    size_t start = line_start(buf, editor->cursor_pos);
    size_t column = display_column(buf, start, editor->cursor_pos);
    while (count--) {
        if (direction < 0) {
            if (!start) break;
            start = line_start(buf, start - 1);
        } else {
            size_t end = line_end(buf, start);
            if (end == buf->text_size) break;
            start = end + 1;
        }
    }
    size_t position = start;
    size_t current_column = 0;
    while (position < buf->text_size && buffer_character(buf, position) != '\n') {
        size_t width = buffer_character(buf, position) == '\t' ? TAB_WIDTH - current_column % TAB_WIDTH : 1;
        if (current_column + width > column) break;
        current_column += width;
        position++;
    }
    editor->cursor_pos = position;
}

void redraw_window(Editor* editor) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    erase();
    if (rows < 4 || cols < 16) {
        addnstr("Resize terminal", cols > 0 ? cols - 1 : 0);
        refresh();
        return;
    }
    Buffer* buf = editor->buf;
    size_t cursor_line = 0;
    size_t lines = 1;
    size_t words = 0;
    int in_word = 0;
    for (size_t i = 0; i < buf->text_size; i++) {
        unsigned char ch = (unsigned char)buffer_character(buf, i);
        if (ch == '\n') {
            lines++;
            if (i < editor->cursor_pos) cursor_line++;
        }
        if (isspace(ch)) in_word = 0;
        else if (!in_word) {
            words++;
            in_word = 1;
        }
    }
    size_t height = (size_t)(rows - 2);
    size_t width = (size_t)(cols - LINE_NUMBER_WIDTH);
    size_t column = display_column(buf, line_start(buf, editor->cursor_pos), editor->cursor_pos);
    if (cursor_line < editor->top_line) editor->top_line = cursor_line;
    if (cursor_line >= editor->top_line + height) editor->top_line = cursor_line - height + 1;
    if (column < editor->left_column) editor->left_column = column;
    if (column >= editor->left_column + width) editor->left_column = column - width + 1;
    size_t position = line_position(buf, editor->top_line + 1);
    size_t search_length = strlen(editor->search);
    Language language = editor->syntax_mode ? (Language)(editor->syntax_mode - 1) : detect_language(editor->filename);
    SyntaxToken token = {0, 0, SYNTAX_NORMAL};
    SyntaxScanner scanner = {0};
    scanner.buf = buf;
    scanner.language = language;
    for (size_t row = 0; row < height && editor->top_line + row < lines; row++) {
        attron(A_DIM);
        mvprintw((int)row, 0, "%5zu", (editor->top_line + row + 1) % 100000);
        attroff(A_DIM);
        size_t current_column = 0;
        size_t highlighted_until = 0;
        while (position < buf->text_size && buffer_character(buf, position) != '\n') {
            unsigned char ch = (unsigned char)buffer_character(buf, position);
            size_t cells = ch == '\t' ? TAB_WIDTH - current_column % TAB_WIDTH : 1;
            if (search_length && matches_text(buf, editor->search, position)) highlighted_until = position + search_length;
            while (token.end <= position) token = next_highlight_token(&scanner);
            int attributes = syntax_attributes(token.type);
            if (position < highlighted_until) attributes |= A_REVERSE;
            attron(attributes);
            for (size_t i = 0; i < cells; i++) {
                size_t screen_column = current_column + i;
                if (screen_column >= editor->left_column && screen_column - editor->left_column < width) {
                    mvaddch((int)row, (int)(LINE_NUMBER_WIDTH + screen_column - editor->left_column),
                            ch == '\t' ? ' ' : (ch >= 32 && ch <= 126 ? ch : '?'));
                }
            }
            attroff(attributes);
            current_column += cells;
            position++;
        }
        if (position < buf->text_size) position++;
    }
    char status[1024];
    const char* filename = strrchr(editor->filename, '/');
    filename = filename ? filename + 1 : editor->filename;
    snprintf(status, sizeof(status), " %s%s | %zu:%zu | %zu lines | %zu words | %s | indent %s",
             filename, editor->history->revision != editor->history->saved_revision ? " [+]" : "",
             cursor_line + 1, column + 1, lines, words, language_name(language), editor->auto_indent ? "on" : "off");
    attron(A_REVERSE);
    mvhline(rows - 2, 0, ' ', cols);
    mvaddnstr(rows - 2, 0, status, cols - 1);
    attroff(A_REVERSE);
    mvaddnstr(rows - 1, 0, editor->message[0] ? editor->message : "^S Save  ^F Find  ^R Replace  ^G Line  ^Q Quit  F1 Help", cols - 1);
    move((int)(cursor_line - editor->top_line), (int)(LINE_NUMBER_WIDTH + column - editor->left_column));
    refresh();
}

int prompt_input(Editor* editor, const char* label, char* input, size_t capacity) {
    size_t length = strlen(input);
    for (;;) {
        redraw_window(editor);
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        move(rows - 1, 0);
        clrtoeol();
        int label_size = (int)strlen(label);
        if (label_size > cols / 2) label_size = cols / 2;
        addnstr(label, label_size);
        size_t available = (size_t)(cols - label_size - 1);
        size_t offset = length > available ? length - available : 0;
        addnstr(input + offset, (int)available);
        refresh();
        int ch = getch();
        if (ch == 27) return 0;
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) return 1;
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (length) input[--length] = '\0';
        } else if (ch == 21) {
            length = 0;
            input[0] = '\0';
        } else if (ch >= 32 && ch <= 126 && length + 1 < capacity) {
            input[length++] = (char)ch;
            input[length] = '\0';
        }
    }
}

void display_help(Editor* editor) {
    const char* lines[] = {
        "Textura shortcuts",
        "Ctrl+S       Save",
        "Ctrl+Q       Quit (confirm when modified)",
        "Ctrl+Z / Y   Undo / redo",
        "Ctrl+F       Find literal text (case sensitive)",
        "Ctrl+N / P   Next / previous match, wrapping",
        "Ctrl+R       Replace all, undone in one step",
        "Ctrl+G       Go to line",
        "Ctrl+A / E   Start / end of line (also Home / End)",
        "Page Up/Down Move one screen",
        "Enter        Newline with current indentation",
        "Tab          Insert a tab (4-column display)",
        "F2           Toggle automatic indentation",
        "F3           Cycle language: Auto, Text, C, C++, Java, Python",
        "F4           Switch dark / light syntax palette",
        "Esc          Clear search highlights / cancel prompt",
        "Ctrl+U       Clear prompt input",
        "F1           Show this help",
        "Press any key to return"
    };
    for (;;) {
        erase();
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        size_t count = sizeof(lines) / sizeof(lines[0]);
        for (size_t i = 0; i < count && i < (size_t)(rows - 1); i++) {
            mvaddnstr((int)i, 0, lines[i], cols - 1);
        }
        mvaddnstr(rows - 1, 0, "Press any key to return", cols - 1);
        refresh();
        if (getch() != KEY_RESIZE) break;
    }
    redraw_window(editor);
}
