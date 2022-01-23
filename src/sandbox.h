#pragma once

#include <memory>

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

private:
    Sandbox();
    static std::unique_ptr<Sandbox> inst;

    lua_State* state;

}; // class Sandbox

} // namespace mii