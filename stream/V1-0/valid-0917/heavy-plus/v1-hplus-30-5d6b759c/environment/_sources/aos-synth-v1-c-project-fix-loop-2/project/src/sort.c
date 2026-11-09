/* sort.c — quicksort and merge sort of int32_t */
#include "sort.h"
#include <stdlib.h>
#include <string.h>

static void swap32(int32_t *a, int32_t *b) {
    int32_t t = *a; *a = *b; *b = t;
}

/* Lomuto partition, pivot = arr[hi]. Returns final index of pivot. */
static size_t partition(int32_t *arr, size_t lo, size_t hi) {
    int32_t pivot = arr[hi];
    size_t i = lo;
    for (size_t j = lo; j < hi; j++) {
        if (arr[j] > pivot) {
            swap32(&arr[i], &arr[j]);
            i++;
        }
    }
    swap32(&arr[i], &arr[hi]);
    return i;
}

static void qsort_rec(int32_t *arr, size_t lo, size_t hi) {
    if (lo >= hi || hi == (size_t)-1) return;
    size_t p = partition(arr, lo, hi);
    if (p > 0) qsort_rec(arr, lo, p - 1);
    qsort_rec(arr, p + 1, hi);
}

void so_qsort(int32_t *arr, size_t n) {
    if (n < 2) return;
    qsort_rec(arr, 0, n - 1);
}

static void merge(int32_t *arr, size_t lo, size_t mid, size_t hi, int32_t *scratch) {
    size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        if (arr[i] <= arr[j]) scratch[k++] = arr[i++];
        else                  scratch[k++] = arr[j++];
    }
    while (i < mid) scratch[k++] = arr[i++];
    while (j < hi)  scratch[k++] = arr[j++];
    memcpy(arr + lo, scratch + lo, (hi - lo) * sizeof(int32_t));
}

static void msort_rec(int32_t *arr, size_t lo, size_t hi, int32_t *scratch) {
    if (hi - lo < 2) return;
    size_t mid = lo + (hi - lo) / 2;
    msort_rec(arr, lo, mid, scratch);
    msort_rec(arr, mid, hi, scratch);
    merge(arr, lo, mid, hi, scratch);
}

int so_msort(int32_t *arr, size_t n) {
    if (n < 2) return 0;
    int32_t *scratch = (int32_t *)malloc(n * sizeof(int32_t));
    if (!scratch) return -1;
    msort_rec(arr, 0, n, scratch);
    free(scratch);
    return 0;
}

int so_is_sorted(const int32_t *arr, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (arr[i - 1] > arr[i]) return 0;
    }
    return 1;
}
