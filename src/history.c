#include <stdlib.h>
#include <string.h>
#include "history.h"

History* create_history(int max_history) {
    History* history = (History*)calloc(1, sizeof(History));
    if (history) history->max_history = max_history;
    return history;
}

static void free_history_node(HistoryNode* node) {
    free(node->before);
    free(node->after);
    free(node);
}

void free_history(History* history) {
    if (!history) return;
    HistoryNode* node = history->head;
    while (node) {
        HistoryNode* next = node->next;
        free_history_node(node);
        node = next;
    }
    free(history);
}

int record_edit(History* history, Buffer* buf, size_t position, size_t length,
                const char* text, size_t text_size, size_t* cursor_pos, size_t cursor_after) {
    if (position > buf->text_size || length > buf->text_size - position) return 0;
    HistoryNode* node = (HistoryNode*)calloc(1, sizeof(HistoryNode));
    if (!node) return 0;
    node->before = (char*)malloc(length ? length : 1);
    node->after = (char*)malloc(text_size ? text_size : 1);
    if (!node->before || !node->after) {
        free_history_node(node);
        return 0;
    }
    for (size_t i = 0; i < length; i++) node->before[i] = buffer_character(buf, position + i);
    if (text_size) memcpy(node->after, text, text_size);
    if (!replace_buffer(buf, position, length, node->after, text_size)) {
        free_history_node(node);
        return 0;
    }
    node->position = position;
    node->before_size = length;
    node->after_size = text_size;
    node->cursor_before = *cursor_pos;
    node->cursor_after = cursor_after;
    node->revision_before = history->revision;
    node->revision_after = ++history->next_revision;

    HistoryNode* discarded = history->current ? history->current->next : history->head;
    while (discarded) {
        HistoryNode* next = discarded->next;
        free_history_node(discarded);
        discarded = next;
        history->count--;
    }
    node->prev = history->current;
    if (history->current) history->current->next = node;
    else history->head = node;
    history->current = node;
    history->tail = node;
    history->revision = node->revision_after;
    history->count++;
    if (history->max_history > 0 && history->count > history->max_history) {
        HistoryNode* old_head = history->head;
        history->head = old_head->next;
        history->head->prev = NULL;
        free_history_node(old_head);
        history->count--;
    }
    *cursor_pos = cursor_after;
    return 1;
}

int undo(History* history, Buffer* buf, size_t* cursor_pos) {
    HistoryNode* node = history->current;
    if (!node) return 0;
    if (!replace_buffer(buf, node->position, node->after_size, node->before, node->before_size)) return 0;
    *cursor_pos = node->cursor_before;
    history->revision = node->revision_before;
    history->current = node->prev;
    return 1;
}

int redo(History* history, Buffer* buf, size_t* cursor_pos) {
    HistoryNode* node = history->current ? history->current->next : history->head;
    if (!node) return 0;
    if (!replace_buffer(buf, node->position, node->before_size, node->after, node->after_size)) return 0;
    *cursor_pos = node->cursor_after;
    history->revision = node->revision_after;
    history->current = node;
    return 1;
}
