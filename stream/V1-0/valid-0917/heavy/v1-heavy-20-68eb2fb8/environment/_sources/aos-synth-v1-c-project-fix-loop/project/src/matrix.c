/* matrix.c — dense double-precision matrix ops */
#include "matrix.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

Matrix *mat_new(size_t r, size_t c) {
    if (r == 0 || c == 0) return NULL;
    Matrix *m = (Matrix *)malloc(sizeof(Matrix));
    if (!m) return NULL;
    m->data = (double *)calloc(r * c, sizeof(double));
    if (!m->data) { free(m); return NULL; }
    m->rows = r;
    m->cols = c;
    return m;
}

void mat_free(Matrix *m) {
    if (!m) return;
    free(m->data);
    free(m);
}

double mat_get(const Matrix *m, size_t i, size_t j) {
    return m->data[i * m->cols + j];
}

void mat_set(Matrix *m, size_t i, size_t j, double v) {
    m->data[i * m->cols + j] = v;
}

int mat_mul(const Matrix *a, const Matrix *b, Matrix *out) {
    if (!a || !b || !out) return -1;
    if (a->cols != b->rows) return -1;
    if (out->rows != a->rows || out->cols != b->cols) return -1;
    for (size_t i = 0; i < a->rows; i++) {
        for (size_t j = 0; j < b->cols; j++) {
            double s = 0.0;
            for (size_t k = 0; k < a->cols; k++) {
                s += a->data[i * a->cols + k] * b->data[k * b->rows + j];
            }
            out->data[i * out->cols + j] = s;
        }
    }
    return 0;
}

double mat_frobenius(const Matrix *m) {
    double s = 0.0;
    size_t n = m->rows * m->cols;
    for (size_t i = 0; i < n; i++) s += m->data[i] * m->data[i];
    return sqrt(s);
}

int mat_add(const Matrix *a, const Matrix *b, Matrix *out) {
    if (!a || !b || !out) return -1;
    if (a->rows != b->rows || a->cols != b->cols) return -1;
    if (out->rows != a->rows || out->cols != a->cols) return -1;
    size_t n = a->rows * a->cols;
    for (size_t i = 0; i < n; i++) {
        out->data[i] = a->data[i] + b->data[i];
    }
    return 0;
}
