#ifndef LIBCOMPUTE_MATRIX_H
#define LIBCOMPUTE_MATRIX_H

#include <stddef.h>

/* Row-major dense matrix of double. Owned data. */
typedef struct {
    size_t  rows;
    size_t  cols;
    double *data;   /* rows*cols entries, row-major */
} Matrix;

/* Allocate an r×c matrix, zero-initialized. NULL on OOM or 0 dim. */
Matrix *mat_new(size_t r, size_t c);

/* Free (safe on NULL). */
void mat_free(Matrix *m);

/* Get / set. No bounds checks. */
double mat_get(const Matrix *m, size_t i, size_t j);
void   mat_set(Matrix *m, size_t i, size_t j, double v);

/* Multiply a (r×k) by b (k×c) into c (r×c). Returns 0 on success, -1 if
 * dimensions mismatch or any arg is NULL. */
int mat_mul(const Matrix *a, const Matrix *b, Matrix *out);

/* Frobenius norm (sqrt of sum of squares). */
double mat_frobenius(const Matrix *m);

/* Elementwise addition c = a + b. Same dim required. */
int mat_add(const Matrix *a, const Matrix *b, Matrix *out);

#endif
