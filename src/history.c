#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"
#include "buffer.h"
#include "utils.h"

History* create_history(int max_history) {
    History* history = (History*)malloc(sizeof(History));
    if (!history) {
        perror("Failed to allocate memory for history");
        return NULL;
    }
    
    history->current = NULL;
    history->head = NULL;
    history->tail = NULL;
    history->batch_mode = 0;
    history->max_history = max_history;
    history->count = 0;
    
    return history;
}

void free_history(History* history) {
    if (!history) return;
    
    HistoryNode* current = history->head;
    while (current) {
        HistoryNode* next = current->next;
        free(current);
        current = next;
    }
    
    free(history);
}

static void add_history_node(History* history, HistoryNode* node) {
    if (!history || !node) return;
    
    if (history->current && history->current != history->tail) {
        HistoryNode* after_current = history->current->next;
        history->current->next = NULL;
        
        HistoryNode* temp = after_current;
        while (temp) {
            HistoryNode* next = temp->next;
            free(temp);
            temp = next;
            history->count--;
        }
        
        history->tail = history->current;
    }
    
    if (!history->head) {
        history->head = node;
        history->tail = node;
        node->prev = NULL;
        node->next = NULL;
    } else {
        node->prev = history->tail;
        node->next = NULL;
        history->tail->next = node;
        history->tail = node;
    }
    
    history->current = node;
    history->count++;
    
    if (history->max_history > 0 && history->count > history->max_history) {
        HistoryNode* old_head = history->head;
        history->head = old_head->next;
        if (history->head) {
            history->head->prev = NULL;
        }
        free(old_head);
        history->count--;
    }
}

void record_insert(History* history, size_t position, char character, size_t x, size_t y) {
    if (!history || history->batch_mode) return;
    
    HistoryNode* node = (HistoryNode*)malloc(sizeof(HistoryNode));
    if (!node) {
        perror("Failed to allocate memory for history node");
        return;
    }
    
    node->type = INSERT_CHAR;
    node->position = position;
    node->character = character;
    node->screen_x = x;
    node->screen_y = y;
    
    add_history_node(history, node);
}

void record_delete(History* history, size_t position, char character, size_t x, size_t y) {
    if (!history || history->batch_mode) return;
    
    HistoryNode* node = (HistoryNode*)malloc(sizeof(HistoryNode));
    if (!node) {
        perror("Failed to allocate memory for history node");
        return;
    }
    
    node->type = DELETE_CHAR;
    node->position = position;
    node->character = character;
    node->screen_x = x;
    node->screen_y = y;
    
    add_history_node(history, node);
}

void record_enter(History* history, size_t position, size_t x, size_t y) {
    if (!history) return;
    
    HistoryNode* node = (HistoryNode*)malloc(sizeof(HistoryNode));
    if (!node) {
        perror("Failed to allocate memory for history node");
        return;
    }
    
    node->type = ENTER_LINE;
    node->position = position;
    node->character = '\n';
    node->screen_x = x;
    node->screen_y = y;
    
    add_history_node(history, node);
}

void start_batch(History* history) {
    if (!history) return;
    history->batch_mode = 1;
}

void end_batch(History* history) {
    if (!history) return;
    history->batch_mode = 0;
}

int undo(History* history, Buffer* buf, size_t* x, size_t* y, size_t width) {
    if (!history || !history->current) return 0;
    
    HistoryNode* node = history->current;
    
    switch (node->type) {
        case INSERT_CHAR:
            move_buffer_cursor(buf, node->position + 1);
            delete_buffer(buf);
            break;
            
        case DELETE_CHAR:
            move_buffer_cursor(buf, node->position);
            insert_buffer(buf, node->character);
            break;
            
        case ENTER_LINE:
            move_buffer_cursor(buf, node->position);
            for (size_t i = 0; i < width - 2; i++) {
                delete_buffer(buf);
            }
            break;
            
        case BATCH_EDIT:
            break;
    }
    
    history->current = node->prev;
    
    *x = node->screen_x;
    *y = node->screen_y;
    
    return 1;
}

int redo(History* history, Buffer* buf, size_t* x, size_t* y, size_t width) {
    if (!history) return 0;
    
    if (history->current == NULL && history->head != NULL) {
        history->current = history->head;
    } 
    else if (!history->current || !history->current->next) {
        return 0;
    }
    else {
        history->current = history->current->next;
    }
    
    HistoryNode* node = history->current;
    
    switch (node->type) {
        case INSERT_CHAR:
            move_buffer_cursor(buf, node->position);
            insert_buffer(buf, node->character);
            break;
            
        case DELETE_CHAR:
            move_buffer_cursor(buf, node->position + 1);
            delete_buffer(buf);
            break;
            
        case ENTER_LINE:
            move_buffer_cursor(buf, node->position);
            for (size_t i = 0; i < width - 2; i++) {
                insert_buffer(buf, ' ');
            }
            break;
            
        case BATCH_EDIT:
            break;
    }
    
    *x = node->screen_x;
    *y = node->screen_y;
    
    return 1;
}
