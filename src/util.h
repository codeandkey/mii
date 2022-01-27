#pragma once

#include <cstdarg>
#include <functional>
#include <vector>
#include <string>

namespace mii {
namespace util {

    /**
     * Logs a debug message to stderr if in debug mode.
     * @param fmt Log format.
     * @param ... Log values.
     */
    void debug(const char* fmt, ...);

    /**
     * Iterates over files in a directory. Optionally recursively scans
     * subdirectories.
     *
     * @param path Parent directory.
     * @param file Entry callback. First argument is path, second is path relative to the scan root.
     */
    void scan(std::string& dir, std::function<void(std::string&, std::string)> callback, int depth = 0, std::string rel = "");

     /**
      * Returns the file name from an absolute path.
      * @param path Input path.
      * @return File name from input.
      */
    std::string basename(std::string& path);

    /**
     * Checks if a path points to an executable file.
     * @param path Path to test.
     * @return true if <path> is an executable regular file, false otherwise.
     */
    bool is_binary(std::string path);

} // namespace util
} // namespace mii

#define mii_debug(x, ...) mii::util::debug(__FILE__ ":%d | " x "\n", __LINE__, ##__VA_ARGS__)