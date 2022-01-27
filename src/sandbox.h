#pragma once

#include <memory>
#include <string>
#include <vector>

namespace mii {
namespace sandbox {
    /**
     * Initializes the sandbox state. Called automatically on first analysis.
     */
    void init();

    /**
     * Cleans up the sandbox Lua state. Should be called before exit.
     */
    void cleanup();

    /**
     * Analyzes a Lua modulefile and fills a vector of PATHs and modulepaths.
     * 
     * @param path Module file path.
     * @param out_paths Output PATH vector.
     * @param out_mpaths Output MODULEPATH vector.
     */
    void analyze(std::string path, std::vector<std::string>& out_paths, std::vector<std::string>& out_mpaths);

} // namespace sandbox
} // namespace mii