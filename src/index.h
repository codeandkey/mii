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
     * @param path Index path.
     */
    void load(std::string path);

    /**
     * Imports a module directory into the index. Performs analysis as
     * necessary, so the imported modules can be searched. Recursively
     * imports submodulepaths.
     *
     * @param mpath Module path root. Should be a path to a *single* directory.
     * @param parent_code Moduledir parent code, if applicable
     * @param parent_mpath Moduledir parent mpath, if applicable
     */
    void import(std::string mpath, std::string parent_code="", std::string parent_mpath="");

    /**
     * Serializes the index to an output stream. The output stream must be
     * opened in binary mode.
     * 
     * @param path Output path.
     */
    void save(std::string path);

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
