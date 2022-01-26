#include <cstring>
#include <stdexcept>

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "util.h"

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

void util::scan(string& path, function<void(string&, string)> callback, int depth, string rel)
{
    DIR* d;
    struct dirent* dp;
    struct stat st;

    mii_debug("walking path %s", path.c_str());

    if (!(d = opendir(path.c_str())))
        throw runtime_error("Failed to walk " + path + ": " + strerror(errno));

    while ((dp = readdir(d)))
    {
        // Ignore self and parent dirs
        if (*dp->d_name == '.')
            continue;

        string abs_path = path + "/" + dp->d_name;

        if (stat(abs_path.c_str(), &st))
            mii_debug("couldn't stat %s : %s", abs_path.c_str(), strerror(errno));

        if (S_ISDIR(st.st_mode))
        {
            if (depth > 0 && is_binary(abs_path)) scan(abs_path, callback, depth - 1, rel + dp->d_name + "/");
        } else
        {
            callback(abs_path, rel + dp->d_name);
        }
    }

    closedir(d);
}

string util::basename(string& path) {
    auto sep = path.rfind("/");

    if (sep == string::npos)
        return path;
    
    return path.substr(sep + 1);
}

bool util::is_binary(string path) {
    return !access(path.c_str(), X_OK);
}