#ifndef HISTORY_H
#define HISTORY_H

#include "buffer.h"

typedef struct HistoryNode {
    size_t position;
    char* before;
    char* after;
    size_t before_size;
    size_t after_size;
    size_t cursor_before;
    size_t cursor_after;
    size_t revision_before;
    size_t revision_after;
    struct HistoryNode* next;
    struct HistoryNode* prev;
} HistoryNode;

typedef struct {
    HistoryNode* current;
    HistoryNode* head;
    HistoryNode* tail;
    int max_history;
    int count;
    size_t revision;
    size_t next_revision;
    size_t saved_revision;
} History;

History* create_history(int max_history);
void free_history(History* history);
int record_edit(History* history, Buffer* buf, size_t position, size_t length,
                const char* text, size_t text_size, size_t* cursor_pos, size_t cursor_after);
int undo(History* history, Buffer* buf, size_t* cursor_pos);
int redo(History* history, Buffer* buf, size_t* cursor_pos);

#endif
