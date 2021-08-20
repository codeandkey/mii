#define _POSIX_C_SOURCE 200809L

#include "search_result.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int mii_search_result_init(mii_search_result* dest, const char* query) {
    memset(dest, 0, sizeof *dest);
    dest->query = mii_strdup(query);
    return 0;
}

void mii_search_result_free(mii_search_result* dest) {
    /* drop allocated result values */
    for (int i = 0; i < dest->num_results; ++i) {
        free(dest->codes[i]);
        free(dest->bins[i]);
    }

    /* drop result arrays */
    free(dest->codes);
    free(dest->bins);
    free(dest->distances);
}

void mii_search_result_add(mii_search_result* p, const char* code, const char* bin, int distance) {
    ++p->num_results;

    /* resize result arrays */
    p->codes = realloc(p->codes, p->num_results * sizeof *p->codes);
    p->bins = realloc(p->bins, p->num_results * sizeof *p->bins);
    p->distances = realloc(p->distances, p->num_results * sizeof *p->distances);

    /* duplicate code/bin and insert into results */
    p->codes[p->num_results - 1] = mii_strdup(code);
    p->bins[p->num_results - 1] = mii_strdup(bin);
    p->distances[p->num_results - 1] = distance;
}

int mii_search_result_next(mii_search_result* p, char** code, char** bin, int* distance) {
    if (p->cur_result >= p->num_results) return -1; /* no more results */

    /* give the caller whatever values they ask for */
    if (code) *code = p->bins[p->cur_result];
    if (bin) *bin = p->bins[p->cur_result];
    if (distance) *distance = p->distances[p->cur_result];

    ++p->cur_result;

    /* return 0, indicating that a result was present */
    return 0;
}

int mii_search_result_write(mii_search_result* p, FILE* f, int mode, int flags) {
    if (!f) return -1;

    int should_color = isatty(fileno(f)) && !(flags & MII_SEARCH_RESULT_NOCOLOR);

    switch (mode) {
    case MII_SEARCH_RESULT_MODE_EXACT:
        if (flags & MII_SEARCH_RESULT_JSON) {
            fprintf(f, "[\n");

            for (int i = 0; i < p->num_results; ++i) {
                fprintf(f, "    { \"code\": \"%s\" },\n", p->codes[i]);
            }

            fprintf(f, "]\n");
        } else {
            /* pretty-print module result listing */
            if (should_color) fprintf(f, "\033[1;37m");
            fprintf(f, "Modules providing ");

            if (should_color) fprintf(f, "\033[1;36m");
            fprintf(f, "%s", p->query);
            if (should_color) fprintf(f, "\033[1;37m");
            fprintf(f, ":");

            if (p->num_results) fprintf(f, " (total %d)", p->num_results);
            fprintf(f, "\n");

            if (!p->num_results) {
                if (should_color) fprintf(f, "\033[2;37m");
                fprintf(f, "    empty result set :(\n");
                if (should_color) fprintf(f, "\033[0;39m");
            } else {
                if (should_color) fprintf(f, "\033[0;39m");

                for (int k = 0; k < p->num_results; ++k) {
                    fprintf(f, "    %s\n", p->codes[k]);
                }
            }
        }
        break;
    case MII_SEARCH_RESULT_MODE_FUZZY:
        if (p->num_results > MII_SEARCH_RESULT_FUZZY_MAX) {
            p->num_results = MII_SEARCH_RESULT_FUZZY_MAX;
        }

        if (flags & MII_SEARCH_RESULT_JSON) {
            fprintf(f, "[\n");

            for (int i = 0; i < p->num_results; ++i) {
                fprintf(f, "    { \"code\": \"%s\", \"command\": \"%s\", \"distance\": %d },\n", p->codes[i], p->bins[i], p->distances[i]);
            }

            fprintf(f, "]\n");
        } else {
            /* pretty-print module result listing */
            if (should_color) fprintf(f, "\033[1;37m");
            fprintf(f, "Results for ");

            if (should_color) fprintf(f, "\033[1;36m");
            fprintf(f, "\"%s\"", p->query);
            if (should_color) fprintf(f, "\033[1;37m");
            fprintf(f, ":");

            if (p->num_results) fprintf(f, " (total %d)", p->num_results);
            fprintf(f, "\n");

            /* formatting strings/colors for relevance values */
            const char* relevance_strings[] = {
                "exact",
                "high",
                "medium",
                "low",
            };

            const char* relevance_colors[] = {
                "\033[1;36m",
                "\033[1;32m",
                "\033[1;33m",
                "\033[1;31m",
            };

            /* compute the maximum code + bin width */
            int max_codewidth = 8, max_binwidth = 9;

            for (int k = 0; k < p->num_results; ++k) {
                int clen = strlen(p->codes[k]), blen = strlen(p->bins[k]);

                if (clen > max_codewidth) max_codewidth = clen;
                if (blen > max_binwidth) max_binwidth = blen;
            }

            /* render everything in pretty columns and colors */
            if (!p->num_results) {
                if (should_color) fprintf(f, "\033[2;37m");
                fprintf(f, "    empty result set :(\n");
                if (should_color) fprintf(f, "\033[0;39m");
            } else {
                fprintf(f, "    %-*s", max_codewidth, "MODULE");
                fprintf(f, "    %-*s", max_binwidth, "COMMAND");
                fprintf(f, "    %s\n", "RELEVANCE");

                if (should_color) fprintf(f, "\033[0;39m");

                for (int k = 0; k < p->num_results; ++k) {
                    fprintf(f, "    %-*s", max_codewidth, p->codes[k]);

                    if (should_color) fprintf(f, "\033[0;36m");
                    fprintf(f, "    %-*s", max_binwidth, p->bins[k]);
                    if (should_color) fprintf(f, "\033[0;39m");

                    if (should_color) fprintf(f, relevance_colors[p->distances[k]]);
                    fprintf(f, "    %s\n", relevance_strings[p->distances[k]]);
                    if (should_color) fprintf(f, "\033[0;39m");
                }

                if (should_color) fprintf(f, "\033[0;39m");
            }
        }
        break;
    case MII_SEARCH_RESULT_MODE_SHOW:
        if (flags & MII_SEARCH_RESULT_JSON) {
            fprintf(f, "[\n");

            for (int i = 0; i < p->num_results; ++i) {
                fprintf(f, "    { \"command\": \"%s\" },\n", p->bins[i]);
            }

            fprintf(f, "]\n");
        } else {
            fprintf(f, "Commands for ");

            if (should_color) fprintf(f, "\033[1;36m");
            fprintf(f, "%s", p->query);
            if (should_color) fprintf(f, "\033[0;39m");
            fprintf(f, ":");

            if (p->num_results) fprintf(f, " (total %d)", p->num_results);
            fprintf(f, "\n");

            if (!p->num_results) {
                if (should_color) fprintf(f, "\033[2;37m");
                fprintf(f, "    empty result set :(\n");
                if (should_color) fprintf(f, "\033[0;39m");
            } else {
                if (should_color) fprintf(f, "\033[1;37m");

                for (int k = 0; k < p->num_results; ++k) {
                    fprintf(f, "    %s\n", p->bins[k]);
                }

                if (should_color) fprintf(f, "\033[0;39m");
            }
        }
        break;
    }
    return 0;
}

void mii_search_result_sort(mii_search_result* res) {
    /* order results by lowest distance */
    /* a simple selection sort will do fine */

    char* tmp;
    int min_dist, min_dist_index;

    /* no results to sort */
    if (!res->num_results) return;

    for (int i = 0; i < res->num_results - 1; ++i) {
        min_dist = res->distances[i];
        min_dist_index = i;

        for (int j = i + 1; j < res->num_results; ++j) {
            if (res->distances[j] < min_dist) {
                min_dist = res->distances[j];
                min_dist_index = j;
            }
        }

        if (min_dist_index != i) {
            /* swap rows */
            res->distances[min_dist_index] = res->distances[i];
            res->distances[i] = min_dist;

            tmp = res->bins[min_dist_index];
            res->bins[min_dist_index] = res->bins[i];
            res->bins[i] = tmp;

            tmp = res->codes[min_dist_index];
            res->codes[min_dist_index] = res->codes[i];
            res->codes[i] = tmp;
        }
    }
}


int mii_search_result_get_unique_bins(mii_search_result* res, char*** bins_out, int* num_results) {
    /* get first <*num_results> unique bins */

    int unique_count = 0;

    for (int i = 0, j; i < res->num_results && unique_count < *num_results; ++i) {
        for (j = 0 ; j < unique_count; ++j) {
            if (strcmp(res->bins[i], (*bins_out)[j]) == 0) {
                /* duplicate found */
                break;
            }
        }
        if (j == unique_count) {
            /* unique bin */
            *bins_out = realloc(*bins_out, sizeof(char*) * (unique_count + 1));
            (*bins_out)[unique_count++] = res->bins[i];
        }
    }

    /* return true number of unique bins */
    *num_results = unique_count;

    return 0;
}
