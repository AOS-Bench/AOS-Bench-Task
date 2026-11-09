#ifndef LIBCOMPUTE_LIST_H
#define LIBCOMPUTE_LIST_H

#include <stddef.h>
#include <stdint.h>

/* Growable list of int32_t. */
typedef struct {
    int32_t *data;
    size_t   size;
    size_t   cap;
} List;

/* Init an empty list (in-place, no allocation yet). */
void list_init(List *l);

/* Push v. Grows via doubling on overflow. Returns 0 on success, -1 on OOM. */
int list_push(List *l, int32_t v);

/* Pop last. Returns 0 and *out on success, -1 if empty. */
int list_pop(List *l, int32_t *out);

/* Return element at index. No bounds check. */
int32_t list_get(const List *l, size_t i);

/* Release the internal buffer. Safe to call on init'd-but-empty lists. */
void list_free(List *l);

/* Number of elements. */
size_t list_size(const List *l);

#endif
