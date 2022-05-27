#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "options.h"
#include "util.h"
#include "index.h"
#include "sandbox.h"

using namespace mii;
using namespace std;

/**
 * Builds a new index and saves it to <dst>.
 * 
 * @param dst  Selected index path
 */
int cmd_build(string dst);

/**
 * Searches the index.
 *
 * @param args Argument list
 */
int cmd_find(vector<string> args);

/**
 * Shows help information.
 *
 * @param arg0 First argument
 */
int cmd_help(string arg0);

/**
 * Shows version information.
 */
int cmd_version();

/**
 * Writes the mii index to stdout.
 */
int cmd_list();

int main(int argc, char** argv) {
    mii_debug("prefix: %s", options::prefix().c_str());
    mii_debug("version: %s", options::version().c_str());

    string index_path = options::prefix() + "var/mii/index";

    // Check for global index
    char* index_env = getenv("MII_INDEX");
    if (index_env && strlen(index_env))
        index_path = index_env;

    // Parse options
    string arg0 = argv[0];
    vector<string> args;

    string subcmd;

    for (int i = 0; i < argc; ++i)
        args.push_back(argv[i]);

    args.erase(args.begin()); // pop mii command

    while (args.size())
    {
        string arg = args.front();

        if (arg.size() && arg[0] != '-')
        {
            // Grab subcommand
            subcmd = arg;
            args.erase(args.begin()); // pop subcommand
            break;
        }

        if (arg == "-h" || arg == "--help")
            return cmd_help(arg0);
        else if (arg == "-v" || arg == "--version")
            return cmd_version();

        cerr << "error: unrecognized argument " << arg << "\n";
        return -1;
    }

    if (subcmd.empty())
    {
        subcmd = "help";
        cerr << "error: expected subcommand\n";
    }

    auto cleanup = [](int ret) -> int {
        sandbox::cleanup();
        return ret;
    };

    if (subcmd == "build")
        return cleanup(cmd_build(index_path));
    else if (subcmd == "help")
        return cleanup(cmd_help(arg0));
    else if (subcmd == "version")
        return cleanup(cmd_version());

    // Load index from disk
    index::load(index_path);

    if (subcmd == "find")
        return cleanup(cmd_find(args));
    else if (subcmd =="list")
        return cleanup(cmd_list());

    return 0;
}

int cmd_build(string dst)
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

int cmd_find(vector<string> args)
{
    bool exact = false, parse = false;
    string bin;

    // Parse options
    for (auto& arg : args) {
        if (arg.size() && arg[0] != '-')
        {
            if (bin.size())
            {
                cerr << "find error: only a single COMMAND is allowed\n";
                return -1;
            }

            bin = arg;
            continue;
        }

        if (arg == "-e" || arg == "--exact")
            exact = true;
        else if (arg == "-p" || arg == "--parse")
            parse = true;
        else if (arg == "-h" || arg == "--help")
        {
            cout << "USAGE: find [-e|--exact] [-p|--parse] COMMAND\n"
                    << "    Searches for modules providing COMMAND\n\n";

            cout << "find options:\n"
                    << "    -e --exact\tOnly show exact matches\n"
                    << "    -p --parse\tPrint output in parser-friendly format\n";

            return 0;
        }
        else
        {
            cerr << "find error: unrecognized option " << arg << "\n";
            return -1;
        }
    }

    if (!bin.size())
    {
        cerr << "find error: a COMMAND is required\n";
        return -1;
    }

    vector<index::Result> results;
    
    if (exact)
        results = index::search_exact(bin);
    else
        results = index::search_fuzzy(bin);

    if (!results.size())
    {
        if (!parse)
            cout << "No results found for '" << bin << "'" << endl;

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

int cmd_list()
{
    for (auto& mp : index::get_mpaths())
    {
        cout << "modulepath " << mp.get_root() << endl;

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

int cmd_help(string arg0)
{
    cout << "USAGE: " << arg0 << " [OPTIONS] <SUBCOMMAND>\n";
    cout << "\nOPTIONS:\n";
    cout << "    -h, --help            Print this message\n";
    cout << "    -v, --version         Print version information\n";
    cout << "    -i, --index <PATH>    Use index at PATH\n";
    cout << "\nSUBCOMMANDS:\n";
    cout << "    build             Scan the MODULEPATH and build the index\n";
    cout << "    find [-ep] CMD    Search modules for command CMD\n";
    cout << "    help              Print this message\n";
    cout << "    list              Print a tree of cached modules\n";
    cout << "    version           Print version information\n";
    return 0;
}

int cmd_version()
{
    cout << "Mii version " MII_VERSION " build " MII_BUILD_TIME << endl;
    return 0;
}
