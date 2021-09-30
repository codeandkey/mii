#define _POSIX_C_SOURCE 200809L

#include "search_result.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/* sorting helper functions */
void _mii_search_result_swap(mii_search_result* res, int a, int b);
int _mii_search_result_compare(mii_search_result* res, int a, int b);
int _mii_search_result_compare_codes(const char* code_a, const char* code_b);
int _mii_search_result_get_priority(const char* parents, const char* code);

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
        free(dest->parents[i]);
    }

    free(dest->query);

    /* drop result arrays */
    free(dest->codes);
    free(dest->bins);
    free(dest->parents);
    free(dest->distances);
    free(dest->priorities);
}

void mii_search_result_add(mii_search_result* p, const char* code, const char* bin, int distance, const char* parents) {
    ++p->num_results;

    /* resize result arrays */
    p->codes = realloc(p->codes, p->num_results * sizeof *p->codes);
    p->bins = realloc(p->bins, p->num_results * sizeof *p->bins);
    p->distances = realloc(p->distances, p->num_results * sizeof *p->distances);
    p->parents = realloc(p->parents, p->num_results * sizeof *p->parents);
    p->priorities = realloc(p->priorities, p->num_results * sizeof *p->priorities);

    /* duplicate code/bin and insert into results */
    p->codes[p->num_results - 1] = mii_strdup(code);
    p->bins[p->num_results - 1] = mii_strdup(bin);
    p->distances[p->num_results - 1] = distance;
    p->parents[p->num_results - 1] = (parents != NULL) ? mii_strdup(parents) : mii_strdup("");
    p->priorities[p->num_results - 1] = _mii_search_result_get_priority(parents, code);
}

int mii_search_result_next(mii_search_result* p, char** code, char** bin, char** parent, int* distance) {
    if (p->cur_result >= p->num_results) return -1; /* no more results */

    /* give the caller whatever values they ask for */
    if (code) *code = p->bins[p->cur_result];
    if (bin) *bin = p->bins[p->cur_result];
    if (parent) *parent = p->parents[p->cur_result];
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
                fprintf(f, "    { \"code\": \"%s\", \"parents\": \"%s\" },\n", p->codes[i], p->parents[i]);
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

                /* compute the maximum code + bin width */
                int max_codewidth = 8, max_parentwidth = 11;

                for (int k = 0; k < p->num_results; ++k) {
                    int clen = strlen(p->codes[k]), plen = strlen(p->parents[k]);
                    if (clen > max_codewidth) max_codewidth = clen;
                    if (plen > max_parentwidth) max_parentwidth = plen;
                }

                for (int k = 0; k < p->num_results; ++k) {
                    fprintf(f, "    %-*s", max_codewidth, p->codes[k]);
                    if (should_color) fprintf(f, "\033[0;36m");
                    fprintf(f, "    %-*s\n", max_parentwidth, p->parents[k]);
                    if (should_color) fprintf(f, "\033[0;39m");
                }
            }
        }
        break;
    case MII_SEARCH_RESULT_MODE_FUZZY: ;
        int num_results = p->num_results;
        if (num_results > MII_SEARCH_RESULT_FUZZY_MAX) {
            num_results = MII_SEARCH_RESULT_FUZZY_MAX;
        }

        if (flags & MII_SEARCH_RESULT_JSON) {
            fprintf(f, "[\n");

            for (int i = 0; i < num_results; ++i) {
                fprintf(f, "    { \"code\": \"%s\", \"command\": \"%s\", \"parents\": \"%s\", \"distance\": %d },\n", p->codes[i], p->bins[i], p->parents[i], p->distances[i]);
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

            if (num_results) fprintf(f, " (total %d)", num_results);
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
            int max_codewidth = 8, max_binwidth = 9, max_parentwidth = 11;

            for (int k = 0; k < num_results; ++k) {
                int clen = strlen(p->codes[k]), blen = strlen(p->bins[k]), plen = strlen(p->parents[k]);

                if (clen > max_codewidth) max_codewidth = clen;
                if (blen > max_binwidth) max_binwidth = blen;
                if (plen > max_parentwidth) max_parentwidth = plen;
            }

            /* render everything in pretty columns and colors */
            if (!num_results) {
                if (should_color) fprintf(f, "\033[2;37m");
                fprintf(f, "    empty result set :(\n");
                if (should_color) fprintf(f, "\033[0;39m");
            } else {
                fprintf(f, "    %-*s", max_codewidth, "MODULE");
                fprintf(f, "    %-*s", max_binwidth, "COMMAND");
                fprintf(f, "    %-*s", max_parentwidth, "PARENT(S)");
                fprintf(f, "    %s\n", "RELEVANCE");

                if (should_color) fprintf(f, "\033[0;39m");

                for (int k = 0; k < num_results; ++k) {
                    fprintf(f, "    %-*s", max_codewidth, p->codes[k]);

                    if (should_color) fprintf(f, "\033[0;36m");
                    fprintf(f, "    %-*s", max_binwidth, p->bins[k]);
                    if (should_color) fprintf(f, "\033[0;39m");

                    fprintf(f, "    %-*s", max_parentwidth, p->parents[k]);

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
    /* order results based on multiple factors */
    /* a simple selection sort will do fine */

    int min_index;

    /* no results to sort */
    if (!res->num_results) return;

    for (int i = 0; i < res->num_results - 1; ++i) {
        min_index = i;

        for (int j = i + 1; j < res->num_results; ++j) {
            if (_mii_search_result_compare(res, j, min_index) < 0) {
                min_index = j;
            }
        }

        if (min_index != i) {
            _mii_search_result_swap(res, i, min_index);
        }
    }
}

void _mii_search_result_swap(mii_search_result* res, int a, int b) {
    int tmp_num;

    /* swap distances */
    tmp_num = res->distances[a];
    res->distances[a] = res->distances[b];
    res->distances[b] = tmp_num;

    /* swap priorities */
    tmp_num = res->priorities[a];
    res->priorities[a] = res->priorities[b];
    res->priorities[b] = tmp_num;

    char* tmp;

    /* swap bins */
    tmp = res->bins[a];
    res->bins[a] = res->bins[b];
    res->bins[b] = tmp;

    /* swap codes */
    tmp = res->codes[a];
    res->codes[a] = res->codes[b];
    res->codes[b] = tmp;

    /* swap parents */
    tmp = res->parents[a];
    res->parents[a] = res->parents[b];
    res->parents[b] = tmp;
}

/* compare different search results in the following order: */
/* binary -> priority -> parent -> code */
int _mii_search_result_compare(mii_search_result* res, int a, int b) {
    int diff;

    /* compare binary distances */
    diff = res->distances[a] - res->distances[b];
    if (diff > 0) return 1;
    if (diff < 0) return -1;

    /* compare priorities */
    diff = res->priorities[a] - res->priorities[b];
    if (diff < 0) return 1;
    if (diff > 0) return -1;

    /* compare parent alpha */
    diff = strcmp(res->parents[a], res->parents[b]);
    if (diff < 0) return 1;
    if (diff > 0) return -1;

    /* finally, compare code alpha + version */
    return _mii_search_result_compare_codes(res->codes[a], res->codes[b]);
}

/* compare module names alphabetically and versions numerically */
int _mii_search_result_compare_codes(const char* code1, const char* code2) {
    char* code1_cpy = mii_strdup(code1);
    char* code2_cpy = mii_strdup(code2);

    char* p1, *p2;
    int diff;

    int is_versioned = (strchr(code1_cpy, '/') != NULL) && (strchr(code2_cpy, '/') != NULL);

    /* if versioned, split accordingly */
    if (is_versioned) {
        /* fine since pointer stays at beginning */
        code1_cpy = strtok_r(code1_cpy, "/", &p1);
        code2_cpy = strtok_r(code2_cpy, "/", &p2);
    }

    diff = strcmp(code1_cpy, code2_cpy);

    /* order found or only compare alphas, cleanup before returning */
    if (diff != 0 || !is_versioned) {
        free(code1_cpy);
        free(code2_cpy);
    }

    if (diff > 0) return 1;
    if (diff < 0) return -1;

    if (!is_versioned) return 0;

    /* compare versions numerically */
    char* token1 = strtok_r(NULL, ".", &p1);
    char* token2 = strtok_r(NULL, ".", &p2);

    while(diff == 0 && token1 != NULL && token2 != NULL) {
        char* endptr1, *endptr2;

        diff = strtol(token1, &endptr1, 10) - strtol(token2, &endptr2, 10);

        /* one of the tokens is not a number */
        if (token1 == endptr1 || token2 == endptr2) {
            /* flipped to fit with the strtol return */
            diff = - strcmp(token1, token2);
        }

        token1 = strtok_r(NULL, ".", &p1);
        token2 = strtok_r(NULL, ".", &p2);
    }

    free(code1_cpy);
    free(code2_cpy);

    if (diff < 0) return 1;
    if (diff > 0) return -1;

    return 0;
}

int _mii_search_result_get_priority(const char* parents, const char* code) {
    int priority = 0;
    char* loaded_modules = getenv("LOADEDMODULES");

    /* no modules loaded, no need to check */
    if (loaded_modules == NULL) {
        if (parents == NULL) return MII_SEARCH_RESULT_PRIORITY_NO_PARENT;
        return 0;
    }

    /* iterate over every loaded module */
    loaded_modules = mii_strdup(loaded_modules);
    char* module_name = strtok(loaded_modules, ":");

    while(module_name != NULL) {
        /* check if module providing the cmd is already loaded */
        if (strcmp(code, module_name) == 0) {
            priority = MII_SEARCH_RESULT_PRIORITY_LOADED_MOD;
            break;
        }

        /* check if parent is already loaded */
        if (parents != NULL && strstr(parents, module_name) != NULL) {
            priority += MII_SEARCH_RESULT_PRIORITY_LOADED_PARENT;
        }

        module_name = strtok(NULL, ":");
    }
    free(loaded_modules);

    /* if no parents, but module is not loaded */
    if (parents == NULL && priority != MII_SEARCH_RESULT_PRIORITY_LOADED_MOD) {
        priority = MII_SEARCH_RESULT_PRIORITY_NO_PARENT;
    }

    return priority;
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
