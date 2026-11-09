#pragma once
#include <cassert>
#include <cmath>
#include <climits>
#include <cfloat>
#include <string>
#include <complex>
#include <queue>
#include <type_traits>

// make sure some parallel model is defined
#if !defined(USE_SERIAL) && !defined(USE_OMP) && !defined(USE_MPI) && !defined(USE_MPI_OMP) && !defined(USE_KOKKOS) && !defined(USE_CUDA) && !defined(USE_HIP)
#error "No parallel model not defined"
#endif

#define NO_OPTIMIZE __attribute__((optimize("O0")))
#define NO_INLINE __attribute__((noinline)) __attribute__((optimize("O3")))

#if !defined(DRIVER_PROBLEM_SIZE)
#error "DRIVER_PROBLEM_SIZE not defined"
#endif

#if !defined(MAX_VALIDATION_ATTEMPTS)
#define MAX_VALIDATION_ATTEMPTS 2
#endif

#if !defined(SPARSE_LA_SPARSITY)
// sparsity to use for sparse linear algebra benchmarks
#define SPARSE_LA_SPARSITY 0.1
#endif

// include the necessary libraries for the parallel model
#if defined(USE_OMP) || defined(USE_MPI_OMP)
#include <omp.h>
#elif defined(USE_MPI) || defined(USE_MPI_OMP)
#include <mpi.h>
#elif defined(USE_KOKKOS)
#include <Kokkos_Core.hpp>
#elif defined(USE_CUDA)
#include <cuda_runtime.h>
#elif defined(USE_HIP)
#include <hip/hip_runtime.h>
#endif

// some helper macros to unify CUDA and HIP interfaces
#if defined(USE_CUDA)
#define GRID_STRIDE_LOOP(i, n) for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < (n); i += blockDim.x * gridDim.x)
#define ALLOC(ptr, size) cudaMalloc(&(ptr), (size))
#define COPY_H2D(dst, src, size) cudaMemcpy((dst), (src), (size), cudaMemcpyHostToDevice)
#define COPY_D2H(dst, src, size) cudaMemcpy((dst), (src), (size), cudaMemcpyDeviceToHost)
#define FREE(ptr) cudaFree((ptr))
#define SYNC() cudaDeviceSynchronize()
#define DOUBLE_COMPLEX_T cuDoubleComplex
#define MAKE_DOUBLE_COMPLEX(r,i) make_cuDoubleComplex((r),(i))
#elif defined(USE_HIP)
#define GRID_STRIDE_LOOP(i, n) for (int i = hipBlockIdx_x * hipBlockDim_x + hipThreadIdx_x; i < (n); i += hipBlockDim_x * hipGridDim_x)
#define ALLOC(ptr, size) hipMalloc(&(ptr), (size))
#define COPY_H2D(dst, src, size) hipMemcpy((dst), (src), (size), hipMemcpyHostToDevice)
#define COPY_D2H(dst, src, size) hipMemcpy((dst), (src), (size), hipMemcpyDeviceToHost)
#define FREE(ptr) hipFree((ptr))
#define SYNC() hipDeviceSynchronize()
#define DOUBLE_COMPLEX_T hipDoubleComplex
#define MAKE_DOUBLE_COMPLEX(r,i) make_hipDoubleComplex((r),(i))
#endif

#if defined(USE_CUDA) || defined(USE_HIP)
__device__ double atomicMul(double* address, double val) { 
  unsigned long long int* address_as_ull = (unsigned long long int*)address; 
  unsigned long long int old = *address_as_ull, assumed; 
  do { 
    assumed = old; 
    old = atomicCAS(address_as_ull, assumed, __double_as_longlong(val * __longlong_as_double(assumed))); 
  } while (assumed != old); return __longlong_as_double(old);
} 
#endif

// Kokkos utilities
#if defined(USE_KOKKOS)
template <typename DType>
void copyVectorToView(std::vector<DType> const& vec, Kokkos::View<DType*> view) {
    assert(vec.size() == view.size());
    for (int i = 0; i < vec.size(); i += 1) {
        view(i) = vec[i];
    }
}

template <typename DType>
void copyViewToVector(Kokkos::View<DType*> view, std::vector<DType>& vec) {
    assert(vec.size() == view.size());
    for (int i = 0; i < vec.size(); i += 1) {
        vec[i] = view(i);
    }
}

template <typename DType>
void fillRandKokkos(Kokkos::View<DType*> &x, DType min, DType max) {
    for (int i = 0; i < x.size(); i += 1) {
        DType val;
        if constexpr (std::is_floating_point_v<DType>) {
            val = (rand() / (double) RAND_MAX) * (max - min) + min;
        } else if constexpr (std::is_integral_v<DType>) {
            val = rand() % (max - min) + min;
        }
        x(i) = val;
    }
}
#endif


// MPI utilities
#if defined(USE_MPI) || defined(USE_MPI_OMP)
#define IS_ROOT(rank) ((rank) == 0)
#define BCAST(vec,dtype) MPI_Bcast((vec).data(), (vec).size(), MPI_##dtype, 0, MPI_COMM_WORLD)
#define BCAST_PTR(ptr,size,dtype) MPI_Bcast(ptr, size, MPI_##dtype, 0, MPI_COMM_WORLD)
#define SYNC() MPI_Barrier(MPI_COMM_WORLD)
#define GET_RANK(rank) MPI_Comm_rank(MPI_COMM_WORLD, &(rank))
#else
#define IS_ROOT(rank) true
#define BCAST(vec,dtype)
#define BCAST_PTR(ptr,size,dtype)
#define GET_RANK(rank) rank = 0
#if !defined(SYNC)
#define SYNC()
#endif
#endif


template <typename T>
void fillRandString(T &x, size_t minLen, size_t maxLen) {
    for (int i = 0; i < x.size(); i += 1) {
        size_t len = rand() % (maxLen - minLen) + minLen;
        std::string str(len, ' ');
        for (int j = 0; j < len; j += 1) {
            str[j] = 'a' + rand() % 26;
        }
        x[i] = str;
    }
}

// utility functions
template <typename T, typename DType>
void fillRand(T &x, DType min, DType max) {
    
    for (int i = 0; i < x.size(); i += 1) {
        DType val;
        if constexpr (std::is_floating_point_v<DType>) {
            val = (rand() / (double) RAND_MAX) * (max - min) + min;
        } else if constexpr (std::is_integral_v<DType>) {
            val = rand() % (max - min) + min;
        } else if constexpr (std::is_same_v<DType, std::complex<double>>) {
            const double real = (rand() / (double) RAND_MAX) * (max - min) + min;
            const double imag = (rand() / (double) RAND_MAX) * (max - min) + min;
            val = std::complex<double>(real, imag);
        }
        x[i] = val;
    }
}

// compare two vectors of floating point numbers
template <typename Vec, typename FType>
bool fequal(Vec const& a, Vec const& b, FType epsilon = 1e-6) {
    assert(a.size() == b.size());
    for (int i = 0; i < a.size(); i += 1) {
        if (std::abs(a[i] - b[i]) > epsilon) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// AOS-Bench verifier helpers (trusted; used only by cpu.cc / omp-driver.cc,
// never visible to the agent). Two groups:
//
// 1. Deterministic private RNG. The baseline and agent builds must generate
//    bit-identical input sequences. libc rand() is deliberately NOT used, so
//    stray rand()/srand() calls inside generated code cannot desynchronize
//    the two builds.
//
// 2. Dump-record writers. Each timed iteration's output is appended to a dump
//    file; compare.cc checks the baseline and agent streams record-by-record.
//    Record format: <1-byte type char> <uint64 count> <payload>
//      'd' -> count doubles (compared with atol/rtol)
//      'i' -> count int32   (compared exactly)
//      'q' -> count uint64  (compared exactly; whole-array digests)
//    Writers are deliberately serial and thread-count independent so the
//    trusted overhead is identical in the agent and baseline builds.
// ---------------------------------------------------------------------------
#include <cstdio>
#include <cstdint>

inline uint64_t aosSplitmix64(uint64_t x) {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

// --- private deterministic RNG -------------------------------------------
inline uint64_t aosRngNext(uint64_t &state) {
    state += 0x9E3779B97F4A7C15ull;
    uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

inline double aosRngUniform(uint64_t &state, double lo, double hi) {
    const double u = (aosRngNext(state) >> 11) * (1.0 / 9007199254740992.0); // [0,1)
    return lo + (hi - lo) * u;
}

inline void aosFillUniform(uint64_t &state, double *p, size_t n, double lo, double hi) {
    for (size_t i = 0; i < n; i += 1) {
        p[i] = aosRngUniform(state, lo, hi);
    }
}

inline void aosFillInt(uint64_t &state, int *p, size_t n, int lo, int hi) { // [lo, hi)
    for (size_t i = 0; i < n; i += 1) {
        p[i] = lo + (int)(aosRngNext(state) % (uint64_t)(hi - lo));
    }
}

inline void aosFillIndex(uint64_t &state, size_t *p, size_t n, size_t bound) { // [0, bound)
    for (size_t i = 0; i < n; i += 1) {
        p[i] = (size_t)(aosRngNext(state) % (uint64_t)bound);
    }
}

// --- dump-record writers --------------------------------------------------
inline void aosDumpRecHeader(FILE *f, char type, uint64_t n) {
    std::fputc(type, f);
    std::fwrite(&n, sizeof(n), 1, f);
}

inline void aosDumpDoubles(FILE *f, const double *p, size_t n) {
    aosDumpRecHeader(f, 'd', n);
    std::fwrite(p, sizeof(double), n, f);
}

inline void aosDumpU64(FILE *f, uint64_t v) {
    aosDumpRecHeader(f, 'q', 1);
    std::fwrite(&v, sizeof(v), 1, f);
}

// Contiguous block sums tiling the WHOLE array (every element contributes to
// exactly one sum, in a fixed serial order). A skipped/uncomputed element
// shifts its block's sum by O(element magnitude), far beyond the fp-noise
// tolerance, while benign accumulation-order noise stays within atol/rtol.
inline void aosDumpBlockSums(FILE *f, const double *p, size_t n, size_t block) {
    const uint64_t cnt = (n + block - 1) / block;
    aosDumpRecHeader(f, 'd', cnt);
    for (size_t start = 0; start < n; start += block) {
        const size_t end = (start + block < n) ? (start + block) : n;
        double s = 0.0;
        for (size_t i = start; i < end; i += 1) {
            s += p[i];
        }
        std::fwrite(&s, sizeof(s), 1, f);
    }
}

// Strided full-precision sample of a large array. Always paired with a
// full-coverage record (block sums or digest).
inline void aosDumpSampleDoubles(FILE *f, const double *p, size_t n, size_t stride) {
    const uint64_t cnt = (n + stride - 1) / stride;
    aosDumpRecHeader(f, 'd', cnt);
    for (size_t i = 0; i < n; i += stride) {
        std::fwrite(&p[i], sizeof(double), 1, f);
    }
}

inline void aosDumpSampleInts(FILE *f, const int *p, size_t n, size_t stride) {
    const uint64_t cnt = (n + stride - 1) / stride;
    aosDumpRecHeader(f, 'i', cnt);
    for (size_t i = 0; i < n; i += stride) {
        std::fwrite(&p[i], sizeof(int), 1, f);
    }
}

// Position-weighted exact digest of a full int array (int outputs are exact,
// so exact comparison is safe). Covers EVERY element.
inline uint64_t aosDigestInts(const int *p, size_t n) {
    uint64_t acc = 0;
    for (size_t i = 0; i < n; i += 1) {
        acc += aosSplitmix64(i) * (uint64_t)(int64_t)p[i];
    }
    return acc;
}
