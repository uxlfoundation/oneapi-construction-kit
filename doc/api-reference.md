# API Reference

Here you will find documentation generated from ComputeAorta's source code.

* [`cl` module](#cl-module)
  * [`compiler` module](#compiler-module)
  * [`extension` module](#extension-module)
* [`cargo` module](#cargo-module)
* [`vecz` module](#vecz-module)
* [`builtins` module](#builtins-module)
  * [`abacus`](#abacus)
  * [`libimg`](#libimg)
* [`core` module](#core-module)
  * [`compiler::utils`](#compilerutils)
* [`host` core target](#host-core-target)
* [`cross` core target](#cross-core-target)
* [`tracer` module](#tracer-module)

* {ref}`genindex`

## `cl` module

```{eval-rst}
.. doxygengroup:: cl
    :members:
```

### `compiler` module

The OpenCL C frontend compiler and a set of LLVM passes to transform the
generated LLVM IR before hand off to [core](#core-module) are contained in the
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

## `core` module

```{eval-rst}
.. doxygengroup:: core
    :members:
```

### `compiler::utils`

A set of C++ utilities are provides along side [core](#core-module), whilst
it is not required to make use of them they prove useful when implementing and
maintaining an implementation of the API for a specific device.

```{eval-rst}
.. doxygengroup:: utils
    :members:
```

### `host` core target

```{eval-rst}
.. doxygengroup:: host
    :members:
```

### `cross` core target

A [core](#core-module) target for offline and cross-compilation of kernels which
does not support execution of kernels or other runtime commands. Whilst the
entire set of core API entry points is defined all entry points will return
`core_error_feature_unsupported` _except_ those listed here.

```{eval-rst}
.. doxygenfunction:: crossGetDeviceInfos
.. doxygenfunction:: crossCreateFinalizer
.. doxygenfunction:: crossDestroyFinalizer
.. doxygenfunction:: crossCreateBinaryFromSource
.. doxygenfunction:: crossDestroyBinary
```

## `tracer` module

```{eval-rst}
.. doxygengroup:: tracer
    :members:
```
