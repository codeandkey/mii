#include <iostream>
#include <string>

#include "options.h"
#include "util.h"
#include "index.h"

using namespace mii;
using namespace std;

int main(int argc, char** argv) {
    // Load prefix
    options::prefix(argv[0]);

    mii_debug("prefix: %s", options::prefix().c_str());
    mii_debug("version: %s", options::version().c_str());

    Index ind;
    
    ind.import("/home/jtst/git/spack/share/spack/lmod/linux-archrolling-x86_64/Core");

    for (auto& mp : ind.get_mpaths())
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
