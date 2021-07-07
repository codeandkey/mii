#define _POSIX_C_SOURCE 200809L

#include "analysis.h"
#include "modtable.h"
#include "util.h"
#include "log.h"

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

/* lua interpreter state */
static lua_State *lua_state;

/* number of paths to scan for a given modulefile */
static int num_paths;

/* word expansion functions */
char* _mii_analysis_expand(const char* expr);

/* module type analysis functions */
int _mii_analysis_lmod(const char* path, char*** bins_out, int* num_bins_out);
int _mii_analysis_tcl(const char* path, char*** bins_out, int* num_bins_out);

/* path scanning functions */
int _mii_analysis_scan_path(char* path, char*** bins_out, int* num_bins_out);

/*
 * initialize lua interpreter
 */
int mii_analysis_init() {
    lua_state = luaL_newstate();
    luaL_openlibs(lua_state);

    /* sandbox path when mii is installed */
    char* lua_path = mii_join_path(MII_PREFIX, "share/mii/lua/sandbox.luac");

    if(access(lua_path, F_OK) == 0) {
        /* found file, execute it and return */
        luaL_dofile(lua_state, lua_path);
        free(lua_path);
        return 0;
    }
    free(lua_path);

    lua_path = "./sandbox.luac";
    if(access(lua_path, F_OK) == 0) {
        /* mii is not installed, but should work anyway */
        luaL_dofile(lua_state, lua_path);
        return 0;
    }

    mii_error("failed to load Lua file");
    lua_close(lua_state);

    return -1;
}

/*
 * close lua interpreter
 */
void mii_analysis_free() {
    lua_close(lua_state);
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

/*
 * run a modulefile's code in a lua sandbox
 */
char** _mii_analysis_lua_run(lua_State* lua_state, const char* code) {
    /* execute modulefile */
    lua_getglobal(lua_state, "sandbox_run");
    lua_pushstring(lua_state, code);
    lua_call(lua_state, 1, 1);

    /* allocate memory for the paths */
    luaL_checktype(lua_state, 1, LUA_TTABLE);
    num_paths = lua_objlen(lua_state, -1);
    char** paths = malloc(sizeof (char*) * num_paths);

    /* retrieve paths from lua stack */
    for (int i = 1; i <= num_paths; ++i) {
        lua_rawgeti(lua_state, -i, i);
        paths[i-1] = mii_strdup(lua_tostring(lua_state, -1));
    }
    lua_pop(lua_state, 1);

    return paths;
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

        /* get paths and analyze */
        char** bin_paths = _mii_analysis_lua_run(lua_state, buffer);
        for(int i = 0; i < num_paths; ++i) {
            _mii_analysis_scan_path(bin_paths[i], bins_out, num_bins_out);
            free(bin_paths[i]);
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
