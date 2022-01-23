#include <iostream>
#include <string>

#include "options.h"
#include "util.h"

using namespace mii;

int main(int argc, char** argv) {
    // Load prefix
    options::prefix(argv[0]);

    mii_debug("prefix: %s\n", options::prefix().c_str());
    mii_debug("version: %s\n", options::version().c_str());

    return 0;
}