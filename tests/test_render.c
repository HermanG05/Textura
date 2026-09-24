#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include "syntax.h"
#include "utils.h"

static void set_text(Editor* editor, const char* filename, const char* text) {
    assert(replace_buffer(editor->buf, 0, editor->buf->text_size, text, strlen(text)));
    editor->filename = filename;
    editor->cursor_pos = 0;
    editor->top_line = 0;
    editor->left_column = 0;
    editor->search[0] = '\0';
    redraw_window(editor);
}

static chtype cell_for(Editor* editor, const char* text, const char* fragment) {
    const char* match = strstr(text, fragment);
    assert(match);
    size_t line = 0;
    size_t column = 0;
    for (const char* current = text; current < match; current++) {
        if (*current == '\n') {
            line++;
            column = 0;
        } else column++;
    }
    assert(line >= editor->top_line && column >= editor->left_column);
    chtype cell = mvwinch(stdscr, (int)(line - editor->top_line),
                         (int)(column - editor->left_column + LINE_NUMBER_WIDTH));
    assert((cell & A_CHARTEXT) == (unsigned char)*fragment);
    return cell;
}

static void assert_color(Editor* editor, const char* text, const char* fragment, SyntaxType type) {
    assert(PAIR_NUMBER(cell_for(editor, text, fragment)) == (int)type);
}

int main(void) {
    FILE* input = tmpfile();
    FILE* output = tmpfile();
    assert(input && output);
    SCREEN* screen = newterm("xterm-256color", output, input);
    assert(screen);
    assert(resize_term(24, 100) == OK);
    initialize_syntax_colors(0);
    Editor editor = {0};
    editor.buf = create_buffer();
    editor.history = create_history(10);
    assert(editor.buf && editor.history);
    const char* c_text = "#include <stdio.h>\nsize_t length = strlen(value);\nBuffer* buf = NULL;\nbuf->text_size += INITIAL_BUFFER_SIZE;\n/* comment */";
    set_text(&editor, "example.c", c_text);
    assert_color(&editor, c_text, "#include", SYNTAX_DIRECTIVE);
    assert_color(&editor, c_text, "<stdio.h>", SYNTAX_STRING);
    assert_color(&editor, c_text, "size_t", SYNTAX_TYPE);
    assert_color(&editor, c_text, "length", SYNTAX_VARIABLE);
    assert_color(&editor, c_text, "strlen", SYNTAX_FUNCTION);
    assert_color(&editor, c_text, "=", SYNTAX_OPERATOR);
    assert_color(&editor, c_text, "(", SYNTAX_PUNCTUATION);
    assert_color(&editor, c_text, "Buffer", SYNTAX_TYPE);
    assert_color(&editor, c_text, "NULL", SYNTAX_CONSTANT);
    assert_color(&editor, c_text, "text_size", SYNTAX_MEMBER);
    assert_color(&editor, c_text, "INITIAL_BUFFER_SIZE", SYNTAX_CONSTANT);
    assert_color(&editor, c_text, "comment", SYNTAX_COMMENT);
    strcpy(editor.search, "strlen");
    redraw_window(&editor);
    chtype matched = cell_for(&editor, c_text, "strlen");
    assert(PAIR_NUMBER(matched) == SYNTAX_FUNCTION);
    assert(matched & A_REVERSE);
    assert(matched & A_BOLD);
    assert(!(cell_for(&editor, c_text, "value") & A_REVERSE));

    const char* cpp_text = "std::vector<std::string> names;\nnames.push_back(\"hello\\n\");\nstd::cout << names.size();";
    set_text(&editor, "example.cpp", cpp_text);
    assert_color(&editor, cpp_text, "vector", SYNTAX_TYPE);
    assert_color(&editor, cpp_text, "push_back", SYNTAX_FUNCTION);
    assert_color(&editor, cpp_text, "hello", SYNTAX_STRING);
    assert_color(&editor, cpp_text, "\\n", SYNTAX_ESCAPE);
    assert_color(&editor, cpp_text, "cout", SYNTAX_BUILTIN);

    const char* java_text = "@Override\npublic String label() {\n    return String.format(\"%s\", this.name);\n}";
    set_text(&editor, "Example.java", java_text);
    assert_color(&editor, java_text, "@Override", SYNTAX_DIRECTIVE);
    assert_color(&editor, java_text, "public", SYNTAX_KEYWORD);
    assert_color(&editor, java_text, "String", SYNTAX_TYPE);
    assert_color(&editor, java_text, "label", SYNTAX_FUNCTION);
    assert_color(&editor, java_text, "format", SYNTAX_FUNCTION);
    assert_color(&editor, java_text, "name", SYNTAX_MEMBER);

    const char* python_text = "@cache\ndef label(user):\n    return f\"Hello {user.name}: {len(items) + 42}\\n\"";
    set_text(&editor, "example.py", python_text);
    assert_color(&editor, python_text, "@cache", SYNTAX_DIRECTIVE);
    assert_color(&editor, python_text, "def", SYNTAX_KEYWORD);
    assert_color(&editor, python_text, "label", SYNTAX_FUNCTION);
    assert_color(&editor, python_text, "Hello", SYNTAX_STRING);
    assert_color(&editor, python_text, "name", SYNTAX_MEMBER);
    assert_color(&editor, python_text, "len", SYNTAX_BUILTIN);
    assert_color(&editor, python_text, "42", SYNTAX_NUMBER);
    assert_color(&editor, python_text, "+", SYNTAX_OPERATOR);
    assert_color(&editor, python_text, "\\n", SYNTAX_ESCAPE);

    const char* multiline = "/* start\nfirst\nsecond\nthird\nfourth\nfifth\nsixth\nseventh\neighth\nninth\nlast */\nint value;";
    set_text(&editor, "example.c", multiline);
    assert(resize_term(8, 60) == OK);
    editor.cursor_pos = (size_t)(strstr(multiline, "last") - multiline);
    redraw_window(&editor);
    assert(editor.top_line > 0);
    assert_color(&editor, multiline, "last", SYNTAX_COMMENT);
    assert(replace_buffer(editor.buf, 0, 2, "  ", 2));
    redraw_window(&editor);
    assert_color(&editor, multiline, "last", SYNTAX_VARIABLE);

    assert(resize_term(24, 100) == OK);
    set_text(&editor, "example.txt", python_text);
    assert_color(&editor, python_text, "def", SYNTAX_NORMAL);
    editor.syntax_mode = LANGUAGE_PYTHON + 1;
    redraw_window(&editor);
    assert_color(&editor, python_text, "def", SYNTAX_KEYWORD);
    initialize_syntax_colors(1);
    redraw_window(&editor);
    short foreground, background;
    assert(pair_content(SYNTAX_FUNCTION, &foreground, &background) == OK);
    assert(foreground == 25);
    assert_color(&editor, python_text, "label", SYNTAX_FUNCTION);
    initialize_syntax_colors(0);
    assert(pair_content(SYNTAX_FUNCTION, &foreground, &background) == OK);
    assert(foreground == 117);

    free_history(editor.history);
    free_buffer(editor.buf);
    endwin();
    delscreen(screen);
    fclose(input);
    fclose(output);
    puts("Syntax rendering tests passed");
    return 0;
}
