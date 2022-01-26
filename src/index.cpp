#include "index.h"
#include "util.h"

using namespace mii;
using namespace std;

Index::Index() {};

Index::Index(istream& inp)
{
    if (!(inp.flags() & ios::binary))
        throw runtime_error("Moduledir must be parsed from binary stream");

    auto check_eof = [&]()
    {
        if (inp.eof())
            throw runtime_error("Unexpected EOF parsing module");
    };

    // 1. Moduledir count
    uint32_t mpath_count;
    check_eof();
    inp.read((char*) &mpath_count, sizeof mpath_count);

    // 2. Each moduledir
    for (unsigned i = 0; i < mpath_count; ++i)
        mpaths.emplace_back(inp);
}

void Index::import(std::string mpath)
{
    for (auto& mp : mpaths)
        if (mp.get_root() == mpath)
        {
            mii_debug("Dependency loop detected in mpath %s", mpath.c_str());
            return;
        }

    mpaths.emplace_back(mpath);

    // Continue importing new child modulepaths
    for (auto& m : mpaths.back().get_modules())
        for (auto& mp : m.get_mpaths())
            import(mp);
}

namespace mii { // needed for overload
ostream& operator<<(ostream& lhs, const Index& rhs)
{
    // 1. Moduledir count
    uint32_t mpath_count = rhs.mpaths.size();
    lhs.write((char*) &mpath_count, sizeof mpath_count);

    // 2. Each moduledir
    for (auto& md : rhs.mpaths)
        lhs << md;

    return lhs;
}
} // namespace mii