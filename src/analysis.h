#ifndef MII_ANALYSIS_H
#define MII_ANALYSIS_H

#define MII_ANALYSIS_LINEBUF_SIZE 512

/*
 * analysis.h
 *
 * functions for analyzing module files and extracting command names
 */

int mii_analysis_init();
void mii_analysis_free();

int mii_analysis_run(const char* modfile, int modtype, char*** bins_out, int* num_bins_out);

#endif
