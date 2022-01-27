#include "index.h"
#include "util.h"

using namespace mii;
using namespace std;

static vector<ModuleDir> mpaths;

void index::load(istream& inp)
{
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

void index::import(std::string mpath)
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

void index::save(ostream& dst)
{
    // 1. Moduledir count
    uint32_t mpath_count = mpaths.size();
    dst.write((char*) &mpath_count, sizeof mpath_count);

    // 2. Each moduledir
    for (auto& md : mpaths)
        dst << md;
}

std::vector<index::Result> index::search_exact(std::string bin)
{
    vector<index::Result> output;

    for (auto& mp : mpaths)
    {
        for (auto& m : mp.get_modules())
        {
            for (auto& b : m.get_bins())
            {
                if (b == bin)
                {
                    index::Result res;

                    res.code = m.get_code();

                    string cparent, cur_mp = mp.get_root();

                    auto find_mp = [&](string rt) -> ModuleDir& {
                        for (auto& tmp : mpaths)
                        {
                            if (tmp.get_root() == rt)
                                return tmp;
                        }
                    };
                    
                    while ((cparent = find_mp(cur_mp).get_parent()).size())
                    {
                        res.parents.push_back(cparent);
                        cur_mp = find_mp(cur_mp).get_parent_mpath();
                    }
                }
            }
        }
    }
}

const vector<ModuleDir>& index::get_mpaths()
{
    return mpaths;
}