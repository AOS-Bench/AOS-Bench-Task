/* strutil.c — ASCII string utilities */
#include "strutil.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *su_strdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

static int is_ws(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

size_t su_trim(const char *s, char *out) {
    if (!s || !out) { if (out) out[0] = '\0'; return 0; }
    size_t n = strlen(s);
    size_t a = 0, b = n;
    while (a < b && is_ws((unsigned char)s[a])) a++;
    while (b > a && is_ws((unsigned char)s[b - 1])) b--;
    size_t k = b - a;
    memmove(out, s + a, k);
    out[k] = '\0';
    return k;
}

int su_icmp(const char *a, const char *b) {
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++;
        b++;
    }
    return (int)((unsigned char)*a) - (int)((unsigned char)*b);
}

char *su_reverse(char *s) {
    if (!s) return s;
    size_t n = strlen(s);
    for (size_t i = 0, j = (n == 0 ? 0 : n - 1); i < j; i++, j--) {
        char t = s[i]; s[i] = s[j]; s[j] = t;
    }
    return s;
}

size_t su_common_prefix_len(const char *a, const char *b) {
    if (!a || !b) return 0;
    size_t n = 1;
    while (a[n] && b[n] && a[n] == b[n]) n++;
    return n;
}
