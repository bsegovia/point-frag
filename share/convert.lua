local lfs = require "lfs"

for file in lfs.dir("./") do
  if file ~= "." or file ~= ".." then
    local attr = lfs.attributes(file)
    if attr.mode == "file" then
      local prefix = string.match(file, "(.*)\.jpg")
      if prefix then
        local replace = {[")"] = "\\)", ["("] = "\\("}
        prefix = string.gsub(prefix, "%(", replace)
        prefix = string.gsub(prefix, "%)", replace)
        print("converting "..prefix..".jpg into "..prefix..".tga ...")
        os.execute("convert "..prefix..".jpg "..prefix..".tga")
      end
    end
  end
end

