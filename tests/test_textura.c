#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "utils.h"

static void assert_text(Buffer* buf, const char* text, size_t length) {
    assert(buf->text_size == length);
    assert(buf->gap_end >= buf->gap_start);
    assert(buf->buffer_size - (buf->gap_end - buf->gap_start) == length);
    for (size_t i = 0; i < length; i++) assert(buffer_character(buf, i) == text[i]);
}

static void test_buffer(void) {
    Buffer* buf = create_buffer();
    assert(buf);
    char expected[16000];
    size_t length = 0;
    srand(17);
    for (size_t i = 0; i < 12000; i++) {
        size_t position = (size_t)rand() % (length + 1);
        if (i % 4 == 0 && position < length) {
            move_buffer_cursor(buf, position + 1);
            delete_buffer(buf);
            memmove(expected + position, expected + position + 1, length - position - 1);
            length--;
        } else {
            char ch = (char)('a' + rand() % 26);
            move_buffer_cursor(buf, position);
            assert(insert_buffer(buf, ch));
            memmove(expected + position + 1, expected + position, length - position);
            expected[position] = ch;
            length++;
        }
        assert_text(buf, expected, length);
    }
    free_buffer(buf);
}

static void test_files(void) {
    char filename[] = "/tmp/textura-test-XXXXXX";
    int fd = mkstemp(filename);
    assert(fd >= 0);
    assert(close(fd) == 0);
    Buffer* buf = create_buffer();
    Buffer* loaded = create_buffer();
    char text[8192];
    for (size_t i = 0; i < sizeof(text); i++) text[i] = (char)(i % 256);
    memcpy(text, "  indented\ttext  \n\n\r\n", 21);
    assert(replace_buffer(buf, 0, 0, text, sizeof(text)));
    move_buffer_cursor(buf, 3000);
    assert(save_contents_to_file(filename, buf));
    assert(load_file_into_buffer(filename, loaded));
    assert_text(loaded, text, sizeof(text));
    assert(replace_buffer(buf, 0, buf->text_size, "short\n", 6));
    assert(save_contents_to_file(filename, buf));
    assert(load_file_into_buffer(filename, loaded));
    assert_text(loaded, "short\n", 6);
    assert(replace_buffer(buf, 0, buf->text_size, "", 0));
    assert(save_contents_to_file(filename, buf));
    assert(load_file_into_buffer(filename, loaded));
    assert_text(loaded, "", 0);
    assert(!save_contents_to_file("/nonexistent-textura-directory/file", buf));
    assert(unlink(filename) == 0);
    assert(load_file_into_buffer(filename, loaded));
    assert(access(filename, F_OK) != 0);
    free_buffer(loaded);
    free_buffer(buf);
}

static void test_history(void) {
    Buffer* buf = create_buffer();
    History* history = create_history(2);
    size_t cursor_pos = 0;
    assert(record_edit(history, buf, 0, 0, "one", 3, &cursor_pos, 3));
    history->saved_revision = history->revision;
    assert(record_edit(history, buf, 3, 0, "two", 3, &cursor_pos, 6));
    assert(undo(history, buf, &cursor_pos));
    assert_text(buf, "one", 3);
    assert(cursor_pos == 3);
    assert(history->revision == history->saved_revision);
    assert(redo(history, buf, &cursor_pos));
    assert_text(buf, "onetwo", 6);
    assert(cursor_pos == 6);
    assert(undo(history, buf, &cursor_pos));
    assert(undo(history, buf, &cursor_pos));
    assert(!undo(history, buf, &cursor_pos));
    assert(record_edit(history, buf, 0, 0, "new", 3, &cursor_pos, 3));
    assert(!redo(history, buf, &cursor_pos));
    assert(history->revision != history->saved_revision);
    assert(history->count == 1);
    assert(record_edit(history, buf, 3, 0, "!", 1, &cursor_pos, 4));
    assert(record_edit(history, buf, 4, 0, "!", 1, &cursor_pos, 5));
    assert(history->count == 2);
    assert(undo(history, buf, &cursor_pos));
    assert(undo(history, buf, &cursor_pos));
    assert(!undo(history, buf, &cursor_pos));
    assert_text(buf, "new", 3);
    assert(record_edit(history, buf, 0, 3, "branch", 6, &cursor_pos, 6));
    assert(!redo(history, buf, &cursor_pos));
    assert_text(buf, "branch", 6);
    free_history(history);
    free_buffer(buf);
}

static void test_features(void) {
    Editor editor = {0};
    editor.buf = create_buffer();
    editor.history = create_history(100);
    Buffer* buf = editor.buf;
    assert(replace_buffer(buf, 0, 0, "  one\n\tone one\n", 15));
    move_buffer_cursor(buf, 9);
    assert(find_text(buf, "one", 0, 0) == 2);
    assert(find_text(buf, "one", 3, 0) == 7);
    assert(find_text(buf, "one", 12, 0) == 2);
    assert(find_text(buf, "one", 1, 1) == 11);
    assert(find_text(buf, "missing", 0, 0) == SIZE_MAX);
    assert(find_text(buf, "", 0, 0) == SIZE_MAX);
    assert(line_position(buf, 2) == 6);
    assert(line_start(buf, 10) == 6);
    assert(line_end(buf, 7) == 14);
    editor.cursor_pos = 5;
    editor.auto_indent = 1;
    assert(insert_newline(&editor));
    assert_text(buf, "  one\n  \n\tone one\n", 18);
    assert(editor.cursor_pos == 8);
    assert(undo(editor.history, buf, &editor.cursor_pos));
    assert(editor.cursor_pos == 5);
    editor.cursor_pos = 14;
    assert(insert_newline(&editor));
    assert_text(buf, "  one\n\tone one\n\t\n", 17);
    assert(undo(editor.history, buf, &editor.cursor_pos));
    size_t count;
    assert(replace_all(&editor, "one", "three", &count));
    assert(count == 3);
    assert_text(buf, "  three\n\tthree three\n", 21);
    assert(undo(editor.history, buf, &editor.cursor_pos));
    assert_text(buf, "  one\n\tone one\n", 15);
    assert(redo(editor.history, buf, &editor.cursor_pos));
    assert(replace_all(&editor, "three", "", &count));
    assert(count == 3);
    assert_text(buf, "  \n\t \n", 6);
    assert(replace_all(&editor, "missing", "x", &count));
    assert(count == 0);
    assert(replace_buffer(buf, 0, buf->text_size, "aaaaa", 5));
    assert(replace_all(&editor, "aa", "b", &count));
    assert(count == 2);
    assert_text(buf, "bba", 3);
    assert(replace_buffer(buf, 0, buf->text_size, "12345\n\tabc\nz", 12));
    editor.cursor_pos = 5;
    move_vertical(&editor, 1, 1);
    assert(editor.cursor_pos == 8);
    move_vertical(&editor, 1, 1);
    assert(editor.cursor_pos == 12);
    assert_text(buf, "12345\n\tabc\nz", 12);
    free_history(editor.history);
    free_buffer(buf);
}

int main(void) {
    test_buffer();
    test_files();
    test_history();
    test_features();
    puts("All Textura tests passed");
    return 0;
}
