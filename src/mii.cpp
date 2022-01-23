#include <iostream>
#include <string>

#include "options.h"
#include "util.h"
#include "module.h"

using namespace mii;

int main(int argc, char** argv) {
    // Load prefix
    options::prefix(argv[0]);

    mii_debug("prefix: %s\n", options::prefix().c_str());
    mii_debug("version: %s\n", options::version().c_str());

    Module m("/home/jtst/git/spack/share/spack/lmod/linux-archrolling-x86_64/Core/automake/1.16.3-sni3p47.lua");

    // print module paths

    for (auto& b : m.get_bins()) {
        mii_debug("bin: %s\n", b.c_str());
    }

    return 0;
}