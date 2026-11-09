#ifndef LIBCOMPUTE_SORT_H
#define LIBCOMPUTE_SORT_H

#include <stddef.h>
#include <stdint.h>

/* In-place ascending quicksort of n int32_t values. */
void so_qsort(int32_t *arr, size_t n);

/* Merge sort (allocates a scratch buffer). Returns 0 on success, -1 on
 * allocation failure. */
int so_msort(int32_t *arr, size_t n);

/* Return 1 if arr[0..n) is non-decreasing, else 0. */
int so_is_sorted(const int32_t *arr, size_t n);

#endif
