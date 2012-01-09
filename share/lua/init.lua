local ffi = require "ffi"
pf = {
  cv = { },
}

setmetatable(pf.cv, {
  __index = function (table, key)
    local str = "cvarGet_" .. key
    if ffi.C[str] == nil then
      error("cvar " .. key .. " unknown")
    else
      local check_string = "cvarIsString_" .. key
      assert(ffi.C[check_string] ~= nil)
      if ffi.C[check_string]() == 0 then
        return ffi.C[str]()
      else
        return ffi.string(ffi.C[str]())
      end
    end
  end,
  __newindex = function (table, key, value)
    local str = "cvarSet_" .. key
    if ffi.C[str] == nil then
      error("cvar " .. key .. " unknown")
    else
      return ffi.C[str](value)
    end
  end})

