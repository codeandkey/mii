#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "moduledir.h"

namespace mii {
namespace index {
    struct Result {
        std::string code, relevance;
        std::vector<std::string> parents;
    };

    /**
     * Loads an index from an input stream. The stream must be opened in
     * binary mode.
     *
     * @param inp Input stream to deserialize from.
     */
    void load(std::istream& inp);

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
     * 
     * @param dst Output stream.
     */
    void save(std::ostream& dst);

    /**
     * Searches the index for all exact matches of a single binary.
     * 
     * @param bin Binary to search
     */
    std::vector<Result> search_exact(std::string bin);

    /**
     * Gets the list of active modulepaths.
     */
    const std::vector<ModuleDir>& get_mpaths();

} // namespace index
} // namespace mii