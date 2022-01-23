#include "options.h"
#include "build.h"

#include <stdexcept>

#include <time.h>
#include <unistd.h>

using namespace mii;
using namespace std;

string options::prefix(std::string arg0)
{
    static string prefix_value;

    if (arg0.length())
    {
        // Find dirname(arg0)
        auto pt = arg0.rfind('/');

        if (arg0.begin() + pt == arg0.end())
            throw runtime_error("Invalid arg0, cannot determine prefix");
        
        arg0.erase(arg0.begin() + pt, arg0.end());
        prefix_value = arg0 + "/../";
    }

    if (!prefix_value.size())
        throw runtime_error("Prefix queried before init");

    return prefix_value;
}

string options::version()
{
    return MII_VERSION;
}
