#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace mii {

/**
 * Module class. Refers to a single modulefile on the disk. Maintains module
 * binaries and child modulepaths.
 */
class Module {
public:
    /**
     * Initialize a module from a path. Searches the module PATHs for binaries
     * and MODULEPATHs for children.
     *
     * @param code Module load code.
     * @param path Module file path.
     */
    Module(std::string code, std::string path);

    /**
     * Deserializes a module from an index file.
     */
    Module(std::istream& inp);

    const std::vector<std::string>& get_bins() const {
        return bins;
    }

    const std::vector<std::string>& get_mpaths() const {
        return mpaths;
    }

    const std::string& get_code() const {
        return code;
    }

    const std::string& get_path() const {
        return abs_path;
    }

    /**
     * Serializes a module to a binary stream.
     */
    friend std::ostream& operator<<(std::ostream& dst, const Module& rhs);

private:
    std::vector<std::string> bins, mpaths;
    std::string parent_mpath, abs_path, code;
    time_t index_time;

}; // class Module
} // namespace mii
