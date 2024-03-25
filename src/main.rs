use std::path::Path;

mod aedif_std;

use mlua::prelude::*;

fn main() -> LuaResult<()> {
    let lua = Lua::new();

    let aedif_std = lua.create_table()?;

    // link aedif core
    aedif_std::link_aedif_core(&lua, &aedif_std)?;

    // link aedif.os
    aedif_std::os::link_aedif_std_os(&lua, &aedif_std)?;

    lua.globals().set("aedif", aedif_std)?;

    lua.load(Path::new("aedif.lua")).exec()
}
