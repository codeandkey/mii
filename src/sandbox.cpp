#include "sandbox.h"
#include "options.h"
#include "util.h"

#include <fstream>
#include <iterator>
#include <stdexcept>

#include <lua.hpp>

/* After Lua 5.1, lua_objlen changed name and LUA_OK is defined */
#if LUA_VERSION_NUM <= 501
#define mii_lua_len lua_objlen
#define LUA_OK 0
#else
#define mii_lua_len lua_rawlen
#endif

using namespace mii;
using namespace std;

static lua_State* state = nullptr;

void sandbox::init()
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
    state = nullptr;
    throw runtime_error("Failed to execute sandbox file");
}

void sandbox::cleanup()
{
    if (state)
        lua_close(state);

    state = nullptr;
}

void sandbox::analyze(string path, vector<string>& out_paths, vector<string>& out_mpaths)
{    
    if (!state)
        sandbox::init();

    ifstream f(path, ios::binary | ios::ate);

    if (!f)
        throw runtime_error("Failed to open " + path + " for reading");

    // Read data into buf
    streamsize sz = f.tellg();
    vector<char> code;

    f.seekg(0, ios::beg);
    code.resize(sz);
    f.read(code.data(), sz);
    code.push_back('\0');

    // Prep sandbox function
    lua_getglobal(state, "sandbox_run");
    lua_pushstring(state, code.data());

    // Execute modulefile
    if(lua_pcall(state, 1, 2, 0) != LUA_OK)
        throw runtime_error("Error occurred in Lua sandbox : " + string(lua_tostring(state, -1)));

    if (lua_type(state, -1) != LUA_TTABLE)
        throw runtime_error(string("Sandbox path returned non-table: ") + lua_typename(state, lua_type(state, -1)));

    if (lua_type(state, -2) != LUA_TTABLE)
        throw runtime_error(string("Sandbox mpath returned non-table: ") + lua_typename(state, lua_type(state, -2)));

    // Grab paths
    int num_paths = mii_lua_len(state, -1);

    for (int i = 1; i <= num_paths; ++i)
    {
        lua_rawgeti(state, -1, i);
        out_paths.push_back(lua_tostring(state, -1));
        lua_pop(state, 1);
    }

    // Pop pathtable
    lua_pop(state, 1);

    // Grab mpaths
    int num_mpaths = mii_lua_len(state, -1);

    for (int i = 1; i <= num_mpaths; ++i)
    {
        lua_rawgeti(state, -1, i);
        out_mpaths.push_back(lua_tostring(state, -1));
        lua_pop(state, 1);
    }

    // Pop mpath table
    lua_pop(state, 1);
}
