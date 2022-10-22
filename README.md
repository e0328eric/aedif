# aedif

It is a C/C++ building tool like make or cmake but has a minimal features.
Since this is the toy project, it is not recommended to use in real production.

<<<<<<< HEAD
It is a header only library.
=======
# Building aedif

It can be built in both python script and aedif itself. However, in this moment, it is checked to run only at macos and linux (especially, Arch Linux).
Later, it is planed to support in Windows.

## Using a C script

You need a `gcc` to compile it. But other compilers can be used, for example, `clang`.
Just type this:

```console
$ cc cb.c -o cb
$ ./cb build
```

or if you install this, run

```console
$ cc cb.c -o cb
$ ./cb install
```

Then the `aedif` binary will be installed at `~/.local/bin`.

## Using aedif itself

See **Usage of aedif**.

# Usage of aedif

It uses the lua script to bootstrap whole project. The lua script must have a name called `aedif.lua`
at the main position of your project.

```console
$ aedif
```

Then executables are in `./build/bin`.
>>>>>>> main
