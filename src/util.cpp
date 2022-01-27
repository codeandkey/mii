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
    va_list args;

    if (!getenv("MII_DEBUG")) return;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void util::scan(string& path, function<void(string&, string)> callback, int depth, string rel)
{
    DIR* d;
    struct dirent* dp;
    struct stat st;

    mii_debug("walking %s", path.c_str());

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

        if (!S_ISDIR(st.st_mode))
        {
            callback(abs_path, rel + dp->d_name);
            continue;
        }

        if (depth > 0 && is_binary(abs_path)) // is_binary just checks perms here
            scan(abs_path, callback, depth - 1, rel + dp->d_name + "/");
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

int util::distance(string a, string b)
{
    int a_len = a.size(), b_len = b.size();
    int mat_num = (a_len + 1) * (b_len + 1);

    // initialize 0 matrix
    int* mat = new int[mat_num];
    memset(mat, 0, mat_num * sizeof *mat);

    // initialize top-left matrix boundaries
    for (int i = 1; i <= a_len; ++i) mat[(b_len + 1) * i] = i;
    for (int i = 1; i <= b_len; ++i) mat[i] = i;

    // walk rows until the matrix is filled
    for (int i = 1; i <= a_len; ++i) {
        for (int j = 1; j <= b_len; ++j) {
            int deletion  = mat[(i - 1) * (b_len + 1) + j] + 1;
            int insertion = mat[i * (b_len + 1) + j - 1] + 1;
            int substitution = mat[(i - 1) * (b_len + 1) + j - 1] + (tolower(a[i - 1]) != tolower(b[j - 1]));

            mat[i * (b_len + 1) + j] = min(deletion, min(insertion, substitution));

            // transposition with optimal string alignment distance
            if (i > 1 && j > 1 && tolower(a[i - 1]) == tolower(b[j - 2]) && tolower(a[i - 2]) == tolower(b[j - 1])) {
                mat[i * (b_len + 1) + j] = min(mat[i * (b_len + 1) + j], mat[(i - 2) * (b_len + 1) + j - 2] + 1);
            }
        }
    }

    // distance is the bottom-rightmost value
    int result = mat[mat_num - 1];

    // cleanup
    delete[] mat;

    return result;
}
