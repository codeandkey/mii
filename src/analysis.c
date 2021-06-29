#define _POSIX_C_SOURCE 200809L

#include "analysis.h"
#include "modtable.h"
#include "util.h"
#include "log.h"

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <dirent.h>
#include <errno.h>
#include <regex.h>
#include <unistd.h>
#include <sys/stat.h>
#include <wordexp.h>

static lua_State *L;

static const char* _mii_analysis_lmod_regex_src =
    "\\s*(prepend_path|append_path)\\s*\\(\\s*"
    "\"PATH\"\\s*,\\s*\"([^\"]+)\"";

static regex_t _mii_analysis_lmod_regex;

/* word expansion functions */
char* _mii_analysis_expand(const char* expr);

/* module type analysis functions */
int _mii_analysis_lmod(const char* path, char*** bins_out, int* num_bins_out);
int _mii_analysis_tcl(const char* path, char*** bins_out, int* num_bins_out);

/* path scanning functions */
int _mii_analysis_scan_path(char* path, char*** bins_out, int* num_bins_out);

/*
 * compile regexes
 */
int mii_analysis_init() {
    if (regcomp(&_mii_analysis_lmod_regex, _mii_analysis_lmod_regex_src, REG_EXTENDED | REG_NEWLINE)) {
        mii_error("failed to compile Lmod analysis regex");
        return -1;
    }

    L = luaL_newstate();
    luaL_openlibs(L);

    char buf[PATH_MAX];
    char *res = realpath("./share/mii/lua/sandbox.lua", buf);
    if (!res || luaL_dofile(L, res)) {
        mii_error("failed to load Lua helper files");
        lua_close(L);
        return -1;
    }

    return 0;
}

/*
 * clean up regexes
 */
void mii_analysis_free() {
    regfree(&_mii_analysis_lmod_regex);
    lua_close(L);
}

/*
 * run analysis for an arbitrary module
 */
int mii_analysis_run(const char* modfile, int modtype, char*** bins_out, int* num_bins_out) {
    switch (modtype) {
    case MII_MODTABLE_MODTYPE_LMOD:
        return _mii_analysis_lmod(modfile, bins_out, num_bins_out);
    case MII_MODTABLE_MODTYPE_TCL:
        return _mii_analysis_tcl(modfile, bins_out, num_bins_out);
    }

    return 0;
}

const char* _mii_lua_run(lua_State* L, const char* code) {
    lua_getglobal(L, "sandbox_run");
    lua_pushstring(L, code);
    lua_call(L, 1, 1);
    return lua_tostring(L, -1);
}

/*
 * extract paths from an lmod file
 */
int _mii_analysis_lmod(const char* path, char*** bins_out, int* num_bins_out) {
    char* buffer;
    FILE* f = fopen(path, "r");

    if (!f) {
        mii_error("Couldn't open %s for reading : %s", path, strerror(errno));
        return -1;
    }

    fseek(f, 0L, SEEK_END);
    long s = ftell(f);
    rewind(f);
    buffer = malloc(s + 1);
    if ( buffer != NULL ) {
        fread(buffer, s, 1, f);
        fclose(f); f = NULL;
        buffer[s] = '\0';

        /* get paths that are added to PATH */
        char* bin_paths = mii_strdup(_mii_lua_run(L, buffer));
        char* bin_path = strtok(bin_paths, ":");
        while(bin_path != NULL) {
            _mii_analysis_scan_path(bin_path, bins_out, num_bins_out);
            bin_path = strtok(NULL, ":");
        }

        free(bin_paths);
        free(buffer);
    }
    if (f != NULL) fclose(f);

    return 0;
}

/*
 * extract paths from a tcl file
 */
int _mii_analysis_tcl(const char* path, char*** bins_out, int* num_bins_out) {
    char linebuf[MII_ANALYSIS_LINEBUF_SIZE];

    FILE* f = fopen(path, "r");

    if (!f) {
        mii_error("Couldn't open %s for reading : %s", path, strerror(errno));
        return -1;
    }

    char* cmd, *key, *val, *expanded;

    while (fgets(linebuf, sizeof linebuf, f)) {
        /* strip off newline */
        int len = strlen(linebuf);
        if (linebuf[len - 1] == '\n') linebuf[len - 1] = 0;

        if (!(cmd = strtok(linebuf, " \t"))) continue;

        if (*cmd == '#') continue; /* skip comments */

        if (!strcmp(cmd, "set")) {
            if (!(key = strtok(NULL, " \t"))) continue;
            if (!(val = strtok(NULL, " \t"))) continue;
            if (!(expanded = _mii_analysis_expand(val))) continue;

            setenv(key, expanded, 1);
            free(expanded);
        } else if (!strcmp(cmd, "prepend-path") || !strcmp(cmd, "append-path")) {
            if (!(key = strtok(NULL, " \t"))) continue;
            if (strcmp(key, "PATH")) continue;

            if (!(val = strtok(NULL, " \t"))) continue;
            if (!(expanded = _mii_analysis_expand(val))) continue;

            _mii_analysis_scan_path(expanded, bins_out, num_bins_out);
            free(expanded);
        }
    }

    fclose(f);
    return 0;
}

/*
 * scan a path for commands
 */
int _mii_analysis_scan_path(char* path, char*** bins_out, int* num_bins_out) {
    /* paths might contain multiple in one (seperated by ':'),
     * break them up here */

    DIR* d;
    struct dirent* dp;
    struct stat st;

    for (const char* cur_path = strtok(path, ":"); cur_path; cur_path = strtok(NULL, ":")) {
        mii_debug("scanning PATH %s", cur_path);

        /* TODO: this could be faster, do some benchmarking to see if it's actually slow */
        if (!(d = opendir(cur_path))) {
            mii_debug("Failed to open %s, ignoring : %s", cur_path, strerror(errno));
            continue;
        }

        while ((dp = readdir(d))) {
            if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) continue;

            char* abs_path = mii_join_path(cur_path, dp->d_name);

            if (!stat(abs_path, &st)) {
                /* check the file is executable by the user */
                if ((S_ISREG(st.st_mode) || S_ISLNK(st.st_mode)) && !access(abs_path, X_OK)) {
                    /* found a binary! append it to the list */
                    ++*num_bins_out;
                    *bins_out = realloc(*bins_out, *num_bins_out * sizeof **bins_out);
                    (*bins_out)[*num_bins_out - 1] = mii_strdup(dp->d_name);
                }
            } else {
                mii_warn("Couldn't stat %s : %s", abs_path, strerror(errno));
            }

            free(abs_path);
        }

        closedir(d);
    }

    return 0;
}

char* _mii_analysis_expand(const char* expr) {
    wordexp_t w;

    if (wordexp(expr, &w, WRDE_NOCMD)) {
        /* expansion failed. die quietly */
        mii_debug("Expansion failed on string \"%s\"!", expr);
        return NULL;
    }

    char* output = NULL;
    int len = 0;

    for (int i = 0; i < w.we_wordc; ++i) {
        int wsize = strlen(w.we_wordv[i]);
        len += wsize;
        output = realloc(output, len + 1);
        memcpy(output + len - wsize, w.we_wordv[i], len + 1);
    }

    wordfree(&w);
    return output;
}
