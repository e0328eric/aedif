# aedif
It is a C/C++ building tool like make or cmake but has a minimal features.
Since this is the toy project, it is not recommended to use in real production.

# Building aedif
It can be built in both python script and aedif itself. However, in this moment, it is checked to run only at mac os monterey.
Later, it is planed to execte in both Linux and Windows

## Using a python script
You need a `gcc` to compile it. But other compilers can be used, for example, `clang`.
Just type this:
```console
$ ./cb b
```
or if you install this, run
```console
$ ./cb i
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
