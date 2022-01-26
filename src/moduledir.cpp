#include "moduledir.h"
#include "util.h"

using namespace mii;
using namespace std;

ModuleDir::ModuleDir(string path, string parent, string parent_dir)
{
    util::scan(path, [&](string& fpath, string relpath) {
        if (fpath.size() >= 5 && fpath.substr(fpath.size() - 4) == ".lua")
            modules.emplace_back(relpath.substr(0, relpath.size() - 4), fpath);
    }, 1);

    this->root = path;
    this->parent = parent;
    this->parent_dir = parent_dir;
}

ModuleDir::ModuleDir(istream& inp)
{
    if (!(inp.flags() & ios::binary))
        throw runtime_error("Moduledir must be parsed from binary stream");

    auto check_eof = [&]()
    {
        if (inp.eof())
            throw runtime_error("Unexpected EOF parsing module");
    };

    // 1. Root length
    uint32_t root_len;
    check_eof();
    inp.read((char*) &root_len, sizeof root_len);

    // 2. Root data
    root.resize(root_len);
    check_eof();
    inp.read(&root[0], root_len);

    // 3. Parent code length
    uint32_t parent_len;
    check_eof();
    inp.read((char*) &parent_len, sizeof parent_len);

    // 4. Parent code data
    parent.resize(parent_len);
    check_eof();
    inp.read(&parent[0], parent_len);

    // 5. Parent mpath length
    uint32_t parent_mplen;
    check_eof();
    inp.read((char*) &parent_mplen, sizeof parent_mplen);

    // 6. Parent code data
    parent_dir.resize(parent_mplen);
    check_eof();
    inp.read(&parent_dir[0], parent_mplen);

    // 7. Module count
    uint32_t module_count;
    check_eof();
    inp.read((char*) &module_count, sizeof module_count);

    // 8. Individual modules
    for (unsigned i = 0; i < module_count; ++i) {
        modules.emplace_back(inp);
    }
}

namespace mii { // needed for overload
ostream& operator<<(ostream& lhs, const ModuleDir& rhs)
{
    if (!(lhs.flags() & ios::binary))
        throw runtime_error("Moduledir must be parsed from binary stream");

    // 1. Root length
    uint32_t root_len = rhs.root.size();
    lhs.write((char*) &root_len, sizeof root_len);

    // 2. Root data
    lhs.write(&rhs.root[0], root_len);

    // 3. Parent code length
    uint32_t parent_len = rhs.parent.size();
    lhs.write((char*) &parent_len, sizeof parent_len);

    // 4. Parent code data
    lhs.write(&rhs.parent[0], parent_len);

    // 5. Parent mpath length
    uint32_t parent_mplen = rhs.parent_dir.size();
    lhs.write((char*) &parent_mplen, sizeof parent_mplen);

    // 6. Parent code data
    lhs.write(&rhs.parent_dir[0], parent_mplen);

    // 7. Module count
    uint32_t module_count = rhs.modules.size();
    lhs.write((char*) &module_count, sizeof module_count);

    // 8. Individual modules
    for (unsigned i = 0; i < module_count; ++i) {
        lhs << rhs.modules[i];
    }

    return lhs;
}
} // namespace mii