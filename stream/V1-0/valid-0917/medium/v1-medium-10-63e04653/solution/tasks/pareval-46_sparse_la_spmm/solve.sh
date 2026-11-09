#!/bin/bash
# Oracle solution for ParEval OpenMP task 46_sparse_la_spmm.
set -e
cat > /workspace/generated-code.hpp <<'EOF'
#include <omp.h>

struct COOElement {
   size_t row, column;
   double value;
};

void spmm(std::vector<COOElement> const& A, std::vector<COOElement> const& X, std::vector<double> &Y, size_t M, size_t K, size_t N) {
   Y.assign(M * N, 0);
   #pragma omp parallel for
   for (size_t i = 0; i < A.size(); i += 1) {
      const auto &a = A[i];
      for (const auto& x : X) {
         if (a.column == x.row) {
            #pragma omp atomic
            Y[a.row * N + x.column] += a.value * x.value;
         }
      }
   }
}
EOF
echo "oracle written to /workspace/generated-code.hpp"
