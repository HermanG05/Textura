#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include "buffer.h"

Buffer* create_buffer(void) {
    Buffer* buf = (Buffer*)calloc(1, sizeof(Buffer));
    if (!buf) return NULL;

    buf->buffer = (char*)malloc(INITIAL_BUFFER_SIZE);
    if (!buf->buffer) {
        free(buf);
        return NULL;
    }
    buf->buffer_size = INITIAL_BUFFER_SIZE;
    buf->gap_end = INITIAL_BUFFER_SIZE;
    return buf;
}

int resize_buffer(Buffer* buf, size_t new_size) {
    if (new_size <= buf->buffer_size) return 1;

    char* new_buffer = (char*)realloc(buf->buffer, new_size);
    if (!new_buffer) return 0;

    size_t new_gap_end = buf->gap_end + new_size - buf->buffer_size;
    memmove(new_buffer + new_gap_end, new_buffer + buf->gap_end,
            buf->buffer_size - buf->gap_end);
    buf->buffer = new_buffer;
    buf->gap_end = new_gap_end;
    buf->buffer_size = new_size;
    return 1;
}

void move_buffer_cursor(Buffer* buf, size_t position) {
    if (!buf || position > buf->text_size) return;

    if (position < buf->gap_start) {
        size_t move_size = buf->gap_start - position;
        memmove(buf->buffer + buf->gap_end - move_size, buf->buffer + position, move_size);
        buf->gap_start = position;
        buf->gap_end -= move_size;
    } else if (position > buf->gap_start) {
        size_t move_size = position - buf->gap_start;
        memmove(buf->buffer + buf->gap_start, buf->buffer + buf->gap_end, move_size);
        buf->gap_start = position;
        buf->gap_end += move_size;
    }
}

char buffer_character(const Buffer* buf, size_t position) {
    if (position >= buf->text_size) return '\0';
    return buf->buffer[position < buf->gap_start ? position : position + buf->gap_end - buf->gap_start];
}

int replace_buffer(Buffer* buf, size_t position, size_t length, const char* text, size_t text_size) {
    if (!buf || position > buf->text_size || length > buf->text_size - position) return 0;
    if (text_size > SIZE_MAX - (buf->text_size - length)) return 0;

    size_t needed_size = buf->text_size - length + text_size;
    if (needed_size > buf->buffer_size) {
        size_t new_size = needed_size <= SIZE_MAX / 2 ? needed_size * 2 : needed_size;
        if (!resize_buffer(buf, new_size)) return 0;
    }
    move_buffer_cursor(buf, position);
    buf->gap_end += length;
    if (text_size) memcpy(buf->buffer + buf->gap_start, text, text_size);
    buf->gap_start += text_size;
    buf->text_size = needed_size;
    return 1;
}

int insert_buffer(Buffer* buf, char ch) {
    return replace_buffer(buf, buf->gap_start, 0, &ch, 1);
}

void delete_buffer(Buffer* buf) {
    if (buf && buf->gap_start > 0) {
        buf->gap_start--;
        buf->text_size--;
    }
}

void free_buffer(Buffer* buf) {
    if (!buf) return;
    free(buf->buffer);
    free(buf);
}

int load_file_into_buffer(const char* filename, Buffer* buf) {
    FILE* file = fopen(filename, "rb");
    if (!file) return errno == ENOENT;

    Buffer* loaded = create_buffer();
    if (!loaded) {
        fclose(file);
        return 0;
    }
    char block[4096];
    size_t length;
    int success = 1;
    while ((length = fread(block, 1, sizeof(block), file)) > 0) {
        if (!replace_buffer(loaded, loaded->text_size, 0, block, length)) {
            success = 0;
            break;
        }
    }
    if (ferror(file)) success = 0;
    if (fclose(file) != 0) success = 0;
    if (success) {
        Buffer previous = *buf;
        *buf = *loaded;
        *loaded = previous;
        move_buffer_cursor(buf, 0);
    }
    free_buffer(loaded);
    return success;
}

int save_contents_to_file(const char* filename, const Buffer* buf) {
    FILE* file = fopen(filename, "wb");
    if (!file) return 0;

    size_t tail_size = buf->text_size - buf->gap_start;
    int success = fwrite(buf->buffer, 1, buf->gap_start, file) == buf->gap_start;
    if (fwrite(buf->buffer + buf->gap_end, 1, tail_size, file) != tail_size) success = 0;
    if (fclose(file) != 0) success = 0;
    return success;
}
