// Trusted dump-record comparator for the AOS-Bench ParEval verifier.
// Compares two record streams produced by the baseline and agent builds of
// the benchmark driver (see utilities.hpp for the record format).
//
// Usage: compare <reference_dump> <test_dump> <atol> <rtol>
// Exit:  0 = match, 1 = mismatch, 2 = malformed/IO error.
//
// 'd' records: |test - ref| <= atol + rtol * |ref| per element (NaN fails).
// 'i' / 'q' records: exact equality.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <vector>

static const size_t CHUNK = 1 << 20;

int main(int argc, char **argv) {
    if (argc != 5) {
        std::fprintf(stderr, "Usage: %s <ref> <test> <atol> <rtol>\n", argv[0]);
        return 2;
    }
    FILE *fr = std::fopen(argv[1], "rb");
    FILE *ft = std::fopen(argv[2], "rb");
    const double atol = std::atof(argv[3]);
    const double rtol = std::atof(argv[4]);
    if (fr == nullptr || ft == nullptr) {
        std::fprintf(stderr, "compare: cannot open dump file\n");
        return 2;
    }

    size_t record = 0;
    size_t badCount = 0;
    double maxDiff = 0.0;
    bool sawAny = false;

    for (;;) {
        const int cr = std::fgetc(fr);
        const int ct = std::fgetc(ft);
        if (cr == EOF || ct == EOF) {
            if (cr != ct) {
                std::fprintf(stderr, "compare: record %zu: stream length mismatch\n", record);
                return 1;
            }
            break;
        }
        uint64_t nr = 0, nt = 0;
        if (std::fread(&nr, sizeof(nr), 1, fr) != 1 || std::fread(&nt, sizeof(nt), 1, ft) != 1) {
            std::fprintf(stderr, "compare: record %zu: truncated header\n", record);
            return 2;
        }
        if (cr != ct || nr != nt) {
            std::fprintf(stderr, "compare: record %zu: type/count mismatch ('%c'/%llu vs '%c'/%llu)\n",
                         record, cr, (unsigned long long)nr, ct, (unsigned long long)nt);
            return 1;
        }
        sawAny = true;

        if (cr == 'd') {
            std::vector<double> a(CHUNK), b(CHUNK);
            uint64_t left = nr;
            while (left > 0) {
                const size_t take = (left < CHUNK) ? (size_t)left : CHUNK;
                if (std::fread(a.data(), sizeof(double), take, fr) != take ||
                    std::fread(b.data(), sizeof(double), take, ft) != take) {
                    std::fprintf(stderr, "compare: record %zu: truncated payload\n", record);
                    return 2;
                }
                for (size_t i = 0; i < take; i += 1) {
                    const double diff = std::fabs(a[i] - b[i]);
                    // written so NaN anywhere fails the comparison
                    if (!(diff <= atol + rtol * std::fabs(a[i]))) {
                        badCount += 1;
                        if (!(diff <= maxDiff)) maxDiff = diff;
                    }
                }
                left -= take;
            }
        } else if (cr == 'i') {
            std::vector<int> a(CHUNK), b(CHUNK);
            uint64_t left = nr;
            while (left > 0) {
                const size_t take = (left < CHUNK) ? (size_t)left : CHUNK;
                if (std::fread(a.data(), sizeof(int), take, fr) != take ||
                    std::fread(b.data(), sizeof(int), take, ft) != take) {
                    std::fprintf(stderr, "compare: record %zu: truncated payload\n", record);
                    return 2;
                }
                if (std::memcmp(a.data(), b.data(), take * sizeof(int)) != 0) {
                    badCount += 1;
                }
                left -= take;
            }
        } else if (cr == 'q') {
            std::vector<uint64_t> a(CHUNK), b(CHUNK);
            uint64_t left = nr;
            while (left > 0) {
                const size_t take = (left < CHUNK) ? (size_t)left : CHUNK;
                if (std::fread(a.data(), sizeof(uint64_t), take, fr) != take ||
                    std::fread(b.data(), sizeof(uint64_t), take, ft) != take) {
                    std::fprintf(stderr, "compare: record %zu: truncated payload\n", record);
                    return 2;
                }
                if (std::memcmp(a.data(), b.data(), take * sizeof(uint64_t)) != 0) {
                    badCount += 1;
                }
                left -= take;
            }
        } else {
            std::fprintf(stderr, "compare: record %zu: unknown type '%c'\n", record, cr);
            return 2;
        }
        record += 1;
    }

    if (!sawAny) {
        std::fprintf(stderr, "compare: empty dump streams\n");
        return 1;
    }
    if (badCount > 0) {
        std::fprintf(stderr, "compare: MISMATCH: %zu bad elements/records over %zu records (max |diff| %.9g)\n",
                     badCount, record, maxDiff);
        return 1;
    }
    std::fprintf(stderr, "compare: OK: %zu records match (atol=%g rtol=%g)\n", record, atol, rtol);
    return 0;
}
