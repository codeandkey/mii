#pragma once

/*
 * mii_modtable
 *
 * module entry container
 * keeps track of every module in the local filesystem
 */

#include <time.h>

#include "search_result.h"

/* modulo for the hashtable, preferably a power of 2 */
#define MII_MODTABLE_HASHTABLE_WIDTH 4096

/* minimum levenshtein distance for bins to be considered 'similar' */
#define MII_MODTABLE_DISTANCE_THRESHOLD 4

/* module file types */
#define MII_MODTABLE_MODTYPE_LMOD 0
#define MII_MODTABLE_MODTYPE_TCL 1

#define MII_MODTABLE_SPIDER_CMD ""
#define MII_MODTABLE_BUF_SIZE 4096

typedef struct _mii_modtable_entry {
    char* path, *code;
    int type, num_bins, num_parents;
    char** bins, **parents;
    time_t timestamp;
    int analysis_complete; /* truthy if the bin list is confirmed to be complete */
    struct _mii_modtable_entry* next;
} mii_modtable_entry;

typedef struct _mii_modtable {
    int analysis_complete, num_modules, modules_requiring_analysis;
    mii_modtable_entry* buf[MII_MODTABLE_HASHTABLE_WIDTH];
    char* modulepath; /* split into chunks on init via strtok() */
} mii_modtable;

void mii_modtable_init(mii_modtable* p);
void mii_modtable_free(mii_modtable* p);

int mii_modtable_gen(mii_modtable* p, char* modulepath); /* scan for modules and build a partial table */
int mii_modtable_import(mii_modtable* p, const char* path); /* import an existing table from the disk */

#if MII_ENABLE_SPIDER
int mii_modtable_spider_gen(mii_modtable* p, const char* path, int* count);
#endif

int mii_modtable_preanalysis(mii_modtable* p, const char* path); /* preanalyze up-to-date modules */
int mii_modtable_analysis(mii_modtable* p, int* count); /* perform analysis on all required modules */
int mii_modtable_export(mii_modtable* p, const char* output_path); /* export table to disk, overwriting */

int mii_modtable_search_exact(mii_modtable* p, const char* cmd, mii_search_result* res);
int mii_modtable_search_similar(mii_modtable* p, const char* cmd, mii_search_result* res);
int mii_modtable_search_info(mii_modtable* p, const char* code, mii_search_result* res);
