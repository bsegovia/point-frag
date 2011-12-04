-- we extensively use FFI for all the console stuff
local ffi = require "ffi"
pf = {
  cv = { },
}

setmetatable(pf.cv, {
  __index = function (table, key)
    local str = "cvar_get_" .. key
    if ffi.C[str] == nil then
      error("cvar " .. key .. " unknown")
    else
      return ffi.C[str]()
    end
  end,
  __newindex = function (table, key, value)
    local str = "cvar_set_" .. key
    if ffi.C[str] == nil then
      error("cvar " .. key .. " unknown")
    else
      return ffi.C[str](value)
    end
  end})


