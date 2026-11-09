/* heap.c — array-backed max-heap of int32_t */
#include "heap.h"
#include <stdlib.h>

MaxHeap *heap_new(size_t cap) {
    if (cap == 0) return NULL;
    MaxHeap *h = (MaxHeap *)malloc(sizeof(MaxHeap));
    if (!h) return NULL;
    h->data = (int32_t *)malloc(cap * sizeof(int32_t));
    if (!h->data) { free(h); return NULL; }
    h->size = 0;
    h->cap = cap;
    return h;
}

void heap_free(MaxHeap *h) {
    if (!h) return;
    free(h->data);
    free(h);
}

static void sift_up(int32_t *a, size_t i) {
    while (i > 0) {
        size_t p = i / 2;
        if (a[p] >= a[i]) break;
        int32_t t = a[p]; a[p] = a[i]; a[i] = t;
        i = p;
    }
}

static void sift_down(int32_t *a, size_t n, size_t i) {
    for (;;) {
        size_t l = 2 * i + 1;
        size_t r = 2 * i + 2;
        size_t best = i;
        if (l < n && a[l] > a[best]) best = l;
        if (r < n && a[r] > a[best]) best = r;
        if (best == i) break;
        int32_t t = a[i]; a[i] = a[best]; a[best] = t;
        i = best;
    }
}

int heap_push(MaxHeap *h, int32_t v) {
    if (!h || h->size >= h->cap) return -1;
    h->data[h->size] = v;
    sift_up(h->data, h->size);
    h->size++;
    return 0;
}

int heap_pop(MaxHeap *h, int32_t *out) {
    if (!h || h->size == 0) return -1;
    *out = h->data[0];
    h->size--;
    if (h->size > 0) {
        h->data[0] = h->data[h->size];
        sift_down(h->data, h->size, 0);
    }
    return 0;
}

int heap_peek(const MaxHeap *h, int32_t *out) {
    if (!h || h->size == 0) return -1;
    *out = h->data[0];
    return 0;
}

size_t heap_size(const MaxHeap *h) {
    return h ? h->size : 0;
}
