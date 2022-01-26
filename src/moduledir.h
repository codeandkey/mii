#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "module.h"

namespace mii {
class ModuleDir {
public:
    /**
     * Initializes a fresh modulepath from a directory containing modules.
     * Recursively scans directories for modules.
     *
     * @param path Module path to scan.
     */
    ModuleDir(std::string path);

    /**
     * Deserializes a saved ModuleDir class from an input stream.
     * The input stream must be set to binary mode.
     *
     * @param inp Input stream to parse from.
     */
    ModuleDir(std::istream& inp);

    /**
     * Serializes the ModuleDir to an output stream.
     * The stream must be opened in binary mode.
     */
    friend std::ostream& operator<<(std::ostream& out, const ModuleDir& rhs);

    /**
     * Gets a reference to the list of modules.
     */
    const std::vector<Module>& get_modules() {
        return modules;
    }

private:
    std::vector<Module> modules;
    std::string root;

}; // class ModuleDir
} // namespace mii