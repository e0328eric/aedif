-- print(aedif.os.isfile("./build"))
-- print(aedif.os.isfile("./src/lua_utils.c"))
-- print(aedif.os.isdir("./build"))
-- print(aedif.os.isdir("./src/lua_utils.c"))
-- print(aedif.os.isdir("./src/ffi/os/aedif_os_isfile.c"))

-- aedif.os.mkdir("./foo")
-- aedif.os.mkdir("./bar/foo")
-- print(aedif.os.mkdir("./bar/foo/baz"))
-- aedif.os.mkdir("bar/foo/bbar")

-- print(aedif.os.copy("foo.lua", "bar.lua", true))
print(aedif.os.rmfile("bar.lua"))
print(aedif.os.rmdir("./bar"))
