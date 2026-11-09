# Parallelize with OpenMP: sparse COO matrix-matrix multiply (SpMM)

Write a C++ OpenMP implementation of the function below and save it to
`/workspace/generated-code__pareval-46_sparse_la_spmm-4.hpp` (a stub with the exact signature is already
there). The file must contain a complete, compilable definition of the
function — keep the signature exactly as given.

The `COOElement` struct shown in the snippet is NOT defined anywhere else —
your header must define it exactly as shown, before the function.

```cpp
#include <omp.h>

struct COOElement {
   size_t row, column;
   double value;
};

/* Compute the matrix multiplication Y=AX. A is a sparse MxK matrix in COO format.
   X is a sparse KxN matrix in COO format. Y is a dense MxN matrix in row-major.
   Use OpenMP to compute in parallel.
   Example:

   input: A=[{0,0,-2}, {0,1,1}, {1,1,-1}] X=[{0,1,2}, {1,0,-1}]
   output: Y=[{-1,-4}, {1,0}]
*/
void spmm(std::vector<COOElement> const& A, std::vector<COOElement> const& X, std::vector<double> &Y, size_t M, size_t K, size_t N) {
    // ... your OpenMP implementation ...
}
```

> Note: different COO elements may update the same output cell, so a parallel version must avoid write conflicts (e.g. atomics or row partitioning).

## How your code will be graded

The verifier compiles your header with:

```
g++ -std=c++17 -O3 -fopenmp -DUSE_OMP <hidden driver> -I/workspace -o a.out
```

against a **hidden benchmark driver** (you cannot see it, and network access is
unavailable). The driver `#include`s standard headers (`<vector>`, `<complex>`,
`<cmath>`, ...) before your file, then:

1. **Correctness**: runs your function on random inputs and compares the
   results against a trusted serial reference implementation. Any mismatch
   fails the task.
2. **Performance**: times your function with **4 OpenMP threads** (on 4 CPUs)
   and times the serial reference on the same data. Your implementation must
   achieve a **parallel speedup of at least 1.5x** over the serial baseline.

A correct but serial (or effectively serial) implementation WILL FAIL the
speedup gate. Reward is binary: both checks must pass.

Notes:
- Your file may add any standard `#include`s it needs.
- Do not spawn processes, read files, or print from inside the function.
- The problem size is fixed by the driver; do not special-case input sizes.
