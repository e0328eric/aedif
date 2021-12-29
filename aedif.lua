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
      "./src/lua_utils.c",
      "./src/conversion.c",
      "./src/aedif_lua_module.c",
      "./src/predefined_vars.c",
      "./src/project_data.c",
      "./src/build_data.c",
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
    }

    aedif.compile(target, srcs, lib, lib_dir, includes, nil)
end
