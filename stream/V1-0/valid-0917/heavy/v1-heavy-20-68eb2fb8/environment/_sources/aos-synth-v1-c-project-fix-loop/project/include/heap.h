#ifndef LIBCOMPUTE_HEAP_H
#define LIBCOMPUTE_HEAP_H

#include <stddef.h>
#include <stdint.h>

/* Fixed-size max-heap of int32_t. Owned buffer, caller frees with heap_free. */
typedef struct {
    int32_t *data;
    size_t   size;
    size_t   cap;
} MaxHeap;

/* Allocate a heap with the given capacity. Returns NULL on OOM or cap==0. */
MaxHeap *heap_new(size_t cap);

/* Free the heap (safe on NULL). */
void heap_free(MaxHeap *h);

/* Push v. Returns 0 on success, -1 if heap is at capacity. */
int heap_push(MaxHeap *h, int32_t v);

/* Pop the maximum. Returns 0 and writes to *out on success, -1 if empty. */
int heap_pop(MaxHeap *h, int32_t *out);

/* Peek at the maximum without removing. Returns 0/-1 like heap_pop. */
int heap_peek(const MaxHeap *h, int32_t *out);

/* Number of elements. */
size_t heap_size(const MaxHeap *h);

#endif
