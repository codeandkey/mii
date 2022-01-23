#include "module.h"

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

    // Perform analysis
    if (string(path.end() - 4, path.end()) == ".lua")
        analyze_lua(fd);
    else
        throw runtime_error("Unknown module type for " + path);
}

void Module::analyze_lua(istream& data) {
    // Read data into buf
    istream_iterator<char> it(data), end;
    string code(it, end);

    // 
}
