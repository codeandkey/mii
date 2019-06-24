#define _POSIX_C_SOURC 200809L

#include "modtable.h"
#include "util.h"
#include "log.h"
#include "analysis.h"

#include "xxhash/xxhash.h"

#include <dirent.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* should never really need to change. identify the mii_modtable file format */
static const unsigned char MII_MODTABLE_MAGIC_BYTES[] = { 0xBE, 0xE5 };

int _mii_modtable_parse_from(mii_modtable* p, const char* path, int (*handler)(mii_modtable* p, char* path, char* code, char** bins, int num_bins, time_t timestamp));
int _mii_modtable_get_target_index(const char* path);
mii_modtable_entry* _mii_modtable_locate_entry(mii_modtable* p, const char* path);

/* parse handlers */
int _mii_modtable_parse_handler_import(mii_modtable* p, char* path, char* code, char** bins, int num_bins, time_t timestamp);
int _mii_modtable_parse_handler_preanalysis(mii_modtable* p, char* path, char* code, char** bins, int num_bins, time_t timestamp);

/* mii_modtable generation */
int _mii_modtable_gen_recursive(mii_modtable* p, const char* root);
int _mii_modtable_gen_recursive_sub(mii_modtable* p, const char* root, const char* prefix);

/* initialize an empty mii_modtable */
void mii_modtable_init(mii_modtable* out) {
    memset(out, 0, sizeof *out);
    mii_debug("Initialized empty mii_modtable, hash modulus %d", MII_MODTABLE_HASHTABLE_WIDTH);
}

/*
 * cleanup mii_modtable memory
 */
void mii_modtable_free(mii_modtable* p) {
    mii_modtable_entry* tmp;

    if (p->modulepath) free(p->modulepath);

    for (int i = 0; i < MII_MODTABLE_HASHTABLE_WIDTH; ++i) {
        mii_modtable_entry* cur = p->buf[i];

        while (cur) {
            free(cur->code);
            free(cur->path);

            for (int j = 0; j < cur->num_bins; ++j) {
                free(cur->bins[j]);
            }

            free(cur->bins);
            tmp = cur->next;

            free(cur);
            cur = tmp;
        }
    }

    memset(p, 0, sizeof *p);
}

/*
 * fill a mii_modtable with modules from the disk
 * will fail if the mii_modtable is not empty
 */
int mii_modtable_gen(mii_modtable* p, char* modulepath) {
    if (p->num_modules) {
        mii_error("Table already has modules present. Will not generate over it!\n");
        return -1;
    }

    if (!modulepath || !strlen(modulepath)) {
        mii_error("Empty or blank MODULEPATH! Will not generate module table.");
        return -1;
    }

    p->modulepath = mii_strdup(modulepath);

    /* split modulepath into roots, recursively crawl each */
    for (char* root = strtok(p->modulepath, ":"); root; root = strtok(NULL, ":")) {
        _mii_modtable_gen_recursive(p, root);
    }

    mii_debug("Found %d modules", p->num_modules);

    /* after gen, every module requires analysis */
    p->modules_requiring_analysis = p->num_modules;

    return 0;
}

/*
 * import a mii_modtable from the disk
 */
int mii_modtable_import(mii_modtable* p, const char* path) {
    /* consider imported cache to be current */
    p->analysis_complete = 1;

    return _mii_modtable_parse_from(p, path, _mii_modtable_parse_handler_import);
}

/*
 * perform pre-analysis
 *
 * pre-fills module bins if they are still up to date
 */
int mii_modtable_preanalysis(mii_modtable* p, const char* path) {
    return _mii_modtable_parse_from(p, path, _mii_modtable_parse_handler_preanalysis);
}

/*
 * perform analysis
 * reads any modulefiles that require analysis
 *
 * number of modules analyzed saved in *num if non-NULL
 */
int mii_modtable_analysis(mii_modtable* p, int* num) {
    mii_modtable_entry* cur;
    int count = 0;

    if (!p->modules_requiring_analysis) {
        p->analysis_complete = 1;
        if (num) *num = 0;
        return 0;
    }

    for (int i = 0; i < MII_MODTABLE_HASHTABLE_WIDTH; ++i) {
        cur = p->buf[i];

        while (cur) {
            if (!cur->analysis_complete) {
                /* need to perform analysis on this module */

                if (!mii_analysis_run(cur->path, cur->type, &cur->bins, &cur->num_bins)) {
                    mii_debug("analysis for %s : %d bins", cur->path, cur->num_bins);

                    cur->analysis_complete = 1;
                    ++count;
                }
            }

            cur = cur->next;
        }
    }

    if (num) *num = count;

    p->modules_requiring_analysis = 0;
    p->analysis_complete = 1;

    return 0;
}

/*
 * export a mii_modtable to disk
 */
int mii_modtable_export(mii_modtable* p, const char* path) {
    FILE* f = fopen(path, "wb");

    if (!f) {
        mii_error("Couldn't open %s for writing: %s\n", path, strerror(errno));
        return -1;
    }

    mii_debug("Exporting %d modules to %s", p->num_modules, path);

    /* write magic sequence */
    fwrite(MII_MODTABLE_MAGIC_BYTES, sizeof MII_MODTABLE_MAGIC_BYTES, 1, f);

    /* write number of expected modules */
    fwrite(&p->num_modules, sizeof p->num_modules, 1, f);

    /* write each module entry */
    for (int i = 0; i < MII_MODTABLE_HASHTABLE_WIDTH; ++i) {
        mii_modtable_entry* cur = p->buf[i];

        while(cur) {
            /* don't write modules that analysis failed for */
            if (!cur->analysis_complete) {
                cur = cur->next;
                continue;
            }

            /* write path size and data */
            int path_len = strlen(cur->path);
            fwrite(&path_len, sizeof path_len, 1, f);
            fwrite(cur->path, 1, path_len, f);

            /* write code size and data */
            int code_len = strlen(cur->code);
            fwrite(&code_len, sizeof code_len, 1, f);
            fwrite(cur->code, 1, code_len, f);

            /* write timestamp */
            fwrite(&cur->timestamp, sizeof cur->timestamp, 1, f);

            /* write number of bins */
            fwrite(&cur->num_bins, sizeof cur->num_bins, 1, f);

            /* write each bin length and data */
            for (int j = 0; j < cur->num_bins; ++j) {
                int bin_len = strlen(cur->bins[j]);
                fwrite(&bin_len, sizeof bin_len, 1, f);
                fwrite(cur->bins[j], 1, bin_len, f);
            }

            cur = cur->next;
        }
    }

    /* all done. close the fd */
    fclose(f);
    return 0;
}

/*
 * search for exact bin matches
 */
int mii_modtable_search_exact(mii_modtable* p, const char* cmd, mii_search_result* res) {
    if (!p->analysis_complete) return -1;

    mii_search_result_init(res, cmd);

    mii_debug("Searching for bin \"%s\"..", cmd);

    /* walk through the table and search for exact matches */
    for (int i = 0; i < MII_MODTABLE_HASHTABLE_WIDTH; ++i) {
        mii_modtable_entry* cur = p->buf[i];

        while (cur) {
            for (int j = 0; j < cur->num_bins; ++j) {
                if (!strcmp(cur->bins[j], cmd)) {
                    mii_search_result_add(res, cur->code, cmd, 0);
                }
            }

            cur = cur->next;
        }
    }

    return 0;
}

/*
 * search for similar bin matches
 */
int mii_modtable_search_similar(mii_modtable* p, const char* cmd, mii_search_result* res) {
    if (!p->analysis_complete) return -1;

    mii_search_result_init(res, cmd);

    mii_debug("Searching for bins similar to \"%s\"..", cmd);

    /* walk through the table and search for exact matches */
    for (int i = 0; i < MII_MODTABLE_HASHTABLE_WIDTH; ++i) {
        mii_modtable_entry* cur = p->buf[i];

        while (cur) {
            for (int j = 0; j < cur->num_bins; ++j) {
                int dist = mii_levenshtein_distance(cmd, cur->bins[j]); 

                if (dist < MII_MODTABLE_DISTANCE_THRESHOLD) {
                    mii_search_result_add(res, cur->code, cur->bins[j], dist);
                }
            }

            cur = cur->next;
        }
    }

    mii_search_result_sort(res);

    return 0;
}

/*
 * search for all commands provided by a module
 */
int mii_modtable_search_info(mii_modtable* p, const char* code, mii_search_result* res) {
    if (!p->analysis_complete) return -1;

    mii_search_result_init(res, code);

    /* walk through the table and search for exact matches */
    for (int i = 0; i < MII_MODTABLE_HASHTABLE_WIDTH; ++i) {
        mii_modtable_entry* cur = p->buf[i];

        while (cur) {
            if (!strcmp(cur->code, code)) {
                for (int j = 0; j < cur->num_bins; ++j) {
                    mii_search_result_add(res, cur->code, cur->bins[j], 0);
                }

                break;
            }

            cur = cur->next;
        }
    }

    return 0;
}

/*
 * recursively walk a root and add modules to the hashtable
 */
int _mii_modtable_gen_recursive(mii_modtable* p, const char* root) {
    return _mii_modtable_gen_recursive_sub(p, root, NULL);
}

/*
 * subroutine for _mii_modtable_gen_recursive which allows for recursively
 * computing the module relative paths (and the loading codes)
 */
int _mii_modtable_gen_recursive_sub(mii_modtable* p, const char* root, const char* prefix) {
    char* dir_path = mii_join_path(root, prefix);
    DIR* d = opendir(dir_path);

    struct dirent* dp;
    struct stat st;

    int result = 0;

    if (!d) {
        free(dir_path);
        return -1;
    }

    while ((dp = readdir(d))) {
        if (dp->d_name[0] == '.') continue;

        /* compute the absolute file path */
        char* abs_path = mii_join_path(dir_path, dp->d_name);

        /* stat the type */
        if (stat(abs_path, &st)) {
            mii_warn("Couldn't stat %s: %s", abs_path, strerror(errno));
            free(abs_path);
            continue;
        }

        char* rel_path = mii_join_path(prefix, dp->d_name);

        /* check for normal files (likely modules) */
        if (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode)) {
            /* parse the relative path to get the module type and code */
            int rel_len = strlen(rel_path);
            int mod_type = MII_MODTABLE_MODTYPE_TCL; /* assume tcl unless we detect lmod */

            if (strlen(rel_path) > 4 && !strcmp(rel_path + rel_len - 4, ".lua")) {
                /* mutate the rel_path in place for the code, we don't need it anymore anyway */
                rel_path[rel_len - 4] = 0;
                mod_type = MII_MODTABLE_MODTYPE_LMOD;
            }

            mii_debug("Found module %s at %s", rel_path, abs_path);

            /* insert the new module in */
            mii_modtable_entry* new_module = malloc(sizeof *new_module);

            new_module->path = abs_path;
            new_module->code = rel_path; /* rel_path was mutated to become the code */
            new_module->type = mod_type;
            new_module->timestamp = st.st_mtime;
            new_module->bins = NULL;
            new_module->num_bins = 0;
            new_module->analysis_complete = 0;

            int target_index = _mii_modtable_get_target_index(abs_path);

            new_module->next = p->buf[target_index];
            p->buf[target_index] = new_module;

            /* increment the counter */
            ++p->num_modules;

            /* skip the other checks and cleanup */
            continue;
        }

        /* add if module, recurse if directory */
        if (S_ISDIR(st.st_mode)) {
            result |= _mii_modtable_gen_recursive_sub(p, root, rel_path);
        }

        /*
         * only free these if entry is a non-module, otherwise
         * ownership is transferred to the module entry
         */
        free(rel_path);
        free(abs_path);
    }

    closedir(d);
    free(dir_path);
    return result;
}

/*
 * generic function for parsing saved modtables
 * <handler> is called for each imported module with allocated module info
 * if the handler returns nonzero this function is interrupted and returns immediately
 */
int _mii_modtable_parse_from(mii_modtable* p, const char* path, int (*handler)(mii_modtable* p, char* path, char* code, char** bins, int num_bins, time_t timestamp)) {
    int res = 0;
    FILE* f = fopen(path, "rb");

    /* TODO: add error checking for unexpected EOF */

    if (!f) {
        mii_error("Couldn't open %s for reading: %s", path, strerror(errno));
        return -1;
    }

    /* verify magic bytes */
    unsigned char magic_seq[sizeof MII_MODTABLE_MAGIC_BYTES]; /* sneaky array width copy ;) */

    fread(magic_seq, sizeof magic_seq, 1, f);

    if (memcmp(magic_seq, MII_MODTABLE_MAGIC_BYTES, sizeof magic_seq)) {
        mii_error("Couldn't parse from %s: bad magic sequence", path);
        fclose(f);
        return -1;
    }

    /* try and bring in some modules */
    int num_modules;
    fread(&num_modules, sizeof num_modules, 1, f);

    for (int i = 0; i < num_modules; ++i) {
        /* read module path */
        int mod_path_size;
        fread(&mod_path_size, sizeof mod_path_size, 1, f);

        char* mod_path = malloc(mod_path_size + 1);
        fread(mod_path, 1, mod_path_size, f);
        mod_path[mod_path_size] = 0;

        /* read module code */
        int mod_code_size;
        fread(&mod_code_size, sizeof mod_code_size, 1, f);

        char* mod_code = malloc(mod_code_size + 1);
        fread(mod_code, 1, mod_code_size, f);
        mod_code[mod_code_size] = 0;

        /* read module timestamp */
        time_t mod_timestamp;
        fread(&mod_timestamp, sizeof mod_timestamp, 1, f);

        /* read module bins */
        int mod_num_bins;
        fread(&mod_num_bins, sizeof mod_num_bins, 1, f);

        char** mod_bins = NULL;
        if (mod_num_bins) mod_bins = malloc(mod_num_bins * sizeof *mod_bins);

        for (int j = 0; j < mod_num_bins; ++j) {
            /* grab bin size */
            int bin_size;
            fread(&bin_size, sizeof bin_size, 1, f);

            /* allocate and read bin data */
            mod_bins[j] = malloc(bin_size + 1);
            fread(mod_bins[j], 1, bin_size, f);

            /* null-terminate bin string */
            mod_bins[j][bin_size] = 0;
        }

        /* that's all we need! call the handler */
        if ((res = handler(p, mod_path, mod_code, mod_bins, mod_num_bins, mod_timestamp))) {
            break;
        }
    }

    fclose(f);
    return res;
}

int _mii_modtable_parse_handler_import(mii_modtable* p, char* path, char* code, char** bins, int num_bins, time_t timestamp) {
    /* import: just insert every module into the modtable and don't worry too much */
    int target_index = _mii_modtable_get_target_index(path);

    mii_modtable_entry* new_entry = malloc(sizeof *new_entry);

    new_entry->path = path;
    new_entry->code = code;
    new_entry->bins = bins;
    new_entry->num_bins = num_bins;
    new_entry->timestamp = timestamp;
    new_entry->next = p->buf[target_index];
    new_entry->analysis_complete = 1;

    p->buf[target_index] = new_entry;
    ++p->num_modules;

    mii_debug("Imported module: path %s, code %s, %d bins", path, code, num_bins);
    return 0;
}

int _mii_modtable_parse_handler_preanalysis(mii_modtable* p, char* path, char* code, char** bins, int num_bins, time_t timestamp) {
    /* preanalysis phase
     * locate any matching modules and check if they are up to date.
     * if so, then prefill the binary list.
     * otherwise, leave it NULL so it gets regenerated in analysis. */

    mii_modtable_entry* mod = _mii_modtable_locate_entry(p, path);

    if (mod && (mod->timestamp <= timestamp)) {
        /* found a matching module, and the timestamp in the db is up to date.
         * pass over the bins */

        mod->bins = bins;
        mod->num_bins = num_bins;
        mod->analysis_complete = 1;

        --p->modules_requiring_analysis;
    } else {
        /* didn't find anything. free the bins */
        free(bins);
    }

    /* free everything else too */
    free(path);
    free(code);

    return 0;
}

/*
 * compute the hash index for a path, modulo the hash table width
 */
int _mii_modtable_get_target_index(const char* path) {
    return XXH32(path, strlen(path), 0) % MII_MODTABLE_HASHTABLE_WIDTH;
}

/* 
 * locate an existing entry in the hashtable.
 * returns NULL if not found
 */
mii_modtable_entry* _mii_modtable_locate_entry(mii_modtable* p, const char* path) {
    int ind = _mii_modtable_get_target_index(path);
    mii_modtable_entry* cur = p->buf[ind];

    while (cur) {
        if (!strcmp(cur->path, path)) return cur;
        cur = cur->next;
    }

    return NULL;
}
