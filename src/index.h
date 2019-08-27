/*
 * index.h
 *
 * advanced indexing structure
 */

#ifndef MII_INDEX_H
#define MII_INDEX_H

#define MII_INDEX_WIDTH 4096

typedef struct _mii_index_entry {
    struct _mii_index_entry* next;
    char* code;
    mii_strlist bins, modulepaths;
} mii_index_entry;

typedef struct _mii_index {
    char* root;
    mii_index_entry* entries[MII_INDEX_WIDTH];
} mii_index;

int mii_index_init(mii_index* p, const char* root);
void mii_index_free(mii_index* p);

int mii_index_crawl(mii_index* p);
int mii_index_merge_cache(mii_index* p);
int mii_index_analyze(mii_index* p);
int mii_index_write_cache(mii_index* p);
int mii_index_hash(const char* path);

#endif /* MII_INDEX_H */
