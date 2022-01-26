#include "index.h"

using namespace mii;
using namespace std;

Index::Index() {};

Index::Index(istream& inp)
{
}

void Index::import(std::string mpath)
{
    // Scan directory for modulefiles
}

namespace mii { // needed for overload
ostream& operator<<(ostream& lhs, const Index& rhs)
{
    // 1. Modulepath count
    uint32_t mpath_count = rhs.indices.size();
    lhs.write((char*) &mpath_count, sizeof mpath_count);

    // 2. Each modulepath



    return lhs;
}
} // namespace mii