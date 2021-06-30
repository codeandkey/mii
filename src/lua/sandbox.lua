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

function script_path()
    local str = debug.getinfo(2, "S").source:sub(2)
    return str:match("(.*/)")
end

package.path = script_path() .. '?.lua;' .. package.path
require('utils')

local concatTbl = table.concat

test_env = {
    pathJoin        = pathJoin,
    prepend_path    = handle_path,
    append_path     = handle_path,
    os              = {getenv = os.getenv},
    assert          = assert,
    dofile          = dofile,
    loadfile        = loadfile,
}

local meta_function  = {}
local meta_table     = {}
local fake_func      = {}

meta_function.__index = function (...)
    return function (...)
        return nil
    end
end

meta_function.__call = function (...)
    return ""
end

meta_table.__index = function(...)
    return fake_func
end

setmetatable(fake_func, meta_function)
setmetatable(test_env, meta_table)

--------------------------------------------------------------------------
-- This function is what actually "loads" a modulefile with protection
-- against modulefiles call functions they shouldn't or syntax errors
-- of any kind.
-- @param untrusted_code A string containing lua code

local function run5_1(untrusted_code)
    A = {}
    if untrusted_code:byte(1) == 27 then return nil, "binary bytecode prohibited" end
    local untrusted_function, message = loadstring(untrusted_code)
    if not untrusted_function then return nil, message end
    setfenv(untrusted_function, test_env)
    status, msg = pcall(untrusted_function)
    if (not status) then
        print(msg)
    end
    return concatTbl(A,":")
end

--------------------------------------------------------------------------
-- This function is what actually "loads" a modulefile with protection
-- against modulefiles call functions they shouldn't or syntax errors
-- of any kind. This run codes under environment [Lua 5.2] or later.
-- @param untrusted_code A string containing lua code
local function run5_2(untrusted_code)
    A = {}
    local untrusted_function, message = load(untrusted_code, nil, 't', test_env)
    if not untrusted_function then return nil, message end
    status, msg = pcall(untrusted_function)
    if (not status) then
        print(msg)
    end
    return concatTbl(A,":")
end

--------------------------------------------------------------------------
-- Define two version: Lua 5.1 or 5.2.  It is likely that
-- The 5.2 version will be good for many new versions of
-- Lua but time will only tell.
sandbox_run = (_VERSION == "Lua 5.1") and run5_1 or run5_2