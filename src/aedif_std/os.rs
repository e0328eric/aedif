use std::fs;
use std::path::{Path, PathBuf};

use mlua::prelude::*;

pub fn link_aedif_std_os<'lua>(lua: &'lua Lua, aedif_table: &LuaTable<'lua>) -> LuaResult<()> {
    let table = lua.create_table()?;

    table.set("mkdir", lua.create_function(aedif_mkdir)?)?;
    table.set("mkdir_all", lua.create_function(aedif_mkdir_all)?)?;
    table.set("remove", lua.create_function(aedif_remove)?)?;
    table.set("concat", lua.create_function(aedif_concat)?)?;

    aedif_table.set("os", table)?;

    Ok(())
}

fn aedif_mkdir(_lua: &'_ Lua, dirname: String) -> LuaResult<()> {
    Ok(fs::create_dir(dirname)?)
}

fn aedif_mkdir_all(_lua: &'_ Lua, dirname: String) -> LuaResult<()> {
    Ok(fs::create_dir_all(dirname)?)
}

fn aedif_remove(lua: &'_ Lua, (name, kind): (String, String)) -> LuaResult<()> {
    match kind.as_str() {
        "f" | "file" => fs::remove_file(name)?,
        "d" | "dir" => fs::remove_dir(name)?,
        "r" => fs::remove_dir_all(name)?,
        _ => lua.warning("invalid second parameter. do nothing...", false),
    }

    Ok(())
}

fn aedif_concat(_lua: &'_ Lua, paths: Vec<String>) -> LuaResult<String> {
    let mut output = PathBuf::with_capacity(50);

    for path in paths.iter().map(Path::new) {
        output.push(path);
    }

    Ok(output.to_string_lossy().to_string())
}
