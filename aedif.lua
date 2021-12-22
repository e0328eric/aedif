LANGUAGE = "c"
COMPILER = "gcc"
STD = 11
OPT_LEVEL = 0
WARNINGS = { "all", "extra", "pedantic", "conversion" }
ERRORS = { "return-type" }
FLAGS = { "-ggdb" }

aedif.execute("make -C ./lib/lua-5.4.3/")
aedif.execute("mv ./lib/lua/src/liblua.a ./build/lib")

target = "aedif"
srcs = {
    "./src/main.c",
    "./src/lua_debug.c",
    "./src/conversion.c",
    "./src/aedif_lua_module.c",
    "./src/predefined_vars.c",
    "./src/project_data.c",
}
lib = "lua"
lib_dir = "build/lib"
includes = {
   "lib/lua-5.4.3/src",
   "lib",
}

aedif.compile(target, srcs, lib, lib_dir, includes, nil)

