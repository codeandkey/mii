#include "options.h"
#include "build.h"

#include "util.h"

#include <cerrno>
#include <cstring>
#include <ctime>
#include <stdexcept>

#include <unistd.h>
#include <sys/stat.h>

using namespace mii;
using namespace std;

string options::prefix()
{
    // Grab absolute path to running binary
    char* name = realpath("/proc/self/exe", NULL);

    string exepath(name);
    free(name);

    auto pt = exepath.rfind('/');

    if (pt >= exepath.size())
        throw runtime_error("Invalid exepath, cannot determine prefix");
        
    exepath.erase(exepath.begin() + pt, exepath.end());
    return exepath + "/../";
}

string options::version()
{
    return MII_VERSION;
}
