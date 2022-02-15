#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include <unistd.h>
#include <getopt.h>

#include "options.h"
#include "util.h"
#include "index.h"
#include "sandbox.h"

using namespace mii;
using namespace std;

/**
 * Builds a new index and saves it to <dst>.
 * 
 * @param argc Argument count
 * @param argv Argument list
 * @param dst  Selected index path
 */
int cmd_build(int argc, char** argv, string dst);

/**
 * Searches the index.
 *
 * @param argc Argument count
 * @param argv Argument list
 */
int cmd_find(int argc, char** argv);

/**
 * Shows help information.
 *
 * @param argc Argument count
 * @param argv Argument list
 */
int cmd_help(int argc, char** argv);

/**
 * Shows version information.
 *
 * @param argc Argument count
 * @param argv Argument list
 */
int cmd_version(int argc, char** argv);

/**
 * Writes the mii index to stdout.
 *
 * @param argc Argument count
 * @param argv Argument list
 */
int cmd_list(int argc, char** argv);

int main(int argc, char** argv) {
    int opt;

    mii_debug("prefix: %s", options::prefix().c_str());
    mii_debug("version: %s", options::version().c_str());

    string index_path = options::prefix() + "var/mii/index";

    // Check for global index
    char* index_env = getenv("MII_INDEX");
    if (index_env && strlen(index_env))
        index_path = index_env;

    // Parse options
    static struct option long_options[] = {
        { "index",      required_argument, NULL, 'i' },
        { "help",       no_argument,       NULL, 'h' },
        { "version",    no_argument,       NULL, 'v' },
        { NULL,         0,                 NULL,  0 },
    };

    while ((opt = getopt_long(argc, argv, "i:hj", long_options, NULL)) != -1)
        switch (opt)
        {
            case 'h':
                return cmd_help(argc, argv);
            case 'i':
                index_path = optarg;
                break;
            case 'v':
                return cmd_version(0, NULL);
            default:
                return -1;
        }

    if (optind >= argc || !argv[optind])
    {
        cerr << "error: expected subcommand\n";
        return -1;
    }

    auto cleanup = [](int ret) -> int {
        sandbox::cleanup();
        return ret;
    };

    if (argv[optind] == string("build"))
        return cleanup(cmd_build(argc - (optind + 1), argv + optind + 1, index_path));

    if (argv[optind] == string("help"))
        return cleanup(cmd_help(argc, argv));

    if (argv[optind] == string("version"))
        return cleanup(cmd_version(argc - (optind + 1), argv + optind + 1));

    // Load index from disk
    index::load(index_path);

    if (argv[optind] == string("find"))
        return cleanup(cmd_find(argc - (optind + 1), argv + optind + 1));

    if (argv[optind] == string("list"))
        return cleanup(cmd_list(argc - (optind + 1), argv + optind + 1));

    return 0;
}

int cmd_build(int argc, char** argv, string dst)
{
    cout << "Building index.." << endl;

    char* modulepath_env = getenv("MODULEPATH");

    if (!modulepath_env || !strlen(modulepath_env))
    {
        cerr << "error: MODULEPATH is not set\n";
        return -1;
    }

    for (char* cpath = strtok(modulepath_env, ":"); cpath; cpath = strtok(NULL, ":"))
    {
        cout << "Indexing modulepath " << cpath << " .. ";
        cout.flush();

        index::import(cpath);

        cout << "done" << endl;
    }

    cout << "Writing index to " << dst << " .. ";
    cout.flush();

    index::save(dst);

    cout << "done" << endl;
    return -1;
}

int cmd_find(int argc, char** argv)
{
    int opt;
    bool exact = false, parse = false;

    // Parse options
    static struct option long_options[] = {
        { "exact",      no_argument, NULL, 'i' },
        { "parse",      no_argument, NULL, 'h' },
        { NULL,         0,           NULL,  0 },
    };

    while ((opt = getopt_long(argc, argv, "i:hj", long_options, NULL)) != -1)
        switch (opt)
        {
            case 'h':
                cout << "usage: find [-e|--exact] [-p|--parse] COMMAND\n"
                     << "\tsearches for modules providing COMMAND\n\n";

                cout << "find options:\n"
                     << "\t-e --exact\tonly show exact matches\n"
                     << "\t-p --parse\trender output in parser-friendly format\n";
                return 0;
            case 'e':
                exact = true;
                break;
            case 'p':
                parse = true;
                break;
            default:
                return -1;
        }

    if (optind >= argc || !argv[optind])
    {
        cerr << "find error: expected COMMAND\n";
        return -1;
    }

    string bin = argv[optind];

    vector<index::Result> results;
    
    if (exact)
        results = index::search_exact(bin);
    else
        results = index::search_fuzzy(bin);

    if (!results.size())
    {
        if (!parse)
            cout << "no results found for '" << bin << "'" << endl;

        return 0;
    }

    int code_width = 0;

    for (auto& r : results)
        code_width = max(code_width, (int) r.code.size());

    for (auto& r : results)
    {
        for (int i = r.parents.size() - 1; i >= 0; --i)
            cout << r.parents[i] << ":";

        cout << r.code << endl;
    }

    return 0;
}

int cmd_list(int argc, char** argv)
{
    for (auto& mp : index::get_mpaths())
    {
        cout << "mpath " << mp.get_root() << endl;

        for (auto& m : mp.get_modules()) 
        {
            cout << "\tmodule " << m.get_code() << endl;

            for (auto& b : m.get_bins())
                cout << "\t\t" << b << endl;

            for (auto& c : m.get_mpaths())
                cout << "\t\t[child] " << c << endl;
        }
    }

    return 0;
}

int cmd_help(int argc, char** argv)
{
    cout << "usage: " << argv[0] << " [-h|--help] [-v|--version] [-i INDEX] SUBCOMMAND [OPTIONS]\n";
    cout << "available subcommands:\n"
        << "\tbuild  \tbuild the module index\n"
        << "\tfind   \tsearch the index for a command\n"
        << "\thelp   \tshow this information\n"
        << "\tlist   \tshow module index tree\n"
        << "\tselect \tinteractively select a module\n"
        << "\tversion\tshow version information\n";
    return 0;
}

int cmd_version(int argc, char** argv)
{

    cout << "mii version " MII_VERSION " build " MII_BUILD_TIME;
    return 0;
}