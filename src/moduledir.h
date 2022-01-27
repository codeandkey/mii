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
     * @param parent Parent module code, if applicable
     * @param parent_dir Containing mpath for parent module, if applicable
     */
    ModuleDir(std::string path, std::string parent = "", std::string parent_dir = "");

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
    const std::vector<Module>& get_modules() const
    {
        return modules;
    }

    /**
     * Gets the root path of this module directory.
     */
    const std::string& get_root() const
    {
        return root;
    }

    /**
     * Gets the parent module code, if there is one.
     */
    const std::string& get_parent() const
    {
        return parent;
    }

    /**
     * Gets the parent module mpath, if there is one.
     */
    const std::string& get_parent_mpath() const
    {
        return parent_dir;
    }

private:
    std::vector<Module> modules;
    std::string root, parent, parent_dir;

}; // class ModuleDir
} // namespace mii