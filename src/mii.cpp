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
 * @param dst Target index file path
 */
void cmd_build(string dst);

/**
 * Searches the index for modules providing a binary.
 * 
 * @param bin Binary search query.
 */
void cmd_exact(string bin);

int main(int argc, char** argv) {
    // Load prefix
    options::prefix(argv[0]);

    string index_path = options::prefix() + "var/mii/index";

    // Check for global index
    char* index_env = getenv("MII_INDEX");
    if (index_env && strlen(index_env))
        index_path = index_env;

    if (argc > 1 && string(argv[1]) == "build")
    {
        cmd_build(index_path);
        return 0;
    }

    mii_debug("prefix: %s", options::prefix().c_str());
    mii_debug("version: %s", options::version().c_str());

    // Load index from disk
    ifstream inp(index_path, ios::binary);
    index::load(inp);
    inp.close();

    if (argc > 1 && string(argv[1]) == "exact")
    {
        if (argc < 3)
            throw runtime_error("expected argmuent");

        cmd_exact(argv[2]);
        return 0;
    }

    if (!inp)
        throw runtime_error("Couldn't open " + index_path + " for reading: " + strerror(errno));

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

    // Cleanup
    sandbox::cleanup();

    return 0;
}

void cmd_build(string dst)
{
    cout << "Building index.." << endl;

    char* modulepath_env = getenv("MODULEPATH");

    if (!modulepath_env || !strlen(modulepath_env))
        throw runtime_error("MODULEPATH is not set");

    for (char* cpath = strtok(modulepath_env, ":"); cpath; cpath = strtok(NULL, ":"))
    {
        cout << "Indexing modulepath " << cpath << " .. ";
        cout.flush();

        index::import(cpath);

        cout << "done" << endl;
    }

    ofstream target(dst, ios::binary);

    if (!target)
        throw runtime_error("Couldn't open " + dst + " for writing: " + strerror(errno));

    cout << "Writing index to " << dst << " .. ";
    cout.flush();

    index::save(target);
    target.close();

    cout << "done" << endl;
}

void cmd_exact(string bin) {
    vector<index::Result> results = index::search_exact(bin);

    if (!results.size())
    {
        cout << "No results found for '" << bin << "'" << endl;
        return;
    }

    int code_width = 0;

    for (auto& r : results)
        code_width = max(code_width, (int) r.code.size());

    for (auto& r : results)
    {
        cout << setw(code_width) << r.code << endl;

        for (auto& p : r.parents)
            wcout << L'\x2557' << " "  << wstring(p.begin(), p.end()) << endl;
    }
}
