#ifndef MII_ANALYSIS_H
#define MII_ANALYSIS_H

#if MII_ENABLE_SPIDER
#include "cjson/cJSON.h"
#include "modtable.h"
#endif

#define MII_ANALYSIS_LINEBUF_SIZE 512

/*
 * analysis.h
 *
 * functions for analyzing module files and extracting command names
 */

int mii_analysis_init();
void mii_analysis_free();

int mii_analysis_run(const char* modfile, int modtype, char*** bins_out, int* num_bins_out);

#if MII_ENABLE_SPIDER
int mii_analysis_parse_module_json(const cJSON* mod_json, mii_modtable_entry* mod);
#endif

#endif
