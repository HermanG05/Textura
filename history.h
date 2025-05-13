#ifndef HISTORY_H
#define HISTORY_H

#include "buffer.h"

typedef enum {
    INSERT_CHAR,
    DELETE_CHAR,
    ENTER_LINE,
    BATCH_EDIT
} EditType;

typedef struct HistoryNode {
    EditType type;
    size_t position;
    char character;
    size_t screen_x;
    size_t screen_y;
    struct HistoryNode* next;
    struct HistoryNode* prev;
} HistoryNode;

typedef struct {
    HistoryNode* current;
    HistoryNode* head;
    HistoryNode* tail;
    int batch_mode;
    int max_history;
    int count;
} History;

History* create_history(int max_history);
void free_history(History* history);

void record_insert(History* history, size_t position, char character, size_t x, size_t y);
void record_delete(History* history, size_t position, char character, size_t x, size_t y);
void record_enter(History* history, size_t position, size_t x, size_t y);

void start_batch(History* history);
void end_batch(History* history);

int undo(History* history, Buffer* buf, size_t* x, size_t* y, size_t width);
int redo(History* history, Buffer* buf, size_t* x, size_t* y, size_t width);

#endif
