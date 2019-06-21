#ifndef MII_SEARCH_RESULT_H
#define MII_SEARCH_RESULT_H

/*
 * search result structure
 */

#include <stdio.h>

/*
 * export modes
 */

#define MII_SEARCH_RESULT_JSON    1
#define MII_SEARCH_RESULT_NOCOLOR 2

#define MII_SEARCH_RESULT_MODE_EXACT 0
#define MII_SEARCH_RESULT_MODE_FUZZY 1
#define MII_SEARCH_RESULT_MODE_SHOW  2

typedef struct _mii_search_result {
    int num_results, cur_result;
    char** codes, **bins, *query;
    int* distances;
} mii_search_result;

/* structure init + cleanup */

int mii_search_result_init(mii_search_result* dest, const char* query);
void mii_search_result_free(mii_search_result* dest);

/* adding results */

void mii_search_result_add(mii_search_result* p, const char* code, const char* bin, int distance);

/* sorting results */

void mii_search_result_sort(mii_search_result* p);

/* reading/outputting results */

int mii_search_result_next(mii_search_result* p, char** code, char** bin, int* distance);
int mii_search_result_write(mii_search_result* p, FILE* f, int type, int flags);

#endif
