-- print(aedif.os.isfile("./build"))
-- print(aedif.os.isfile("./src/lua_utils.c"))
-- print(aedif.os.isdir("./build"))
-- print(aedif.os.isdir("./src/lua_utils.c"))
-- print(aedif.os.isdir("./src/ffi/os/aedif_os_isfile.c"))

aedif.os.rmfile("bar.lua")
aedif.os.rmdir("./foo", true)
aedif.os.rmdir("./bar", true)

