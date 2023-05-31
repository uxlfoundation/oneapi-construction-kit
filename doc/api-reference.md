# API Reference

Here you will find documentation generated from the oneAPI Construction Kit's source
code.

* [`cl` module](#cl-module)
  * [`compiler` module](#compiler-module)
  * [`extension` module](#extension-module)
* [`cargo` module](#cargo-module)
* [`vecz` module](#vecz-module)
* [`builtins` module](#builtins-module)
  * [`abacus`](#abacus)
  * [`libimg`](#libimg)
* [`mux` module](#mux-module)
  * [`compiler::utils`](#compilerutils)
* [`host` mux target](#host-mux-target)
* [`tracer` module](#tracer-module)

* {ref}`genindex`

## `cl` module

```{eval-rst}
.. doxygengroup:: cl
    :members:
```

### `compiler` module

The OpenCL C frontend compiler and a set of LLVM passes to transform the
generated LLVM IR before hand off to [mux](#mux-module) are contained in the
[compiler module](#compiler-module).

```{eval-rst}
.. doxygengroup:: cl_compiler
    :members:
```

### `extension` module

```{eval-rst}
.. doxygengroup:: cl_extension
    :members:
```

## `cargo` module

```{eval-rst}
.. doxygengroup:: cargo
    :members:
```

## `vecz` module

```{eval-rst}
.. doxygengroup:: vecz
    :members:
```

## `builtins` module

```{eval-rst}
.. doxygengroup:: builtins
    :members:
```

### `abacus`

#### `abacus cast`

```{eval-rst}
.. doxygengroup:: abacus_cast
```

#### `abacus common`

```{eval-rst}
.. doxygengroup:: abacus_common
```

#### `abacus config`

```{eval-rst}
.. doxygengroup:: abacus_config
```

#### `abacus extra`

```{eval-rst}
.. doxygengroup:: abacus_extra
```

#### `abacus geometric`

```{eval-rst}
.. doxygengroup:: abacus_geometric
```

#### `abacus integer`

```{eval-rst}
.. doxygengroup:: abacus_integer
```

#### `abacus math`

```{eval-rst}
.. doxygengroup:: abacus_math
```

#### `abacus memory`

```{eval-rst}
.. doxygengroup:: abacus_memory
```

#### `abacus misc`

```{eval-rst}
.. doxygengroup:: abacus_misc
```

#### `abacus relational`

```{eval-rst}
.. doxygengroup:: abacus_relational
```

#### `abacus type traits`

```{eval-rst}
.. doxygengroup:: abacus_type_traits
```

### `libimg`

```{eval-rst}
.. doxygengroup:: libimg
```

## `mux` module

```{eval-rst}
.. doxygengroup:: mux
    :members:
```

### `compiler::utils`

A set of C++ utilities are provides along side [mux](#mux-module), whilst
it is not required to make use of them they prove useful when implementing and
maintaining an implementation of the API for a specific device.

```{eval-rst}
.. doxygengroup:: utils
    :members:
```

### `host` mux target

```{eval-rst}
.. doxygengroup:: host
    :members:
```

## `tracer` module

```{eval-rst}
.. doxygengroup:: tracer
    :members:
```
