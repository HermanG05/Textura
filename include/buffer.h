#include <stdio.h>
#include <stdlib.h>

#pragma once

#define INITIAL_BUFFER_SIZE 1024  
#define GAP_SIZE 5

typedef struct {
    char* buffer;
    size_t buffer_size;
    size_t gap_start;
    size_t gap_end;
    size_t text_size;
    size_t last_character;
    size_t first_character;

} Buffer; 

Buffer* create_buffer(void);
void insert_buffer(Buffer* buf, char ch);
void delete_buffer(Buffer* buf);
void move_buffer_cursor(Buffer* buf, size_t position);
void resize_buffer(Buffer* buf, size_t new_size);
void free_buffer(Buffer* buf);
void create_new_file(char filename[]);
void load_file_into_buffer(char filename[], Buffer* buf, size_t screen_width);
void trim(char filename[]);
void save_contents_to_file(char filename[], Buffer* buf, size_t screen_width);
