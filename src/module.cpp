#include "module.h"
#include "sandbox.h"
#include "util.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>

using namespace mii;
using namespace std;

Module::Module(string code, string path)
{
    ifstream fd(path);

    // Check for file open failure
    if (!fd) throw runtime_error("Failed to open " + path + " for reading");;

    vector<string> paths;

    // Perform analysis
    if (string(path.end() - 4, path.end()) == ".lua")
        Sandbox::get().analyze(path, paths, mpaths);
    else
        throw runtime_error("Unknown module type for " + path);

    // Scan paths and eliminate duplicates
    for (auto& path: paths)
    {
        util::scan(path, [&](string& path, string rel) {
            if (util::is_binary(path))
                bins.push_back(util::basename(path));
        });
    }

    sort(bins.begin(), bins.end());
    bins.erase(unique(bins.begin(), bins.end()), bins.end());

    this->code = code;
    this->abs_path = path;
}

Module::Module(std::istream& inp)
{
    // Read module data from input stream
    if (!(inp.flags() & ios::binary))
        throw runtime_error("Module must be parsed from binary stream");

    auto check_eof = [&]()
    {
        if (inp.eof())
            throw runtime_error("Unexpected EOF parsing module");
    };

    // 1. Binary count
    uint32_t num_bins;
    check_eof();
    inp.read((char*) &num_bins, sizeof num_bins);
    bins.reserve(num_bins);

    // 2. Binary blocks
    for (unsigned i = 0; i < num_bins; ++i)
    {
        std::string cbin;
        uint8_t bin_length;

        check_eof();
        inp.read((char*) &bin_length, sizeof bin_length);
        cbin.resize(bin_length);
        check_eof();
        inp.read(&cbin[0], bin_length);
        bins.push_back(cbin);
    }

    // 3. Child mpath count
    uint32_t num_mpaths;
    check_eof();
    inp.read((char*) &num_mpaths, sizeof num_mpaths);

    // 4. Child mpath blocks
    for (unsigned i = 0; i < num_mpaths; ++i)
    {
        std::string cmpath;
        uint8_t mpath_length;

        check_eof();
        inp.read((char*) &mpath_length, sizeof mpath_length);
        cmpath.resize(mpath_length);
        check_eof();
        inp.read(&cmpath[0], mpath_length);
        mpaths.push_back(cmpath);
    }

    // 5. Fullpath length
    uint32_t fp_length;
    check_eof();
    inp.read((char*) &fp_length, sizeof fp_length);

    // 6. Fullpath data    
    abs_path.resize(fp_length);
    check_eof();
    inp.read(&abs_path[0], fp_length);

    // 7. Loading code len
    uint32_t code_len;
    check_eof();
    inp.read((char*) &code_len, sizeof code_len);

    // 8. Loading code data.
    code.resize(code_len);
    inp.read(&code[0], code_len);
}

namespace mii { // needed for overload
ostream& operator<<(ostream& lhs, const Module& rhs)
{
    if (!(lhs.flags() & ios::binary))
        throw runtime_error("Module cannot serialize to non-binary streams");

    // 1. Binary count
    uint32_t num_bins = rhs.bins.size();
    lhs.write((char*) &num_bins, sizeof num_bins);

    // 2. Binary blocks
    for (unsigned i = 0; i < num_bins; ++i)
    {
        uint8_t bin_length = rhs.bins[i].size();
        lhs.write((char*) &bin_length, sizeof bin_length);
        lhs.write(&rhs.bins[i][0], bin_length);
    }

    // 3. Child mpath count
    uint32_t num_mpaths = rhs.mpaths.size();
    lhs.write((char*) &num_mpaths, sizeof num_mpaths);

    // 4. Child mpath blocks
    for (unsigned i = 0; i < num_mpaths; ++i)
    {
        uint8_t mpath_length = rhs.mpaths[i].size();

        lhs.write((char*) &mpath_length, sizeof mpath_length);
        lhs.write(&rhs.mpaths[i][0], mpath_length);
    }

    // 5. Fullpath length
    uint32_t fp_length = rhs.abs_path.size();
    lhs.write((char*) &fp_length, sizeof fp_length);

    // 6. Fullpath data
    lhs.write(&rhs.abs_path[0], fp_length);

    // 7. Loading code length
    uint32_t code_len = rhs.code.size();
    lhs.write((char*) &code_len, sizeof code_len);

    // 8. Loading code data
    lhs.write(&rhs.code[0], code_len);

    return lhs;
}
} // namespace mii