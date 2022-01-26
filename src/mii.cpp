#include <iostream>
#include <string>

#include "options.h"
#include "util.h"
#include "moduledir.h"

using namespace mii;
using namespace std;

int main(int argc, char** argv) {
    // Load prefix
    options::prefix(argv[0]);

    mii_debug("prefix: %s", options::prefix().c_str());
    mii_debug("version: %s", options::version().c_str());

    ModuleDir md("/home/jtst/git/spack/share/spack/lmod/linux-archrolling-x86_64/Core");

    // print module paths

    for (auto& m : md.get_modules()) 
    {
        cout << "module " << m.get_code() << endl;

        for (auto& b : m.get_bins())
            cout << "\t" << b << endl;
    }

    return 0;
}
