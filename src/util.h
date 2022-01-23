#pragma once

#include <cstdarg>

namespace mii {
namespace util {

    /**
     * Logs a debug message to stderr if in debug mode.
     * @param fmt Log format.
     * @param ... Log values.
     */
    void debug(const char* fmt, ...);

} // namespace util
} // namespace mii

#define mii_debug(x, ...) mii::util::debug(__FILE__ ":%d | " x, __LINE__, ##__VA_ARGS__)

#define mii_min(x, y) ((x < y) ? (x) : (y))
//char* mii_strdup(const char* str);
//char* mii_join_path(const char* a, const char* b);
//int mii_levenshtein_distance(const char* a, const char* b);
//int mii_recursive_mkdir(const char* path, mode_t mode);