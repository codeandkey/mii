#define _POSIX_C_SOURCE 200809L

#include "util.h"

#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

using namespace mii;
using namespace std;

void util::debug(const char* fmt, ...) 
{
#ifndef MII_DEBUG
    return;
#endif
    va_list args;
    va_start(args, fmt);

    vfprintf(stderr, fmt, args);

    va_end(args);
}

std::vector<std::string> util::scan_path(std::string path)
{
    DIR* d;
    struct dirent* dp;
    struct stat st;

    vector<string> bins_out, subpaths;

    string tok;
    for (unsigned c = 0; c < path.size(); ++c)
    {
        if (path[c] == ':') {
            if (tok.size()) subpaths.push_back(tok);
        } else {
            tok += path[c];
        }
    }

    if (tok.size()) subpaths.push_back(tok);

    for (auto& cur_path : subpaths)
    {
        mii_debug("scanning PATH %s", cur_path.c_str());

        if (!(d = opendir(cur_path.c_str()))) {
            mii_debug("Failed to open %s, ignoring : %s", cur_path.c_str(), strerror(errno));
            continue;
        }

        while ((dp = readdir(d))) {
            if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) continue;

            string abs_path = cur_path + "/" + dp->d_name;

            if (!stat(abs_path.c_str(), &st)) {
                if ((S_ISREG(st.st_mode) || S_ISLNK(st.st_mode)) && !access(abs_path.c_str(), X_OK)) {
                    bins_out.push_back(dp->d_name);
                }
            } else {
                mii_debug("Couldn't stat %s : %s", abs_path.c_str(), strerror(errno));
            }
        }

        closedir(d);
    }

    return bins_out;
}