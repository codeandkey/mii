/*
 * strlist.h
 *
 * flexible mutable string list structure
 */

#ifndef MII_STRLIST_H
#define MII_STRLIST_H

typedef struct _mii_strlist {
    int len;
    char** data;
} mii_strlist;

void mii_strlist_init(mii_strlist* p);
void mii_strlist_free(mii_strlist* p);

void mii_strlist_push_copy(mii_strlist* p, const char* str);
void mii_strlist_push_move(mii_strlist* p, char* str);

int mii_strlist_len(mii_strlist* p);
char* mii_strlist_at(mii_strlist* p, int ind);

#endif
