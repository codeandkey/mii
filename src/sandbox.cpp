#include "sandbox.h"
#include "options.h"

#include <stdexcept>
#include <vector>

using namespace mii;
using namespace std;

Sandbox::Sandbox()
{
    state = luaL_newstate();

    if (!state)
        throw runtime_error("alloc failure");

    luaL_openlibs(state);

    // Determine sandbox path

    vector<string> paths;

    paths.push_back(options::prefix() + "/share/mii/lua/sandbox.luac");
    paths.push_back("./sandbox.luac");

    for (string& p: paths)
        if (luaL_dofile(state, p.c_str()) == LUA_OK)
            return;

    lua_close(state);
    throw runtime_error("Failed to execute sandbox file");
}
