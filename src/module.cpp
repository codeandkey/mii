#include "module.h"
#include "sandbox.h"
#include "util.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>

#include <lua.hpp>

using namespace mii;
using namespace std;

Module::Module(string path) {
    ifstream fd(path);

    // Check for file open failure
    if (!fd) throw runtime_error("Failed to open " + path + " for reading");;

    vector<string> paths;

    // Perform analysis
    if (string(path.end() - 4, path.end()) == ".lua")
        Sandbox::get().analyze(path, paths, mpaths);
    else
        throw runtime_error("Unknown module type for " + path);

    // Scan paths and eliminate duplicates
    for (auto& path: paths)
    {
        auto local_bins = util::scan_path(path);
        bins.insert(bins.end(), local_bins.begin(), local_bins.end());
    }

    unique(bins.begin(), bins.end());
}