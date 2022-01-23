#pragma once

#include <memory>
#include <string>
#include <vector>

#include <lua.hpp>

namespace mii {

/**
 * Used for maintaining a global Lua state for module analysis.
 */
class Sandbox {
public:
    /**
     * Returns a reference to the global sandbox.
     */
    static Sandbox& get() 
    {
        if (!inst)
            inst.reset(new Sandbox());
        
        return *inst;
    }

    /**
     * Analyzes a Lua modulefile and fills a vector of PATHs and modulepaths.
     * 
     * @param path Module file path.
     * @param out_paths Output PATH vector.
     * @param out_mpaths Output MODULEPATH vector.
     */
    void analyze(std::string path, std::vector<std::string>& out_paths, std::vector<std::string>& out_mpaths);

private:
    Sandbox();
    static std::unique_ptr<Sandbox> inst;

    lua_State* state;

}; // class Sandbox

} // namespace mii