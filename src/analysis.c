#define _POSIX_C_SOURCE 200809L

#include "analysis.h"
#include "modtable.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if MII_ENABLE_LUA
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

/* After Lua 5.1, lua_objlen changed name and LUA_OK is defined */
#if LUA_VERSION_NUM <= 501
#define mii_lua_len lua_objlen
#define LUA_OK 0
#else
#define mii_lua_len lua_rawlen
#endif

#endif // MII_ENABLE_LUA

#if MII_ENABLE_SPIDER
#include "cjson/cJSON.h"
#endif

#include <dirent.h>
#include <errno.h>
#include <regex.h>
#include <unistd.h>
#include <sys/stat.h>
#include <wordexp.h>

#if !MII_ENABLE_LUA
static const char* _mii_analysis_lmod_regex_src =
    "\\s*(prepend_path|append_path)\\s*\\(\\s*"
    "\"PATH\"\\s*,\\s*\"([^\"]+)\"";

static regex_t _mii_analysis_lmod_regex;
#else
/* Lua interpreter state */
static lua_State *lua_state;

/* run lua module code in a sandbox */
int _mii_analysis_lua_run(lua_State* lua_state, const char* code, char*** paths_out, int* num_paths_out);
#endif

/* word expansion functions */
char* _mii_analysis_expand(const char* expr);

/* module type analysis functions */
int _mii_analysis_lmod(const char* path, char*** bins_out, int* num_bins_out);
int _mii_analysis_tcl(const char* path, char*** bins_out, int* num_bins_out);

/* path scanning functions */
int _mii_analysis_scan_path(char* path, char*** bins_out, int* num_bins_out);

#if MII_ENABLE_SPIDER
int _mii_analysis_parents_from_json(const cJSON* json, char*** parents_out, int* num_parents_out);
#endif

#if !MII_ENABLE_LUA
/*
 * compile regexes
 */
int mii_analysis_init() {
    if (regcomp(&_mii_analysis_lmod_regex, _mii_analysis_lmod_regex_src, REG_EXTENDED | REG_NEWLINE)) {
        fprintf(stderr, "ERROR: LMOD regex failure\n");
        return -1;
    }

    return 0;
}
#else
/*
 * initialize Lua interpreter
 */
int mii_analysis_init() {
    lua_state = luaL_newstate();
    luaL_openlibs(lua_state);

    /* sandbox path when mii is installed */
    char* lua_path = mii_join_path(MII_PREFIX, "share/mii/lua/sandbox.luac");

    if(access(lua_path, F_OK) == 0) {
        /* found file, try to execute it and return */
        if(luaL_dofile(lua_state, lua_path) == LUA_OK) {
            free(lua_path);
            return 0;
        }
    }
    free(lua_path);

    lua_path = "./sandbox.luac";
    if(access(lua_path, F_OK) == 0) {
        /* mii is not installed, but should work anyway */
        if(luaL_dofile(lua_state, lua_path) == LUA_OK)
            return 0;
    }

    fprintf(stderr, "failed to load Lua file");
    lua_close(lua_state);

    return -1;
}
#endif

/*
 * cleanup regexes or Lua interpreter
 */
void mii_analysis_free() {
#if !MII_ENABLE_LUA
    regfree(&_mii_analysis_lmod_regex);
#else
    lua_close(lua_state);
#endif
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

#if MII_ENABLE_LUA
/*
 * run a modulefile's code in a Lua sandbox
 */
int _mii_analysis_lua_run(lua_State* lua_state, const char* code, char*** paths_out, int* num_paths_out) {
    /* execute modulefile */
    int res;
    lua_getglobal(lua_state, "sandbox_run");
    lua_pushstring(lua_state, code);
    res = lua_pcall(lua_state, 1, 1, 0);

    if(res != LUA_OK) {
        fprintf(stderr, "ERROR: Lua: %s", lua_tostring(lua_state, -1));
        lua_pop(lua_state, 1);
        return -1;
    }

    luaL_checktype(lua_state, 1, LUA_TTABLE);

    /* allocate memory for the paths */
    *num_paths_out = mii_lua_len(lua_state, -1);
    *paths_out = malloc(*num_paths_out * sizeof(char*));

    /* retrieve paths from Lua stack */
    for (int i = 1; i <= *num_paths_out; ++i) {
        lua_rawgeti(lua_state, -i, i);
        (*paths_out)[i-1] = mii_strdup(lua_tostring(lua_state, -1));
    }

    /* pop every rawgeti + table */
    lua_pop(lua_state, *num_paths_out + 1);

    return 0;
}
#endif

/*
 * extract paths from an lmod file
 */
int _mii_analysis_lmod(const char* path, char*** bins_out, int* num_bins_out) {
    FILE* f = fopen(path, "r");

    if (!f) {
        perror(path);
        return -1;
    }

#if !MII_ENABLE_LUA
    char linebuf[MII_ANALYSIS_LINEBUF_SIZE];
    regmatch_t matches[3];

    while (fgets(linebuf, sizeof linebuf, f)) {
        /* strip off newline */
        int len = strlen(linebuf);
        if (linebuf[len - 1] == '\n') linebuf[len - 1] = 0;

        /* execute regex */
        if (!regexec(&_mii_analysis_lmod_regex, linebuf, 3, matches, 0)) {
            if (matches[2].rm_so < 0) continue;
            linebuf[matches[2].rm_eo] = 0;

            _mii_analysis_scan_path(linebuf + matches[2].rm_so, bins_out, num_bins_out);
        }
    }

    fclose(f);
#else
    char* buffer;
    fseek(f, 0L, SEEK_END);
    long s = ftell(f);
    rewind(f);
    buffer = malloc(s + 1);
    if ( buffer != NULL ) {
        fread(buffer, s, 1, f);
        fclose(f); f = NULL;
        buffer[s] = '\0';

        /* get binaries paths */
        char** bin_paths;
        int num_paths;
        if(_mii_analysis_lua_run(lua_state, buffer, &bin_paths, &num_paths)) {
            fprintf(stderr, "ERROR: couldn't execute %s", path);
            free(buffer);
            return -1;
        }

        /* scan every path returned */
        for(int i = 0; i < num_paths; ++i) {
            _mii_analysis_scan_path(bin_paths[i], bins_out, num_bins_out);
            free(bin_paths[i]);
        }

        free(bin_paths);
        free(buffer);
    }
    if (f != NULL) fclose(f);
#endif

    return 0;
}

/*
 * extract paths from a tcl file
 */
int _mii_analysis_tcl(const char* path, char*** bins_out, int* num_bins_out) {
    char linebuf[MII_ANALYSIS_LINEBUF_SIZE];

    FILE* f = fopen(path, "r");

    if (!f) {
        perror(path);
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
    /* paths might contain multiple in one (separated by ':'),
     * break them up here */

    DIR* d;
    struct dirent* dp;
    struct stat st;

    for (const char* cur_path = strtok(path, ":"); cur_path; cur_path = strtok(NULL, ":")) {
        /* TODO: this could be faster, do some benchmarking to see if it's actually slow */

        if (!(d = opendir(cur_path))) {
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
                perror(abs_path);
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
        perror("wordexp");
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

#if MII_ENABLE_SPIDER

/* parse the json and fill module info */
int mii_analysis_parse_module_json(const cJSON* mod_json, mii_modtable_entry* mod) {
    /* stat the type */
    struct stat st;
    if (stat(mod_json->string, &st) != 0) {
        perror(mod_json->string);
        return -1;
    }

    /* get the code */
    cJSON* code = cJSON_GetObjectItemCaseSensitive(mod_json, "fullName");
    if (code == NULL) {
        fprintf(stderr, "ERROR: couldn't find the code in the JSON!\n");
        return -1;
    }

    /* get the parents */
    cJSON* parents_arrs = cJSON_GetObjectItemCaseSensitive(mod_json, "parentAA");
    if(_mii_analysis_parents_from_json(parents_arrs, &mod->parents, &mod->num_parents)) {
        fprintf(stderr, "ERROR: couldn't get parents from the JSON!\n");
        return -1;
    }

    /* fill up some of the info */
    mod->bins = NULL;
    mod->num_bins = 0;
    mod->path = mii_strdup(mod_json->string);
    mod->type = MII_MODTABLE_MODTYPE_LMOD;
    mod->timestamp = st.st_mtime;
    mod->code = mii_strdup(code->valuestring);
    mod->analysis_complete = 1;

    /* get the bins */
    cJSON* bin_paths = cJSON_GetObjectItemCaseSensitive(mod_json, "pathA");
    if (bin_paths != NULL) {
        for (cJSON* path = bin_paths->child; path != NULL; path = path->next) {
            /* analyze the bin paths */
            _mii_analysis_scan_path(path->string, &mod->bins, &mod->num_bins);
        }
    }

    return 0;
}

/* get the parents from a json array (can be NULL) */
int _mii_analysis_parents_from_json(const cJSON* json, char*** parents_out, int* num_parents_out) {
    /* if NULL, nothing to do */
    if (json == NULL) {
        *parents_out = NULL;
        *num_parents_out = 0;
        return 0;
    }

    /* allocate memory for the parents */
    *num_parents_out = cJSON_GetArraySize(json);
    *parents_out = malloc(*num_parents_out * sizeof(char*));

    if (*parents_out == NULL) {
        perror("malloc");
        return -1;
    }

    size_t i = 0;
    for (cJSON* parents = json->child; parents != NULL; parents = parents->next) {
        char* codes_tmp = NULL;
        size_t parent_len, codes_size = 0;

        for (cJSON* parent = parents->child; parent != NULL; parent = parent->next) {
            /* allocate memory new code */
            parent_len = strlen(parent->valuestring);
            codes_tmp = (char*) realloc(codes_tmp, codes_size + parent_len + 1);

            if (codes_tmp == NULL) {
                perror("malloc");
                free(*parents_out);
                return -1;
            }

            /* add new code */
            if (codes_size != 0) strcpy(codes_tmp + codes_size - 1, " ");
            strcpy(codes_tmp + codes_size, parent->valuestring);

            codes_size += parent_len + 1;
        }
        (*parents_out)[i++] = codes_tmp;
    }

    return 0;
}

#endif
