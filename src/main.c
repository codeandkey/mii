#define _POSIX_C_SOURCE 200809L

#include "mii.h"
#include "log.h"
#include "util.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <libgen.h>
#include <unistd.h>

static const char* USAGE_STRING =
    "USAGE: %s [FLAGS] [OPTIONS] <SUBCOMMAND>\n\n"
    "FLAGS:\n"
    "    -j, --json       Output results in JSON encoding\n"
    "    -h, --help       Show this message\n"
    "    -v, --version    Show Mii build version\n"
    "\nOPTIONS:\n"
    "    -d, --datadir <datadir>    Use <datadir> to store index data\n"
    "    -m, --modulepath <path>    Use <path> instead of $MODULEPATH\n"
    "\nSUBCOMMANDS:\n"
    "    build               Regenerate the module index\n"
    "    sync                Update the module index\n"
    "    exact <command>     Find modules which provide <command>\n"
    "    search <command>    Search for commands similar to <command>\n"
    "    show <module>       Show commands provided by <module>\n"
    "    list                List all cached module files\n"
    "    install             Install mii into your shell\n"
    "    enable              Enable mii integration (default)\n"
    "    disable             Disable mii integration\n"
    "    status              Get database and integration status\n"
    "    version             Show Mii build version\n"
    "    help                Show this message\n";

static struct option long_options[] = {
    { "datadir",    required_argument, NULL, 'd' },
    { "modulepath", required_argument, NULL, 'm' },
    { "help",       no_argument,       NULL, 'h' },
    { "json",       no_argument,       NULL, 'j' },
    { "version",    no_argument,       NULL, 'v' },
    { NULL,         0,                 NULL,  0 },
};

static void usage(int header, char* a0);
static int install();
static void version();

int main(int argc, char** argv) {
    int opt;
    int search_result_flags = 0;

    while ((opt = getopt_long(argc, argv, "d:m:hjv", long_options, NULL)) != -1) {
        switch (opt) {
        case 'd': /* set datadir */
            mii_option_datadir(optarg);
            break;
        case 'm': /* set modulepath */
            mii_option_modulepath(optarg);
            break;
        case 'j':
            search_result_flags |= MII_SEARCH_RESULT_JSON;
            break;
        case 'v':
            version();
            return 0;
        case 'h': /* display usage */
            usage(1, *argv);
            return 0;
        }
    }

    /* check there is a subcommand */
    if (optind >= argc) {
        mii_error("Please specify a subcommand!");
        usage(0, *argv);
        return -1;
    }

    /* initialize mii */
    if (mii_init()) return -1;

    /* execute subcommand */
    if (!strcmp(argv[optind], "sync")) {
        if (mii_sync()) return -1;
    } else if (!strcmp(argv[optind], "build")) {
        if (mii_build()) return -1;
    } else if (!strcmp(argv[optind], "exact")) {
        /* check there is a second positional argument */
        if (++optind >= argc) {
            mii_error("exact: missing argument");
            usage(0, *argv);
            return -1;
        }

        /* perform the search */
        mii_search_result res;
        if (mii_search_exact(&res, argv[optind])) return -1;

        /* output the result and clean up */
        mii_search_result_write(&res, stdout, MII_SEARCH_RESULT_MODE_EXACT, search_result_flags);
        mii_search_result_free(&res);
    } else if (!strcmp(argv[optind], "search")) {
        /* check there is a second positional argument */
        if (++optind >= argc) {
            mii_error("search: missing argument");
            usage(0, *argv);
            return -1;
        }

        /* perform the search */
        mii_search_result res;
        if (mii_search_fuzzy(&res, argv[optind])) return -1;

        /* output the result and clean up */
        mii_search_result_write(&res, stdout, MII_SEARCH_RESULT_MODE_FUZZY, search_result_flags);
        mii_search_result_free(&res);
    } else if (!strcmp(argv[optind], "show")) {
        /* check there is a second positional argument */
        if (++optind >= argc) {
            mii_error("show: missing argument");
            usage(0, *argv);
            return -1;
        }

        /* perform the search */
        mii_search_result res;
        if (mii_search_info(&res, argv[optind])) return -1;

        /* output the result and clean up */
        mii_search_result_write(&res, stdout, MII_SEARCH_RESULT_MODE_SHOW, search_result_flags);
        mii_search_result_free(&res);
    } else if (!strcmp(argv[optind], "select")) {
        /* check there is a second positional argument */
        if (++optind >= argc) {
            mii_error("select: missing argument");
            usage(0, *argv);
            return -1;
        }

        char* cmd = argv[optind];
        int maximum = 3;

        /* optinal third argument limits the number of results */
        if (++optind < argc) {
            maximum = strtol(argv[optind], NULL, 10);
        }

        /* perform an exact search */
        mii_search_result res;
        if (mii_search_exact(&res, cmd)) return -1;

        /* see if we need colors */
        int select_colors = isatty(fileno(stderr));

        /* if there is one result, output it and stop */
        if (res.num_results == 1) {
            printf("%s %s\n", res.parents[0], res.codes[0]);
        } else if (res.num_results > 1) {
            /* prompt the user for a module */
            fprintf(stderr, "[mii] ");
            if (select_colors) fprintf(stderr, "\033[1;37m");
            fprintf(stderr, "Please select a module to run ");
            if (select_colors) fprintf(stderr, "\033[0;36m");
            fprintf(stderr, "%s", cmd);
            if (select_colors) fprintf(stderr, "\033[1;37m");
            fprintf(stderr, ":\n");

            /* compute the maximum code width */
            int max_codewidth = 8;

            for (int k = 0; k < res.num_results; ++k) {
                int clen = strlen(res.codes[k]);
                if (clen > max_codewidth) max_codewidth = clen;
            }

            if (select_colors) fprintf(stderr, "\033[1;37m");
            fprintf(stderr, "       %-*s", max_codewidth, "MODULE");
            fprintf(stderr, " %s\n", "PARENT(S)");
            if (select_colors) fprintf(stderr, "\033[0;39m");

            for (int i = 0; i < res.num_results; ++i) {
                if (select_colors) fprintf(stderr, "\033[2;39m");
                fprintf(stderr, "    %-2d", i + 1);
                if (select_colors) fprintf(stderr, "\033[0;39m");
                fprintf(stderr, " %-*s", max_codewidth, res.codes[i]);
                if (select_colors) fprintf(stderr, "\033[0;36m");
                fprintf(stderr, " %s\n", res.parents[i]);
                if (select_colors) fprintf(stderr, "\033[0;39m");
            }

            fprintf(stderr, "Make a selection (1-%d, q aborts) [1]: ", res.num_results);

            char line[4] = {0};

            if (!fgets(line, sizeof line, stdin)) {
                fprintf(stderr, "[mii] No selection made!\n");
                return 0;
            }

            /* default to first option */
            if (*line == '\n') *line = '1';

            for (int i = 0; i < sizeof line; ++i) {
                if (line[i] && !isdigit(line[i]) && line[i] != '\n') {
                    fprintf(stderr, "[mii] Aborted by user!\n");
                    return -1;
                }
            }

            int val = strtol(line, NULL, 10) - 1;

            if (val < 0 || val >= res.num_results) {
                fprintf(stderr, "[mii] Selection out of range! Aborting.\n");
                return -1;
            }

            /* finally output the chosen module */
            printf("%s %s\n", res.parents[val], res.codes[val]);
        } else {
            /* no results. we need to perform a fuzzy search now */
            mii_search_result_free(&res);
            if (mii_search_fuzzy(&res, cmd)) return -1;

            /* output the best 'maximum' values */
            if (res.num_results) {
                if (res.distances[0] == 0 || res.num_results == 1) {
                    /* user made a case mistake. recommend the right command */
                    if (select_colors) fprintf(stderr, "\033[0;39m");
                    fprintf(stderr, "[mii] Did you mean ");
                    if (select_colors) fprintf(stderr, "\033[0;36m");
                    fprintf(stderr, "\"%s\"", res.bins[0]);
                    if (select_colors) fprintf(stderr, "\033[0;39m");
                    fprintf(stderr, "? ");
                    if (select_colors) fprintf(stderr, "\033[2;37m");
                    fprintf(stderr, "(from %s)", res.codes[0]);
                    if (select_colors) fprintf(stderr, "\033[0;39m");
                    fprintf(stderr, "\n");
                } else {
                    /* no near-matches, so we can just recommened some almost similar (and unique) ones */
                    char** bins = NULL;
                    mii_search_result_get_unique_bins(&res, &bins, &maximum);
                    if (select_colors) fprintf(stderr, "\033[0;39m");
                    fprintf(stderr, "[mii] ");
                    if (select_colors) fprintf(stderr, "\033[0;36m");
                    fprintf(stderr, "%s", cmd);
                    if (select_colors) fprintf(stderr, "\033[0;39m");
                    fprintf(stderr, " not found! Similar commands: ");

                    for (int i = 0; i < maximum; ++i) {
                        if (select_colors) fprintf(stderr, "\033[0;39m");
                        if (i) fprintf(stderr, ", ");
                        if (select_colors) fprintf(stderr, "\033[0;36m");
                        fprintf(stderr, "\"%s\"", bins[i]);
                    }

                    free(bins);

                    if (select_colors) fprintf(stderr, "\033[0;39m");
                    fprintf(stderr, "\n");
                }
            } else {
                return 2; /* return code when we have nothing to give */
            }

            mii_search_result_free(&res);
            return -1; /* bad error code to indicate no module on stdout */
        }

        mii_search_result_free(&res);
    } else if (!strcmp(argv[optind], "list")) {
        if (mii_list()) return -1;
    } else if (!strcmp(argv[optind], "help")) {
        usage(1, *argv);
    } else if (!strcmp(argv[optind], "install")) {
        if (install()) return -1;
    } else if (!strcmp(argv[optind], "disable")) {
        if (mii_disable()) return -1;
    } else if (!strcmp(argv[optind], "enable")) {
        if (mii_enable()) return -1;
    } else if (!strcmp(argv[optind], "status")) {
        if (mii_status()) return -1;
    } else if (!strcmp(argv[optind], "version")) {
        version();
    } else {
        mii_error("Unrecognized subcommand \"%s\"!", argv[optind]);
    }

    /* cleanup */
    mii_free();

    return 0;
}

void usage(int header, char* a0) {
    if (header) {
        fprintf(stderr, "==========================================\n");
        fprintf(stderr, "==  mii %s -- module inverted index  ==\n", MII_VERSION);
        fprintf(stderr, "==========================================\n");
    }

    fprintf(stderr, "\n");
    fprintf(stderr, USAGE_STRING, a0);
}

/* output installation information */
int install() {
    char* env_mii = getenv("MII");
    
    if (env_mii) {
        mii_info("Mii is already enabled on your shell!");
        return 0;
    }

    char* env_shell = getenv("SHELL");
    const char* shellrc_suffix = NULL;

    if (!env_shell || !strlen(env_shell)) {
        mii_error("$SHELL is not set, cannot detect your shell!");
        return -1;
    }

    env_shell = basename(env_shell);

    if (!strcmp(env_shell, "bash")) {
        mii_info("Detected bash shell");
        shellrc_suffix = ".bashrc";
    } else if (!strcmp(env_shell, "zsh")) {
        mii_info("Detected zsh shell");
        shellrc_suffix = ".zshrc";
    } else {
        mii_error("Unsupported shell %s! Supported shells are 'bash', 'zsh'", env_shell);
        return -1;
    }

    printf("To enable mii for your shell, append the following to your .%src:\n", env_shell);
    printf("    source %s/share/mii/init/%s\n", MII_PREFIX, env_shell);

    char* env_home = getenv("HOME");
    if (!env_home) return 0; /* just die quietly if there's no HOME */

    char* shellrc = mii_join_path(env_home, shellrc_suffix);

    printf("Automatically write %s ? (y/N) ", shellrc);
    char resp = getchar();

    if (tolower(resp) == 'y') {
        /* try and append the line to the shell config */
        FILE* f = fopen(shellrc, "a");

        if (!f) {
            mii_error("Couldn't open shell config %s for writing: %s", shellrc, strerror(errno));
            return -1;
        }

        fprintf(f, "\n# enable mii integration for %s\n", env_shell);
        fprintf(f, "source %s/share/mii/init/%s\n", MII_PREFIX, env_shell);

        fclose(f);
        printf("Wrote %s! Mii integration is now enabled.\n", shellrc);
    } else {
        printf("Aborting.\n");
    }

    return 0;
}

void version() {
    printf("mii build %s\n", MII_VERSION);
    printf("Built on %s\n", MII_BUILD_TIME);
}
