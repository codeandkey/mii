#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "moduledir.h"

namespace mii {
class Index {
public:
    /**
     * Initializes a new, empty index.
     */
    Index();

    /**
     * Loads an index from an input stream. The stream must be opened in
     * binary mode.
     *
     * @param inp Input stream to deserialize from.
     */
    Index(std::istream& inp);

    /**
     * Imports a module directory into the index. Performs analysis as
     * necessary, so the imported modules can be searched. Recursively
     * imports submodulepaths.
     *
     * @param mpath Module path root. Should be a path to a *single* directory.
     */
    void import(std::string mpath);

    /**
     * Serializes the index to an output stream. The output stream must be
     * opened in binary mode.
     */
    friend std::ostream& operator<<(std::ostream& lhs, const Index& rhs);

    /**
     * Gets the list of active modulepaths.
     */
    const std::vector<ModuleDir>& get_mpaths() const
    {
        return mpaths;
    }

private:
    std::vector<ModuleDir> mpaths;

}; // class Index
} // namespace mii