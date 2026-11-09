/* Main driver for generated openmp C++ code (AOS-Bench hardened rework).
*
* This driver is compiled twice by tests/test.sh:
*   - agent build: cpu.cc pulls in ONLY /workdir/generated-code.hpp
*   - baseline build (-DPAREVAL_BASELINE_ONLY): cpu.cc pulls in ONLY the
*     trusted baseline.hpp, and /workdir is not on the include path.
*
* Anti-cheat invariants:
*   - Nothing this binary prints is trusted in the agent build; test.sh only
*     parses SerialComputeTime from the baseline build.
*   - Correctness is judged by comparing the per-iteration dump records
*     (written below by trusted code) of the two builds, at the full
*     DRIVER_PROBLEM_SIZE. Every timed iteration is recorded, so skipping
*     iterations, caching, or special-casing sizes produces a mismatch.
*   - The agent build's wall time is measured externally by test.sh.
*
* Externally available functions (defined per-benchmark in cpu.cc):
*   - init() -- returns a pointer to a context object (deterministic inputs)
*   - compute(Context *ctx) -- runs the benchmark implementation
*   - advance(Context *ctx) -- deterministically derives the next iteration's
*     input (trusted, identical in both builds)
*   - dumpIter(Context *ctx, FILE *f) -- appends this iteration's output
*   - destroy(Context *ctx) -- frees the context object
*/
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cfloat>

#include <omp.h>

class Context;
extern "C++" {
    Context *init();
    void compute(Context *ctx);
    void advance(Context *ctx);
    void dumpIter(Context *ctx, FILE *f);
    void destroy(Context *ctx);
}

int main(int argc, char **argv) {

    if (argc != 3) {
        std::fprintf(stderr, "Usage: %s <num_threads> <dump_file>\n", argv[0]);
        return 2;
    }

    const int NITER = 5;
    const int num_threads = std::stoi(std::string(argv[1]));
    omp_set_num_threads(num_threads);

    FILE *dumpFile = std::fopen(argv[2], "wb");
    if (dumpFile == nullptr) {
        std::fprintf(stderr, "cannot open dump file %s\n", argv[2]);
        return 2;
    }

    /* initialize (deterministic: cpu.cc uses a private fixed-seed RNG, so both
       builds see identical input sequences even if generated code calls rand()) */
    Context *ctx = init();

    /* benchmark: record EVERY iteration's output */
    double computeTime = 0.0;
    for (int i = 0; i < NITER; i += 1) {
        const double start = omp_get_wtime();
        compute(ctx);
        computeTime += omp_get_wtime() - start;

        dumpIter(ctx, dumpFile);
        if (i + 1 < NITER) {
            advance(ctx);
        }
    }

    if (std::fclose(dumpFile) != 0) {
        std::fprintf(stderr, "failed to close dump file\n");
        return 2;
    }

#if defined(PAREVAL_BASELINE_ONLY)
    /* Only the trusted baseline build reports timing; the agent build's stdout
       is ignored by the verifier. */
    std::printf("SerialComputeTime: %.*f\n", DBL_DIG - 1, computeTime);
#endif

    /* cleanup */
    destroy(ctx);

    return 0;
}
