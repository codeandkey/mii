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

private:
    std::vector<std::string> binaries;
    std::vector<std::unique_ptr<Module>> children;

    void analyze_lua(std::istream& data);
}; // class Module

} // namespace mii
