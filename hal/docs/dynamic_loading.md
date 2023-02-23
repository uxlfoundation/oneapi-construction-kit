# Dynamic Loading of Device HALs

Device HALs are currently statically linked to the generic RISC-V Core target.
The HAL can be switched at build time by selecting a device in CMake. This
requires HAL implementers to have access to the ComputeAorta source and build it
themselves. This document describes a mechanism for switching between device
HALs at runtime, which will allow for binary releases of ComputeAorta.

## Building Device HALs

Device HALs will keep their current API and be instantiated using the
`hal::create_hal` entry point. They will be built as shared libraries, either
in-tree or out-of-tree. They can be built independently, or as part of `clik` and
`ComputeAorta`.

## Loading Device HALs

The generic Core target will load a device HAL at runtime using `dlopen`
(`LoadLibrary` on Windows). The name of the default device to load is configured
through a CMake variable (`CA_HAL_DEFAULT_DEVICE`) and passed as a compiler
definition to the generic target. The name of the shared library to be loaded is
`libhal_${DEVICE_NAME}.so` and it is expected to be in the same directory as
ComputeAorta libraries containing the generic target (e.g. `libCL.so` and
`libcore.so`). Please note that the name passed to `CA_HAL_DEFAULT_DEVICE`
should not include the `hal_` prefix, the `lib` prefix or `.so` (`.dll`)
suffixes.

In order to locate the device HAL to load, the `DT_RUNTIME` field will be used
when linking ELF shared objects that will load device HALs (e.g. it can be set
to `$ORIGIN` by passing the `-Wl,-rpath,$ORIGIN` option to GCC). This should not
be necessary on Windows, where the current directory is included by default in
the library search path.

The name of the device HAL to load can be overriden at runtime using an
environment variable (`CA_HAL_DEVICE`). This variable can contain a device name
(similar to `CA_HAL_DEFAULT_DEVICE`) or an absolute path to the shared library
to load.

The following search order is to be used to locate the HAL library:

1. If the `CA_HAL_DEVICE` environment variable is set to an absolute path, that
   path is used.
2. If the `CA_HAL_DEVICE` environment variable is set to something else,
   `libhal_${CA_HAL_DEVICE}.so` (`hal_${CA_HAL_DEVICE}.dll`) is used.
3. If the `CA_HAL_DEFAULT_DEVICE` CMake variable is set,
   `libhal_${CA_HAL_DEFAULT_DEVICE}.so` (`hal_${CA_HAL_DEFAULT_DEVICE}.dll`) is
   used.
4. If the HAL library still cannot be loaded, the generic target's
   initialization fails.

Please note that in the previous steps, failure to load the library using a path
specified for a given step does not prevent the subsequent steps from being
attempted.

The ability to select a different device HAL through the `CA_HAL_DEVICE`
environment variable can be disabled at build time using the
`CA_HAL_LOCK_DEVICE_NAME` CMake variable. Setting this option to `TRUE` results
in steps 1 and 2 from the list above being skipped.

## Device identification

There is currently no feedback about which HAL is in use through the OpenCL API.
It could be useful to extend the HAL API to allow querying the name of the HAL
device and include it in the device name reported by OpenCL.
