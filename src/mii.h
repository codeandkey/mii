#ifndef MII_H
#define MII_H

#define MII_VERSION "1.1.1"

/*
 * mii interface
 */

#include "modtable.h"
#include "search_result.h"

/* options should be set BEFORE mii_init() is called */

void mii_option_modulepath(const char* modulepath);
void mii_option_datadir(const char* datadir);

int mii_init();
void mii_free();

/* rebuild the entire cache */
int mii_build();

/* time-based cache sync, updates out-of-date modules */
int mii_sync();

/* list modules */
int mii_list();

/* search operations output a JSON result to stdout */
int mii_search_exact(mii_search_result* res, const char* cmd);
int mii_search_fuzzy(mii_search_result* res, const char* cmd);
int mii_search_info(mii_search_result* res, const char* code);

/* status operations */
int mii_enable();
int mii_disable();
int mii_status();

#endif
