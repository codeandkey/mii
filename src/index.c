#define _POSIX_C_SOURCE 200809L

#include "index.h"
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

static int _mii_index_crawl_sub(mii_index* p, const char* prefix);

int mii_index_init(mii_index* p, const char* root) {
    memset(p, 0, sizeof *p);
    p->root = mii_strdup(root);

    return 0;
}

void mii_index_free(mii_index* p) {
}

int mii_index_crawl(mii_index* p) {
    return _mii_index_crawl_sub(p, NULL);
}

int _mii_index_crawl_sub(mii_index* p, const char* prefix) {
    /* crawl through the disk from the root and build a minimal tree. */
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
            int mod_type = MII_MODULE_TYPE_TCL; /* assume tcl unless we detect lmod */

            if (strlen(rel_path) > 4 && !strcmp(rel_path + rel_len - 4, ".lua")) {
                /* mutate the rel_path in place for the code, we don't need it anymore anyway */
                rel_path[rel_len - 4] = 0;
                mod_type = MII_MODULE_TYPE_LMOD;
            }

            mii_debug("Found module %s at %s", rel_path, abs_path);

            /* insert the new module in */
            mii_module* new_module = malloc(sizeof *new_module);

            new_module->path = abs_path;
            new_module->code = rel_path; /* rel_path was mutated to become the code */
            new_module->type = mod_type;
            new_module->timestamp = st.st_mtime;
            new_module->bins = NULL;
            new_module->num_bins = 0;
            new_module->analysis_complete = 0;

            int target_index = mii_index_hash(abs_path);

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

int mii_index_hash(const char* path) {
    return XXH32(path, strlen(path), 0) % MII_INDEX_WIDTH;
}
