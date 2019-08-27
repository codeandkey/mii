#include "strlist.h"

#include <stdlib.h>
#include <string.h>

#include "util.h"

void mii_strlist_init(mii_strlist* p) {
    p->len = 0;
    p->data = NULL;
}

void mii_strlist_free(mii_strlist* p) {
    for (int i = 0; i < p->len; ++i) {
        free(p->data[i]);
    }

    if (p->data) {
        free(p->data);
        p->data = NULL;
    }

    p->len = 0;
}

void mii_strlist_push_copy(mii_strlist* p, const char* str) {
    mii_strlist_push_move(p, mii_strdup(str));
}

void mii_strlist_push_move(mii_strlist* p, char* str) {
    p->data = realloc(p->data, ++p->len * sizeof *p->data);
    p->data[p->len - 1] = str;
}

int mii_strlist_len(mii_strlist* p) {
    return p->len;
}

char* mii_strlist_at(mii_strlist* p, int ind) {
    return p->data[ind];
}
