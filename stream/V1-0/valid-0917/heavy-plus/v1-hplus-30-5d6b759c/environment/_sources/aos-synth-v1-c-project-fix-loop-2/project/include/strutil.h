#ifndef LIBCOMPUTE_STRUTIL_H
#define LIBCOMPUTE_STRUTIL_H

#include <stddef.h>

/* Duplicate s into a freshly malloc'd, NUL-terminated buffer. Returns NULL
 * on allocation failure or if s is NULL. Caller frees. */
char *su_strdup(const char *s);

/* Trim ASCII whitespace from both ends. Writes into out (must have room for
 * strlen(s)+1). Returns number of chars written (excluding NUL). */
size_t su_trim(const char *s, char *out);

/* Case-insensitive ASCII compare. Returns <0, 0, >0 like strcmp. */
int su_icmp(const char *a, const char *b);

/* Reverse s in place (NUL-terminated). Returns s. */
char *su_reverse(char *s);

/* Return length of longest common prefix of a and b. */
size_t su_common_prefix_len(const char *a, const char *b);

#endif
