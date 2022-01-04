LANGUAGE = "c"
COMPILER = "gcc"
STD = 11
OPT_LEVEL = 0
WARNINGS = { "all", "extra", "pedantic", "conversion" }
ERRORS = { "return-type" }

if aedif.isclean then
    aedif.execute("make clean -C ./lib/lua-5.4.3/")
else
    -- Compile liblua
    aedif.execute("make -C ./lib/lua-5.4.3/")
    aedif.execute("mv ./lib/lua-5.4.3/src/liblua.a ./build/lib")

    -- Compile libyaml
    --aedif.execute("cd ./lib/libyaml/")

    target = "aedif"
    srcs = {
      "./src/main.c",
      "./src/parse_args.c",
      "./src/lua_utils.c",
      "./src/ffi/os/aedif_os_isfile.c",
      "./src/ffi/os/aedif_os_isdir.c",
      "./src/ffi/link_lua.c",
    }
    if aedif.ostype == "macos" then
       lib = "lua"
    elseif aedif.ostype == "linux" then
       lib = {
           "lua",
           "m",
           "dl",
       }
    end
    lib_dir = "build/lib"
    includes = {
      "lib/lua-5.4.3/src",
      "lib",
      "src",
      "src/ffi/os",
    }

    aedif.compile(target, srcs, lib, lib_dir, includes, nil)
end
