#include "sandbox.h"
#include "options.h"
#include "util.h"

#include <fstream>
#include <iterator>
#include <stdexcept>

/* After Lua 5.1, lua_objlen changed name and LUA_OK is defined */
#if LUA_VERSION_NUM <= 501
#define mii_lua_len lua_objlen
#define LUA_OK 0
#else
#define mii_lua_len lua_rawlen
#endif

using namespace mii;
using namespace std;

unique_ptr<Sandbox> Sandbox::inst = nullptr;

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
        else
            lua_pop(state, 1); // pop error message

    lua_close(state);
    throw runtime_error("Failed to execute sandbox file");
}

void Sandbox::analyze(string path, vector<string>& out_paths, vector<string>& out_mpaths)
{    
    ifstream f(path, ios::binary | ios::ate);

    if (!f)
        throw runtime_error("Failed to open " + path + " for reading");

    // Read data into buf
    streamsize sz = f.tellg();
    vector<char> code;

    f.seekg(0, ios::beg);
    code.reserve(sz);
    f.read(code.data(), sz);

    // Prep sandbox function
    lua_getglobal(state, "sandbox_run");
    lua_pushstring(state, code.data());

    // Execute modulefile
    if(lua_pcall(state, 1, 1, 0) != LUA_OK)
    {
        lua_pop(state, 1);
        throw runtime_error("Error occurred in Lua sandbox : " + string(lua_tostring(state, -1)));
    }

    luaL_checktype(state, 1, LUA_TTABLE);

    // Grab paths
    int num_paths = mii_lua_len(state, -1);

    for (int i = 1; i <= num_paths; ++i)
    {
        lua_rawgeti(state, -i, i);
        out_paths.push_back(lua_tostring(state, -1));
    }

    // Pop rawgeti + table off
    lua_pop(state, num_paths + 1);
}
