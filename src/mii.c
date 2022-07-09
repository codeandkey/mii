#define _POSIX_C_SOURCE 200809L

#include "mii.h"
#include "util.h"
#include "analysis.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include <sys/stat.h>
#include <unistd.h>

/* options */
static char* _mii_modulepath = NULL;
static char* _mii_datadir    = NULL;

/* state */
static char* _mii_datafile = NULL;

void mii_option_modulepath(const char* modulepath) {
    if (modulepath) _mii_modulepath = mii_strdup(modulepath);
}

void mii_option_datadir(const char* datadir) {
    if (datadir) _mii_datadir = mii_strdup(datadir);
}

int mii_init() {
    if (!_mii_modulepath) {
        char* env_modulepath = getenv("MODULEPATH");

        if (env_modulepath) {
            _mii_modulepath = mii_strdup(env_modulepath);
        } else {
#ifndef NDEBUG
            fprintf(stderr, "warning: MODULEPATH is not set!");
#endif
            return -1;
        }
    }

    /* -d option has priority */
    if (!_mii_datadir)
    {
        char *index_file = getenv("MII_INDEX_FILE");

        if (index_file)
        {
            char* index_file_cpy = mii_strdup(index_file);
            char* mii_index_dir = dirname(index_file_cpy);

            /* create parent dir */
            int res = mii_recursive_mkdir(mii_index_dir, 0755);

            free(index_file_cpy);

            /* couldn't create index dir */
            if (res) {
                return -1;
            }

            _mii_datafile = mii_strdup(index_file);
        }

        char* home = getenv("HOME");

        if (!home) {
            fprintf(stderr, "ERROR: can't compute data dir without $HOME set\n");
            return -1;
        }

        _mii_datadir = mii_join_path(home, ".mii");
    }

    int res = mkdir(_mii_datadir, 0755);

    if (res && (errno != EEXIST)) {
        fprintf(stderr, "ERROR: datadir init failed: mkdir: %s\n", strerror(errno));
        return -1;
    }

    if (!_mii_datafile) 
    {
        _mii_datafile = mii_join_path(_mii_datadir, "index");
    }

    #ifndef NDEBUG
    fprintf(stderr, "Initialized mii with cache path %s", _mii_datafile);
    #endif
    return 0;
}

void mii_free() {
    if (_mii_modulepath) free(_mii_modulepath);
    if (_mii_datadir) free(_mii_datadir);
    if (_mii_datafile) free(_mii_datafile);
}

int mii_build() {
    /*
     * BUILD: rebuild the index from the modules on the disk
     * this is equivalent to a sync, but without the import/merge step
     */

    mii_modtable index;
    mii_modtable_init(&index);
    int count;

#if MII_ENABLE_SPIDER
    if (mii_modtable_spider_gen(&index, _mii_modulepath, &count)) {
        fprintf(stderr, "ERROR: spider generation failed\n");
        return -1;
    }
#else
    /* initialize analysis regular expressions */
    if (mii_analysis_init()) {
        fprintf(stderr, "ERROR: analysis init failed\n");
        return -1;
    }

    /* generate a partial index from the disk */
    if (mii_modtable_gen(&index, _mii_modulepath)) {
        fprintf(stderr, "ERROR: index generation failed\n");
        return -1;
    }

    /* perform analysis over the entire index */
    if (mii_modtable_analysis(&index, &count)) {
        fprintf(stderr, "ERROR: index analysis failed\n");
        return -1;
    }

#endif

    if (count) {
        fprintf(stderr, "Finished analysis on %d modules\n", count);
    } else {
        fprintf(stderr, "Didn't analyze any modules. Is the MODULEPATH correct?\n");
    }

    /* export back to the disk */
    if (mii_modtable_export(&index, _mii_datafile)) {
        fprintf(stderr, "ERROR: index write failed\n");
        return -1;
    }

    /* cleanup */
    mii_modtable_free(&index);

#if !MII_ENABLE_SPIDER
    mii_analysis_free();
#endif

    return 0;
}

int mii_sync() {
    /*
     * SYNC: synchronize the index if necessary
     */

    mii_modtable index;
    mii_modtable_init(&index);

    /* initialize analysis regular expressions */
    if (mii_analysis_init()) {
        fprintf(stderr, "ERROR: unexpected analysis failure\n");
        return -1;
    }

    /* generate a partial index from the disk */
    if (mii_modtable_gen(&index, _mii_modulepath)) {
        fprintf(stderr, "ERROR: index generation failed\n");
        return -1;
    }

    /* try and import up-to-date modules from the cache */
    if (mii_modtable_preanalysis(&index, _mii_datafile)) {
        fprintf(stderr, "WARNING: index preanalysis failed, rebuilding the index..\n");
    }

    /* perform analysis over any remaining modules */
    int count;

    if (mii_modtable_analysis(&index, &count)) {
        fprintf(stderr, "ERROR: index analysis failed\n");
        return -1;
    }


    /* export back to the disk only if modules were analyzed */
    if (count) {
        printf("Finished analysis on %d modules\n", count);

        if (mii_modtable_export(&index, _mii_datafile)) {
            fprintf(stderr, "ERROR: index write failed\n");
            return -1;
        }
    } else {
        printf("All modules up to date :)\n");
    }

    /* cleanup */
    mii_modtable_free(&index);
    mii_analysis_free();

    return 0;
}

int mii_search_exact(mii_search_result* res, const char* cmd) {
    mii_modtable index;
    mii_modtable_init(&index);

    /* try and import the cache from the disk */
    if (mii_modtable_import(&index, _mii_datafile)) {
        fprintf(stderr, "Index import failed, rebuilding..\n");

        if (mii_build()) return -1;

        fprintf(stderr, "Trying to import new index..\n");

        if (mii_modtable_import(&index, _mii_datafile)) {
            fprintf(stderr, "ERROR: failed to import again, giving up..\n");
            return -1;
        }
    }

    /* perform the search */
    if (mii_modtable_search_exact(&index, cmd, res)) {
        fprintf(stderr, "ERROR: search failed\n");
        return -1;
    }

    /* cleanup */
    mii_modtable_free(&index);

    return 0;
}

int mii_search_fuzzy(mii_search_result* res, const char* cmd) {
    mii_modtable index;
    mii_modtable_init(&index);

    /* try and import the cache from the disk */
    if (mii_modtable_import(&index, _mii_datafile)) {
        fprintf(stderr, "Index import failed, rebuilding..\n");

        if (mii_build()) return -1;

        fprintf(stderr, "Trying to import new index..\n");

        if (mii_modtable_import(&index, _mii_datafile)) {
            fprintf(stderr, "ERROR: failed to import again, giving up..\n");
            return -1;
        }
    }

    /* perform the search */
    if (mii_modtable_search_similar(&index, cmd, res)) {
        fprintf(stderr, "ERROR: search failed\n");
        return -1;
    }

    /* cleanup */
    mii_modtable_free(&index);

    return 0;
}

int mii_search_info(mii_search_result* res, const char* cmd) {
    mii_modtable index;
    mii_modtable_init(&index);

    /* try and import the cache from the disk */
    if (mii_modtable_import(&index, _mii_datafile)) {
        fprintf(stderr, "Index import failed, rebuilding..\n");

        if (mii_build()) return -1;

        fprintf(stderr, "Trying to import new index..\n");

        if (mii_modtable_import(&index, _mii_datafile)) {
            fprintf(stderr, "ERROR: failed to import again, giving up..\n");
            return -1;
        }
    }

    /* perform the search */
    if (mii_modtable_search_info(&index, cmd, res)) {
        fprintf(stderr, "ERROR: search failed\n");
        return -1;
    }

    /* cleanup */
    mii_modtable_free(&index);

    return 0;
}

int mii_list() {
    mii_modtable index;
    mii_modtable_init(&index);

    /* try and import the cache from the disk */
    if (mii_modtable_import(&index, _mii_datafile)) {
        fprintf(stderr, "Index import failed, rebuilding..\n");

        if (mii_build()) return -1;

        fprintf(stderr, "Trying to import new index..\n");

        if (mii_modtable_import(&index, _mii_datafile)) {
            fprintf(stderr, "ERROR: failed to import again, giving up..\n");
            return -1;
        }
    }

    int should_color = isatty(fileno(stdout));
    int code_width, count = 0;

    /* compute code column width if we're pretty printing */
    if (should_color) {
        code_width = 0;

        for (int i = 0; i < MII_MODTABLE_HASHTABLE_WIDTH; ++i) {
            for (mii_modtable_entry* cur = index.buf[i]; cur; cur = cur->next) {
                int len = strlen(cur->code);
                if (len > code_width) code_width = len;
                ++count;
            }
        }

        printf("\033[0;39mIndexed modules (total %d):\n", count);
    }

    for (int i = 0; i < MII_MODTABLE_HASHTABLE_WIDTH; ++i) {
        for (mii_modtable_entry* cur = index.buf[i]; cur; cur = cur->next) {
            if (should_color) {
                printf("    \033[0;39m%-*s    \033[2;37m%s\n", code_width, cur->code, cur->path);
            } else {
                printf("%s\n", cur->code);
            }
        }
    }

    if (should_color) printf("\033[0;39m");

    mii_modtable_free(&index);
    return 0;
}

int mii_enable() {
    char* disable_path = mii_join_path(_mii_datadir, "disabled");

    if (remove(disable_path)) {
        fprintf(stderr, "Couldn't enable mii, perhaps it is already enabled? (%s)\n", strerror(errno));
    } else {
        fprintf(stderr, "Re-enabled shell integration!\n");
    }

    free(disable_path);

    return 0;
}

int mii_disable() {
    char* disable_path = mii_join_path(_mii_datadir, "disabled");
    FILE* disable_fd = fopen(disable_path, "wb");


    if (!disable_fd) {
        fprintf(stderr, "ERROR: couldn't write disable lock: %s\n", strerror(errno));
        free(disable_path);
        return -1;
    }

    fclose(disable_fd);
    fprintf(stderr, "Disabled shell integration!\n");

    free(disable_path);

    return 0;
}

int mii_status() {
    char* disable_path;
    FILE* disable_file;

    disable_path = mii_join_path(_mii_datadir, "disabled");

    printf("mii %s\nstatus: ", MII_VERSION);

    if ((disable_file = fopen(disable_path, "r"))) {
        fclose(disable_file);
        printf("disabled\n");
    } else {
        printf("enabled\n");
    }

    free(disable_path);
    return 0;
}
