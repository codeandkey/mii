--------------------------------------------------------------------------
-- Lmod License
--------------------------------------------------------------------------
--
--  Lmod is licensed under the terms of the MIT license reproduced below.
--  This means that Lmod is free software and can be used for both academic
--  and commercial purposes at absolutely no cost.
--
--  ----------------------------------------------------------------------
--
--  Copyright (C) 2008-2018 Robert McLay
--
--  Permission is hereby granted, free of charge, to any person obtaining
--  a copy of this software and associated documentation files (the
--  "Software"), to deal in the Software without restriction, including
--  without limitation the rights to use, copy, modify, merge, publish,
--  distribute, sublicense, and/or sell copies of the Software, and to
--  permit persons to whom the Software is furnished to do so, subject
--  to the following conditions:
--
--  The above copyright notice and this permission notice shall be
--  included in all copies or substantial portions of the Software.
--
--  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
--  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
--  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
--  NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
--  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
--  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
--  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
--  THE SOFTWARE.
--
--------------------------------------------------------------------------

local concatTbl = table.concat

local meta_table     = {}
fake_obj             = {}

function fake_func (...)
    return fake_obj
end

function fake_compare (...)
    return true
end

function getenv (...)
    local val = os.getenv(...)
    if val then return val else return "" end
end

-- The bare minimum to check for PATH modifications in modulefiles
test_env = {
    pathJoin        = pathJoin,
    prepend_path    = handle_path,
    append_path     = handle_path,
    os              = {getenv = getenv},
    assert          = assert,
    error           = error,
    ipairs          = ipairs,
    pairs           = pairs,
}

meta_table.__index  = fake_func

-- These functions may get called if the modulefile tries to use a
-- function which isn't defined in the test env
local meta_obj = {
    __index     = fake_func,
    __call      = fake_func,
    __concat    = fake_func,
    __add       = fake_func,
    __eq        = fake_compare,
    __lt        = fake_compare,
    __le        = fake_compare,
}

setmetatable(fake_obj, meta_obj)
setmetatable(test_env, meta_table)

--------------------------------------------------------------------------
-- Load the provided code in a sandbox environment and return it as a
-- function. This function works with Lua 5.1.
-- @param untrusted_code A string containing lua code
local function loadcode5_1(untrusted_code)
    if untrusted_code:byte(1) == 27 then error("binary bytecode prohibited") end
    local untrusted_function, message = loadstring(untrusted_code)
    if not untrusted_function then error(message) end
    setfenv(untrusted_function, test_env)
    return untrusted_function
end

--------------------------------------------------------------------------
-- Load the provided code in a sandbox environment and return it as a
-- function. This function works with Lua 5.2 or higher.
-- @param untrusted_code A string containing lua code
local function loadcode5_2(untrusted_code)
    local untrusted_function, message = load(untrusted_code, nil, 't', test_env)
    if not untrusted_function then error(message) end
    return untrusted_function
end

loadcode = (_VERSION == "Lua 5.1") and loadcode5_1 or loadcode5_2

--------------------------------------------------------------------------
-- Read an entire file and load it's code as a function in a sandbox env.
-- @param filename The name/path of the file to load
function loadfile(filename)
    local file = assert(io.open(filename, "r"))
    local code = file:read("*a")
    file:close()
    return loadcode(code)
end

--------------------------------------------------------------------------
-- Execute a lua file in a sandbox environment.
-- @param filename The name/path of the file to load
function dofile(filename)
    return assert(loadfile(filename))()
end

-- add the functions to the sandbox env
test_env.loadfile = loadfile
test_env.dofile   = dofile

--------------------------------------------------------------------------
-- Load the provided code in a sandbox environment and execute it
-- @param untrusted_code A string containing lua code
function sandbox_run(untrusted_code)
    paths = {}
    loadcode(untrusted_code)()
    return paths
end