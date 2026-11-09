/* list.c — growable int32_t list */
#include "list.h"
#include <stdlib.h>
#include <string.h>

void list_init(List *l) {
    l->data = NULL;
    l->size = 0;
    l->cap = 0;
}

int list_push(List *l, int32_t v) {
    if (!l) return -1;
    if (l->size >= l->cap) {
        size_t new_cap = (l->cap == 0) ? 8 : l->cap * 2;
        int32_t *nd = (int32_t *)realloc(l->data, new_cap * sizeof(int32_t));
        if (!nd) return -1;
        l->data = nd;
        l->cap = new_cap;
    }
    l->data[l->size++] = v;
    return 0;
}

int list_pop(List *l, int32_t *out) {
    if (!l || l->size == 0) return -1;
    l->size--;
    *out = l->data[l->size - 1];
    return 0;
}

int32_t list_get(const List *l, size_t i) {
    return l->data[i];
}

void list_free(List *l) {
    if (!l) return;
    free(l->data);
    l->data = NULL;
    l->size = 0;
    l->cap = 0;
}

size_t list_size(const List *l) {
    return l ? l->size : 0;
}
