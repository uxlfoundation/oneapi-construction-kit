# Modules

```{toctree}
:maxdepth: 2

modules/builtins
modules/mux
modules/host
modules/riscv
modules/compiler
modules/loader
modules/vecz
modules/spirv-ll
modules/debug
modules/cargo
modules/metadata
```

The `modules` directory contains a number of sub-directories, the names of these
sub-directories describe generally the functionality of the source code
contained within. Modules are laid out so that, if necessary, they can be split
out into a different stand alone repository or be optionally included;
separation is not required or even desirable in most cases. Modularisation
provides the infrastructure to easily add and remove whole features if they are
not required for a specific application.

## Creating a Module

### Directory Layout

Modules are intended to be able to function as stand alone projects and this is
reflected in their directory layout. Below `<name>` refers to the name of the
module.

```text
<name>/README.md
<name>/CMakeLists.txt
<name>/include/<name>/*.h
<name>/source/*.cpp
<name>/LICENCE.txt          - optional
<name>/doc/*                - optional
<name>/external/*           - optional
<name>/scripts/*            - optional
<name>/test/*               - optional
<name>/tools/*              - optional
```

> Directory paths above are relative to the `modules` directory.

### Code Layout

Source code contained in a module should live in a separate namespace or have a
symbol name prefix, the name used should be the `<name>` of the module. For C++
use the following syntax to denote a modules scope, nested namespaces should be
kept to a minimum preferably reserved for implementation details.

```cpp
namespace <name> {
...
}
```

For C interfaces use the following convention when naming symbols.

```c
<name><symbol>;
```

> Including existing projects as a module

### CMake

All modules must contain `<name>/CMakeLists.txt` describing how to build the
source files, modules are included in the project using the
`add_ocl_subdirectory` function in the `CMakeLists.txt` located in this
directory. The `add_ocl_subdirectory` function sets the variable
`<NAME>_COMPILE_OPTIONS` which contains a list of project wide compiler options,
the module must use these options in order to successfully build the project.
This is the suggested way to setup a modules build interface.

> Note that `PRIVATE` is specified, this denotes that the settings will not be
> propagated to any targets which link against `<name>`.

```cmake
target_compile_options(<name> PRIVATE ${API_COMPILE_OPTIONS})
```

All modules must set target specific options for include directories, compile
options, and link libraries.

> Note that `PUBLIC` is specified, this denotes that the settings will be
> propagated to all targets which link against `<name>`.

```cmake
target_include_directories(<name> PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(<name> PUBLIC <lib0> <lib2> ...)
```

Header only modules may populate the CMake variable `MODULES_INCLUDE_DIRS` to
enabling dependency tracking. This approach is only suggested as a backup when
the suggested approach above is intractable.

```cmake
# Append to the list of module include directories, the cache MUST be updated.
list(APPEND MODULES_INCLUDE_DIRS ${CARGO_INCLUDE_DIR})
set(MODULES_INCLUDE_DIRS ${MODULES_INCLUDE_DIRS}
  CACHE INTERNAL "List of module include directories")
```
