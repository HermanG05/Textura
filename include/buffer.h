#pragma once

#include <stdio.h>
#include <stdlib.h>

#define INITIAL_BUFFER_SIZE 1024

typedef struct {
    char* buffer;
    size_t buffer_size;
    size_t gap_start;
    size_t gap_end;
    size_t text_size;
} Buffer;

Buffer* create_buffer(void);
int insert_buffer(Buffer* buf, char ch);
void delete_buffer(Buffer* buf);
void move_buffer_cursor(Buffer* buf, size_t position);
int resize_buffer(Buffer* buf, size_t new_size);
void free_buffer(Buffer* buf);
char buffer_character(const Buffer* buf, size_t position);
int replace_buffer(Buffer* buf, size_t position, size_t length, const char* text, size_t text_size);
int load_file_into_buffer(const char* filename, Buffer* buf);
int save_contents_to_file(const char* filename, const Buffer* buf);
