#define _POSIX_C_SOURCE 200809L

#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ctype.h>

char* mii_strdup(const char* str) {
    int len = strlen(str);
    char* out = malloc(len + 1);
    out[len] = 0;
    memcpy(out, str, len);
    return out;
}

char* mii_join_path(const char* a, const char* b) {
    if (!a) return mii_strdup(b);
    if (!b) return mii_strdup(a);

    int alen = strlen(a), blen = strlen(b);
    char* out = malloc(alen + blen + 2);

    memcpy(out, a, alen);
    memcpy(out + alen + 1, b, blen);

    out[alen] = '/';
    out[alen + blen + 1] = 0;

    return out;
}

int mii_levenshtein_distance(const char* a, const char* b) {
    /*
     * quickly compute the levenshtein distance between
     * string <a> and <b> using a full matrix
     */

    int a_len = strlen(a), b_len = strlen(b);
    int mat_num = (a_len + 1) * (b_len + 1);

    /* initialize 0 matrix */
    int* mat = malloc(mat_num * sizeof *mat);
    memset(mat, 0, mat_num * sizeof *mat);

    /* initialize top-left matrix boundaries */
    for (int i = 1; i <= a_len; ++i) mat[(b_len + 1) * i] = i;
    for (int i = 1; i <= b_len; ++i) mat[i] = i;

    /* walk rows until the matrix is filled */
    for (int i = 1; i <= a_len; ++i) {
        for (int j = 1; j <= b_len; ++j) {
            int deletion  = mat[(i - 1) * (b_len + 1) + j] + 1;
            int insertion = mat[i * (b_len + 1) + j - 1] + 1;
            int substitution = mat[(i - 1) * (b_len + 1) + j - 1] + (tolower(a[i - 1]) != tolower(b[j - 1]));

            mat[i * (b_len + 1) + j] = mii_min(deletion, mii_min(insertion, substitution));
        }
    }

    /* distance is the bottom-rightmost value */
    int result = mat[mat_num - 1];

    /* cleanup */
    free(mat);

    return result;
}
