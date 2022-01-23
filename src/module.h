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
     * @param path Module file path.
     */
    Module(std::string path);

    std::vector<std::string>& get_bins() {
        return bins;
    }

    std::vector<std::string>& get_mpaths() {
        return mpaths;
    }

private:
    std::vector<std::string> bins, mpaths;
}; // class Module

} // namespace mii
