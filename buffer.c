#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffer.h"
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

Buffer* create_buffer(void) {
    Buffer* gapBuffer = (Buffer*)malloc(sizeof(Buffer));
    if (!gapBuffer) return NULL;
    
    gapBuffer->buffer = (char*)malloc(sizeof(char)*INITIAL_BUFFER_SIZE);
    if (!gapBuffer->buffer) {
        free(gapBuffer);
        return NULL;
    }
    
    memset(gapBuffer->buffer, '\0', INITIAL_BUFFER_SIZE);
    
    gapBuffer->buffer_size = INITIAL_BUFFER_SIZE;
    gapBuffer->gap_start = 0;
    gapBuffer->gap_end = INITIAL_BUFFER_SIZE;
    gapBuffer->text_size = 0;
    gapBuffer->first_character = 0;
    gapBuffer->last_character = 0;
    
    return gapBuffer;
}

void insert_buffer(Buffer* buf, char ch) {
    if (buf) {
        if (buf->gap_start == buf->gap_end) {
            resize_buffer(buf, buf->buffer_size + GAP_SIZE);
        }
        buf->buffer[buf->gap_start] = ch;
        buf->gap_start++;
        buf->text_size++;
    } else {
        perror("Error inserting character into buffer module");
    }
}

void delete_buffer(Buffer* buf) {
    if (buf) {
        if (buf->gap_start > 0) {
            buf->buffer[buf->gap_start] = '\0';
            buf->gap_start--;
            buf->text_size--;
        }
    } else {
        perror("Error deleting character from buffer module");
    }
}

void move_buffer_cursor(Buffer* buf, size_t position) {
    if (!buf || position > buf->text_size) {
        return;
    }

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


void resize_buffer(Buffer* buf, size_t new_size) {
    if (new_size <= buf->buffer_size) {
        return;
    }
    else if (new_size > buf->text_size) {

        size_t new_gap_end = buf->gap_start + GAP_SIZE;

        char* new_buffer = (char*) realloc(buf->buffer, new_size * sizeof(char));
        if (!new_buffer) {
            perror("Failed to reallocate memory");
        }
        
        memmove(new_buffer + new_gap_end, new_buffer + buf->gap_end, buf->buffer_size - buf->gap_end);

        buf->buffer = new_buffer;
        buf->gap_end = new_gap_end;
        buf->buffer_size = new_size;
    }
}

void free_buffer(Buffer* buf) {
    free(buf->buffer);
    free(buf);
}

void create_new_file(char filename[]) {
    FILE* file = fopen(filename, "wx");

    if (!file) {
        perror("Error creating file");
    } else {
        fclose(file);
    }
}

void load_file_into_buffer(char filename[], Buffer* buf, size_t screen_width) {
    if (!buf || !filename) {
        fprintf(stderr, "Error: Invalid buffer or filename\n");
        return;
    }

    FILE* file = fopen(filename, "r+");
    if (!file) {
        file = fopen(filename, "r");
        if (!file) {
            create_new_file(filename);
            file = fopen(filename, "r");
            if (!file) {
                perror("Error opening/creating file");
                return;
            }
        }
    }

    buf->text_size = 0;
    buf->gap_start = 0;
    buf->gap_end = INITIAL_BUFFER_SIZE - GAP_SIZE;
    buf->first_character = 0;
    buf->last_character = 0;

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    size_t needed_size = file_size + screen_width * 2;
    if (needed_size > buf->buffer_size) {
        resize_buffer(buf, needed_size);
    }

    int ch;
    int x_pos = 1;
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            size_t spaces_needed = screen_width - (x_pos - 1);
            for (size_t i = 0; i < spaces_needed; i++) {
                insert_buffer(buf, ' ');
            }
            x_pos = 1;
        } else {
            insert_buffer(buf, ch);
            x_pos++;
            
            if (x_pos >= (int)(screen_width - 1)) {
                x_pos = 1;
            }
        }
    }

    fclose(file);
}


void trim(char filename[]) {
    FILE* file = fopen(filename, "r+");

    char current_line[500];
    char* buffer = (char*)malloc(4096);
    size_t index = 0;

    while (fgets(current_line, sizeof(current_line), file)) {
        char* trimmed = current_line;
        while (isspace((unsigned char)*trimmed)) {
            trimmed++;
        }
        int len = strlen(trimmed);
        memcpy(buffer+index, trimmed, len);
        index += len;
    }

    buffer[index] = '\0';
    rewind(file);
    fclose(file);

    file = fopen(filename, "w");
    fputs(buffer, file);
    free(buffer);
    fclose(file);
}

void save_contents_to_file(char filename[], Buffer* buf, size_t screen_width) {
    if (!filename || !buf) {
        fprintf(stderr, "Error: Invalid filename or buffer\n");
        return;
    }
    
    FILE* file = fopen(filename, "r+");

    if (!file) {
        file = fopen(filename, "w");
        if (!file) {
            perror("Error opening/creating file");
            return;
        }
    }

    char* temp_buffer = (char*)malloc(buf->text_size + buf->text_size/screen_width + 1);
    if (!temp_buffer) {
        perror("Failed to allocate memory for saving");
        fclose(file);
        return;
    }
    
    size_t temp_index = 0;
    size_t line_pos = 0;
    int last_was_space = 0;
    
    for (size_t i = 0; i < buf->gap_start; i++) {
        char ch = buf->buffer[i];
        
        if (line_pos >= screen_width - 2) {
            temp_buffer[temp_index++] = '\n';
            line_pos = 0;
            last_was_space = 1;
            continue;
        }
        
        if (ch == ' ') {
            if (!last_was_space) {
                temp_buffer[temp_index++] = ch;
            }
            last_was_space = 1;
        } else {
            temp_buffer[temp_index++] = ch;
            last_was_space = 0;
        }
        
        line_pos++;
    }
    
    for (size_t i = buf->gap_end; i < buf->buffer_size && buf->buffer[i] != '\0'; i++) {
        char ch = buf->buffer[i];
        
        if (line_pos >= screen_width - 2) {
            temp_buffer[temp_index++] = '\n';
            line_pos = 0;
            last_was_space = 1;
            continue;
        }
        
        if (ch == ' ') {
            if (!last_was_space) {
                temp_buffer[temp_index++] = ch;
            }
            last_was_space = 1;
        } else {
            temp_buffer[temp_index++] = ch;
            last_was_space = 0;
        }
        
        line_pos++;
    }
    
    temp_buffer[temp_index] = '\0';
    
    rewind(file);
    fputs(temp_buffer, file);
    
    int fd = fileno(file);
    if (fd != -1) {
        ftruncate(fd, ftell(file));
    }
    
    free(temp_buffer);
    fclose(file);
}
