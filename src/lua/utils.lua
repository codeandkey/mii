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

local getenv    = os.getenv
local TILDE     = getenv("HOME") or "~"
local concatTbl = table.concat

local function argsPack(...)
   local argA = { n = select("#", ...), ...}
   return argA
end
local pack    = (_VERSION == "Lua 5.1") and argsPack or table.pack  -- luacheck: compat

--------------------------------------------------------------------------
-- An iterator to loop split a pieces.  This code is from the
-- lua-users.org/lists/lua-l/2006-12/msg00414.html
-- @param self input string
-- @param pat pattern to split on.

function string.split(self, pat)
   pat  = pat or "%s+"
   local st, g = 1, self:gmatch("()("..pat..")")
   local function getter(myself, segs, seps, sep, cap1, ...)
      st = sep and seps + #sep
      return myself:sub(segs, (seps or 0) - 1), cap1 or sep, ...
   end
   local function splitter(myself)
      if st then return getter(myself, st, g()) end
   end
   return splitter, self
end

--------------------------------------------------------------------------
-- Join argument into a path that has single slashes between directory
-- names and no trailing slash.
-- @return a file path with single slashes between directory names
-- and no trailing slash.

function string.trim(self)
   local ja = self:find("%S")
   if (ja == nil) then
      return ""
   end
   local  jb = self:find("%s+$") or 0
   return self:sub(ja,jb-1)
end

function pathJoin(...)
   local a    = {}
   local argA = pack(...)
   for i = 1, argA.n  do
      local v = argA[i]
      if (v and v ~= '') then
         local vType = type(v)
         if (vType ~= "string") then
            local msg = "bad argument #" .. i .." (string expected, got " .. vType .. " instead)\n"
            assert(vType ~= "string", msg)
         end
         v = v:trim()
         if (v:sub(1,1) == '/' and i > 1) then
      if (v:len() > 1) then
         v = v:sub(2,-1)
      else
         v = ''
      end
         end
         v = v:gsub('//+','/')
         if (v:sub(-1,-1) == '/') then
      if (v:len() > 1) then
         v = v:sub(1,-2)
      elseif (i == 1) then
         v = '/'
            else
         v = ''
      end
         end
         if (v:len() > 0) then
      a[#a + 1] = v
   end
      end
   end
   local s = concatTbl(a,"/")
   s = path_regularize(s)
   return s
end

--------------------------------------------------------------------------
-- Remove leading and trail spaces and extra slashes.
-- @param value A path
-- @return A clean canonical path.
function path_regularize(value, full)
   if value == nil then return nil end
   local doubleSlash = value:find("[^/]//$")
   value = value:gsub("^%s+", "")
   value = value:gsub("%s+$", "")
   value = value:gsub("//+" , "/")
   value = value:gsub("/%./", "/")
   value = value:gsub("/$"  , "")
   value = value:gsub("^~"  , TILDE)
   if (value == '') then
      value = ' '
      return value
   end
   local a    = {}
   local aa   = {}
   for dir in value:split("/") do
      aa[#aa + 1] = dir
   end

   local first  = aa[1]
   local icnt   = 2
   local num    = #aa
   if (first == ".") then
      for i = 2, num do
         if (aa[i] ~= ".") then
            icnt = i
            break
         else
            icnt = icnt + 1
         end
      end
      a[1] = (icnt > num) and "." or aa[icnt]
      icnt = icnt + 1
   else
      a[1] = first
   end

   if (full) then
      for i = icnt, #aa do
         local dir  = aa[i]
         local prev = a[#a]
         if (    dir == ".." and prev ~= "..") then
            a[#a] = nil
         elseif (dir ~= ".") then
            a[#a+1] = dir
         end
      end
   else
      for i = icnt, #aa do
         local dir  = aa[i]
         local prev = a[#a]
         if (dir ~= ".") then
            a[#a+1] = dir
         end
      end
   end

   value = concatTbl(a,"/")

   if (doubleSlash) then
      value = value .. '//'
   end

   return value
end

local function convert2table(...)
   local argA = pack(...)
   local t    = {}

   if (argA.n == 1 and type(argA[1]) == "table" ) then
      t = argA[1]
      t[1] = t[1]:trim()
   else
      t[1]    = argA[1]:trim()
      t[2]    = argA[2]
      t.delim = argA[3]
   end
   t.priority = tonumber(t.priority or "0")
   return t
end

function handle_path(...)
   local t = convert2table(...)
   if t[1] == "PATH" then
      A[#A+1] = t[2]
   end
end