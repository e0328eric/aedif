pub mod os;

use mlua::{prelude::*, FromLua, Value};

pub fn link_aedif_core<'lua>(lua: &'lua Lua, aedif_table: &LuaTable<'lua>) -> LuaResult<()> {
    aedif_table.set("compile_obj", lua.create_function(aedif_compile_obj)?)?;

    Ok(())
}

#[repr(u8)]
#[derive(Debug)]
enum Language {
    C(CStandard),
    Cpp(CppStandard),
}

impl<'lua> FromLua<'lua> for Language {
    fn from_lua(value: LuaValue<'lua>, lua: &'lua Lua) -> LuaResult<Self> {
        match value {
            Value::Table(lang) => {
                let lang_type: String = lang.get("langtype")?;
                match lang_type.as_str() {
                    "c" | "C" => {
                        let lang_std = CStandard::from_lua(lang.get("std")?, &lua)?;
                        Ok(Self::C(lang_std))
                    }
                    "cpp" | "CPP" | "Cpp" | "c++" | "C++" => {
                        let lang_std = CppStandard::from_lua(lang.get("std")?, &lua)?;
                        Ok(Self::Cpp(lang_std))
                    }
                    _ => Err(LuaError::runtime("invalid language type found")),
                }
            }
            Value::String(lang) => match lang.to_str()? {
                "c" | "C" => Ok(Self::C(CStandard::C11)),
                "cpp" | "CPP" | "Cpp" | "c++" | "C++" => Ok(Self::Cpp(CppStandard::Cpp17)),
                _ => Err(LuaError::runtime("invalid language type found")),
            },
            _ => Err(LuaError::runtime("invalid language specifier")),
        }
    }
}

#[repr(u8)]
#[derive(Debug)]
enum Compiler {
    Gcc,
    Clang,
    Msvc,
    Mingw,
}

impl<'lua> FromLua<'lua> for Compiler {
    fn from_lua(value: Value<'lua>, lua: &'lua Lua) -> LuaResult<Self> {
        const ERR_MSG: &str = "invalid compiler name found";

        match value {
            #[cfg(target_os = "linux")]
            Value::Nil => Ok(Self::Gcc),
            #[cfg(target_os = "macos")]
            Value::Nil => Ok(Self::Clang),
            #[cfg(target_os = "windows")]
            Value::Nil => Ok(Self::Msvc),
            Value::String(value) => Ok(match value.to_str()? {
                "gcc" => Self::Gcc,
                "clang" => Self::Clang,
                "msvc" => Self::Msvc,
                "mingw" => Self::Mingw,
                _ => return Err(LuaError::runtime(ERR_MSG)),
            }),
            _ => Err(LuaError::runtime(ERR_MSG)),
        }
    }
}

#[repr(u8)]
#[derive(Debug)]
enum CStandard {
    C89,
    C90,
    C99,
    C11,
    C17,
    Gnu89,
    Gnu90,
    Gnu99,
    Gnu11,
    Gnu17,
}

impl<'lua> FromLua<'lua> for CStandard {
    fn from_lua(value: Value<'lua>, lua: &'lua Lua) -> LuaResult<Self> {
        const ERR_MSG: &str = "invalid c standard name found";

        if let Value::String(value) = value {
            Ok(match value.to_str()? {
                "c89" => Self::C89,
                "c90" => Self::C90,
                "c99" => Self::C99,
                "c11" => Self::C11,
                "c17" => Self::C17,
                "gnu89" => Self::Gnu89,
                "gnu90" => Self::Gnu90,
                "gnu99" => Self::Gnu99,
                "gnu11" => Self::Gnu11,
                "gnu17" => Self::Gnu17,
                _ => return Err(LuaError::runtime(ERR_MSG)),
            })
        } else {
            Err(LuaError::runtime(ERR_MSG))
        }
    }
}

#[repr(u8)]
#[derive(Debug)]
enum CppStandard {
    Cpp98,
    Cpp11,
    Cpp14,
    Cpp17,
    Cpp20,
    Cpp23,
    Gnupp98,
    Gnupp11,
    Gnupp14,
    Gnupp17,
    Gnupp20,
    Gnupp23,
}

impl<'lua> FromLua<'lua> for CppStandard {
    fn from_lua(value: Value<'lua>, lua: &'lua Lua) -> LuaResult<Self> {
        const ERR_MSG: &str = "invalid c standard name found";

        if let Value::String(value) = value {
            Ok(match value.to_str()? {
                "c++98" => Self::Cpp98,
                "c++11" => Self::Cpp11,
                "c++14" => Self::Cpp14,
                "c++17" => Self::Cpp17,
                "c++20" => Self::Cpp20,
                "c++23" => Self::Cpp23,
                "gnu++98" => Self::Gnupp98,
                "gnu++11" => Self::Gnupp11,
                "gnu++14" => Self::Gnupp14,
                "gnu++17" => Self::Gnupp17,
                "gnu++20" => Self::Gnupp20,
                "gnu++23" => Self::Gnupp23,
                _ => return Err(LuaError::runtime(ERR_MSG)),
            })
        } else {
            Err(LuaError::runtime(ERR_MSG))
        }
    }
}

#[derive(Debug)]
struct Project {
    lang: Language,
    compiler: Compiler,
}

impl<'lua> FromLua<'lua> for Project {
    fn from_lua(value: Value<'lua>, lua: &'lua Lua) -> LuaResult<Self> {
        if let Value::Table(value) = value {
            let lang = Language::from_lua(value.get("lang")?, lua)?;
            let compiler = Compiler::from_lua(value.get("compiler")?, lua)?;
            Ok(Self { lang, compiler })
        } else {
            Err(LuaError::runtime("invalid project type"))
        }
    }
}

fn aedif_compile_obj(_lua: &'_ Lua, (project, src): (Project, String)) -> LuaResult<()> {
    println!("Project: {project:?}");
    println!("src: {src}");

    Ok(())
}
