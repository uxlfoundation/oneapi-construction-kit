# Developer Guide

## Supported Software

oneAPI Construction Kit depends on LLVM and Clang. The compiler information
section contains a list of [supported
versions](overview/compiler/supported-llvm-versions.rst).

LLVM and Clang need to be built and installed separately. The oneAPI Construction
Kit build then uses this installation as a dependency.

## Directory layout

Current directory layout:

* `source`: Contains source specific to implementing open standards, such as
  OpenCL and Vulkan.
  * `source/cl`: Encapsulates all source code which implements the OpenCL open
    standard, this is an optional component and may not be present dependent on
    license agreement.
    * `source/cl/external/OpenCL-Headers/include`: This directory holds the
      upstream headers in the sub-directory `external` (for example `CL/cl.h`).
    * `source/cl/source/extension/include`: This directory holds the headers
      for the Codeplay specific OpenCL extensions.
  * `source/vk`: Encapsulates all source code which implements the Vulkan open
    standard, this is an optional component and may not be present dependent on
    license agreement.
* `modules`: Contains shared _modules_ used in the implementation of the open
  standards in `source`. For details on the _modules_ layout in the [modules
  documentation](modules.md). Currently we have the following modules:
  * `modules/cargo`: Generic module containing the implementations of
    containers.
  * `modules/compiler`: Module providing the main compilation infrastructure, which
    in turn contains the following parts:
    * `builtins`: Module providing the OpenCL C builtins, math library (abacus)
      and the image library (libimg) are part of this module.
    * `cookie` contains template files for generating target implementations.
    * `targets` contains target-specific compilation pipeline infrastructure,
      including the `host` device implementation.
    * `tools` contains the `muxc` executable.
    * `utils` contains various common code for transforming IR and metadata.
    * `vecz`: contains the Vectorizer.
  * `modules/mux`: Definition of the Mux API.
* `doc`: Contains documentation in a structure matching the `source` and
  `modules` directory trees.
* `cmake`: Hold various utility CMake scripts.
* `platform`: Holds CMake toolchains files for the various supported platforms,
  as well as all the necessary information to build the oneAPI Construction Kit
  for the specific platforms.
* `scripts`: Holds various scripts used by the oneAPI Construction Kit,
  includes an OpenCL CTS runner, performance analysis scripts, scripts to help
  with building, and scripts used in continuous integration.

## Branches

The two long running branches are:

* `stable`: This branch is merged to on a successful nightly run and should not be merged into directly.
* `main`: This is the main branch for on-going development.

> No force pushes are allowed on these two branches.

## Coding style

### C++ Style

All the C++ code in the oneAPI Construction Kit should be formatted using
[`clang-format`][clang-format] version 16, and using the `.clang-format` file
provided by the project (option: `-style=file`).

Header include guards should follow the convention of
`<MODULE>_<FILE>_H_INCLUDED`.

[clang-format]: https://clang.llvm.org/docs/ClangFormat.html

### CMake Style

All the CMake scripts in the oneAPI Construction Kit should be free of
[`cmakelint`][cmakelint] warnings when the following checks are disabled in the `.cmakelintrc`
configuration file:

[cmakelint]: https://pypi.org/project/cmakelint/

* `linelength` when writing CMake best effort should be made to keep lines
  shorter than 80 characters, however this is not always practical or possible,
  the check is disabled.
* `syntax` it is possible for [cmakelint][cmakelint] to generate some false
  positives when checking CMake syntax, configuring a build with CMake itself is
  the best method of validating syntax, the check is disabled.
* `convention/filename`/`package/consistency`/`package/stdargs` these checks are
  more strict than CMake's documentation suggests, they also trigger warnings in
  the modules provided by CMake itself, the checks are disabled.

To run `cmakelint` using the `.cmakelintrc` configuration file in the root of
the oneAPI Construction Kit repository:

```console
$ cmakelint --config=.cmakelintrc <file> [<file>] ...
```

### Python Style

All the Python code in the oneAPI Construction Kit *must* be formatted using
[`yapf`][yapf] set to the [pep8][pep8] style (default). As with `clang-format`
it's not perfect in all situations and occasionally does something baffling,
but the consistency mostly keeps the holy warriors at bay.

You may also wish to run Python code run through the [pylint][pylint]
(configured in `.pylintrc`) and [flake8][flake8] linters (configured in
`.flake8`), though these tools can be noisy and encourage somewhat ugly
suggestions - so they are not a pre-merge requirement. As with any linter they
can catch errors such as use of undefined names - so we encourage that you use
them as part of your development flow.

Python code must support Python 3.6.+ version streams. Old code can be updated
using the `futurize` command line tool from the [future][future] package to find
issues and suggested solutions.

[yapf]: https://pypi.org/project/yapf/
[pep8]: https://www.python.org/dev/peps/pep-0008/
[isort]: https://pypi.org/project/isort/
[pylint]: https://pypi.org/project/pylint/
[flake8]: https://pypi.org/project/flake8/
[future]: https://pypi.org/project/future/

## Khronos OpenCL ICD

The Khronos ICD allows multiple OpenCL implementations to coexist in the same
system, these implementations will usually be exposed to the OpenCL user as
individual `cl_platform_id`'s. To inform the system's OpenCL ICD where to find
the oneAPI Construction Kit OpenCL driver it needs to be registered. Note that we also
support fetching and building an ICD within the toolkit through cmake options as described
[here](/source/cl/icd-loader.rst).

### Linux Registration

On Linux the ICD looks for all files with the `.icd` extension in the
`/etc/OpenCL/vendors` directory. To register the oneAPI Construction Kit
OpenCL driver issue the following command replacing `<install_dir>` with the
path to the oneAPI Construction Kit install and `<bits>` bit width of the
binary. The command will likely require root privileges.

```sh
echo <install_dir>/lib/libCL.so > /etc/OpenCL/vendors/<any_name>.icd
```

### Windows Registration

For Windows the ICD inspects the registry so to register the oneAPI Construction
Kit OpenCL driver a new registry entry must be added. Add a `REG_DWORD` value
to the appropriate registry path.

*   32-bit - `HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Khronos\OpenCL\Vendors`
*   64-bit - `HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\OpenCL\Vendors`

The `REG_DWORD` value's name should be the path to the oneAPI Construction Kit
OpenCL driver and its data should be `0`.

### Android Registration

To register the oneAPI Construction Kit OpenCL driver with the Android ICD
follow the same instructions as Linux except that the directory containing the
`.icd` file should be `/system/vendor/Khronos/OpenCL/vendors/`.

```sh
echo <install_dir>/lib/libCL.so > /system/vendor/Khronos/OpenCL/vendors/<any_name>.icd
```

### Building ICD From Source

In order to test the oneAPI Construction Kit OpenCL driver on systems which
do not already have an installed ICD we must first build it from source. Due to
the binary redistribution clause in the license oneAPI Construction Kit does not
bundle the Khronos ICD, it is however open source and can be cloned then built
from the public [GitHub](https://github.com/KhronosGroup/OpenCL-ICD-Loader) repository.

The Khronos ICD does not provide the OpenCL headers files, those are also
located on [GitHub](https://github.com/KhronosGroup/OpenCL-Headers).

```sh
git clone https://github.com/KhronosGroup/OpenCL-ICD-Loader.git
git clone https://github.com/KhronosGroup/OpenCL-Headers.git
```

The ICD expects to be built against the latest version of the OpenCL headers
which at the time of writing is OpenCL 2.2. This is fine since the ICD is
backwards compatible. We must symlink or copy the OpenCL headers into the
`./inc` directory of the ICD repository.

```sh
ln -s OpenCL-Headers/opencl22/CL OpenCL-ICD-Loader/inc/CL
```

Now you can follow the instructions on how to build the ICD as seen in the
[README](https://github.com/KhronosGroup/OpenCL-ICD-Loader/blob/master/README.txt).

## CMake

Help regarding the CMake options and targets available in the oneAPI
Construction Kit when building the project. For advice on modifying the CMake
code itself see our [CMake Development](cmake) documentation.

### CMake Flags

The flags used when invoking CMake on the command line which are used in the
examples shown later in this document.

- `-B<path>`: An undocumented option which creates a build directory `<path>` if
  it does not already exist then configures the build system in that directory.
  It is important to specify the source directory position argument otherwise
  you will see unexpected behaviour.
- `-G<generator>`: Specifies the build system generator to use, when not
  specified the platform specific default generator is used.
- `-D<variable>=<value>`: Defines a CMake option stored in `CMakeCache.txt` to
  control how CMake configures the build directory.

### CMake Options

The builtin CMake options used when invoking CMake on the command line.

- `CMAKE_BUILD_TYPE`: CMake provides a default set of build types:
  - `Debug`: Enable debug symbols and disable optimizations.
  - `Release`: Enable optimizations and disable assertions.
  - `RelWithDebInfo`: Enable debug symbols, optimizations, and disable
    assertions.
  - `MinSizeRel`: Enable size optimizations and disable assertions.
- `CMAKE_INSTALL_PREFIX`: Path to write files produced by the `install` target.
- `CMAKE_TOOLCHAIN_FILE`: Path to a CMake script, used to cross-compile a
  project, which defines variables that inform CMake where the compiler,
  assembler, linker, etc. for the target platform reside.

#### oneAPI Construction Kit CMake Options

- `CMAKE_BUILD_TYPE`: In addition to the defaults provided by CMake the oneAPI
  Construction Kit extends the builtin build types:
  - `ReleaseAssert`: Enable assertions is a Release build.
- `CA_USE_SANITIZER`: Enable support for dynamic analysis sanitizers:
  - `Address`: Enable [AddressSanitizer][asan] dynamic analysis for
    memory errors.
  - `Thread`: Enable [ThreadSanitizer][tsan] dynamic analysis for data
    races.
  - `Undefined`: Enable [UndefinedBehaviourSanitizer][ubsan] dynamic
    analysis for undefined behaviour. This is currently broken with gcc; use
    clang for working ubsan support (CA-4237).
  - `Address,Undefined`: Enable combined [AddressSanitizer][asan] and
    [UndefinedBehaviourSanitizer][ubsan] dynamic analysis.
  - `Fuzzer`: Enable [libFuzzer][libfuzzer] instrumentation.
- `CA_LLVM_INSTALL_DIR`: Tells the oneAPI Construction Kit to use the LLVM
  and Clang installation that can be found at this prefix. The LLVM and Clang
  installations must be development installations i.e. they must contain the
  relevant llvm headers and support tools, and their version must match
  a supported LLVM version.
- `CA_ENABLE_API`: Semi-colon separated list of APIs to enable. Valid values
  are `cl` for OpenCL, and `vk` for Vulkan. Enabling an API when an optional
  component is not present dependent on license agreement will result in a CMake
  error. The default is `cl;vk`.
- `CA_BUILD_32_BITS`: Enable compiling in 32-bit mode on Linux, this requires
  to have the proper 32-bit toolchain installed. When used in combination with
  an external LLVM, the external LLVM also needs to be built in 32-bit mode.
- `CA_EXTERNAL_BUILTINS_DIR` is used to specify the directory
  containing pre-generated builtins. This option is mandatory when cross
  compiling. It is usually set to the `modules/builtins` directory in the build
  directory of a host oneAPI Construction Kit build, but can be set to another
  directory as long as it contains generated builtins.
- `CA_EXTERNAL_BUILTINS`: This option is used to specify whether or not builtins
  should be generated. If it is set to `OFF`, `CA_EXTERNAL_BUILTINS_DIR` must be
  provided to indicate which builtins to use instead. This option is set to `ON`
  for cross compile builds.
- `CA_BUILTINS_TOOLS_DIR`: This options makes it possible to specify which tools
  to use in order the build the builtins, executables for the correct versions
  of `clang` and `llvm-link` must be found in this directory. This can also be
  used for cross-compile builds in which case the tools must work on the host.
- `CA_RUNTIME_COMPILER_ENABLED`: This option determines whether the oneAPI
  Construction Kit is built with or without a runtime compiler (LLVM). It
  defaults to `ON`. Without a runtime compiler, only pre-compiled binaries can
  be run, and the oneAPI Construction Kit implements an embedded profile.
- `CA_CLANG_TIDY_FLAGS`: This option specifies a semi-colon separated list of
  additional flags which are passed to `clang-tidy` when invoking `tidy`
  targets.
- `CA_HOST_ENABLE_BUILTIN_KERNEL`: This option enables builtin kernel support
  within the host target. By default, it is set to `OFF`. If enabled this will
  report that host supports builtin kernels and will also enable two test
  kernels that are used by UnitMux and UnitCL to verify functionality.
- `CA_HOST_ENABLE_FP64`: This option determines whether host is built with or
  without double support. By default, it is only enabled on non-Windows
  platforms.
- `CA_HOST_ENABLE_FP16`: This option determines whether host is built with or
  without half support. It is disabled by default since we can't detect if this
  feature is natively supported by hardware, which is a requirement.
- `CA_HOST_ENABLE_PAPI_COUNTERS`: This option enables performance counter
  support in host via the Mux `query_pool` API and the PAPI performance counter
  API. Requires the PAPI library and headers to be installed on the system.
  Currently this only works on Linux.
- `CA_HOST_CROSS_COMPILERS`: This option specifies a semi-colon separated list
  of compilers registered to enable offline or cross-compilation for non-native
  host CPU's, e.g. for Linux kernel cross-compile `arm`, `aarch64`, `x86`,
  `x86_64` may be specified, alternatively set to `all` to enable all backends
  which were built during the LLVM install.
- `CMAKE_SKIP_RPATH`: On Linux the oneAPI Construction Kit specifies a relative
 `RPATH` for all targets when they are installed using `CMAKE_INSTALL_RPATH`,
  this ensures that when the `install` target is invoked the user does not need
  to specify `LD_LIBRARY_PATH` to correctly execute a test binary in order to
  use the installed OpenCL or Vulkan library. Do disable this behaviour set
  `-DCMAKE_SKIP_RPATH=ON` when configuring CMake in build directory.
- `CA_HOST_TARGET_CPU`: This option is used by host to optimize for performance
  on a given CPU. If set to "native" host will optimize for the CPU being used
  to compile it. Otherwise a CPU name can be provided, for example "skylake",
  but be warned that this string will be passed directly to the llvm backend so
  make sure it's a valid CPU name. Information about your host CPU can be found
  by running `llc --version`, and a list of host CPUs supported by your
  installed version of LLVM can be found by running
  `llc --march=[your-arch] --mcpu=help`. Beware that if host is compiled with
  this option set, running it on a different CPU from the one specified (or the
  one compiled with if "native" was specified) isn't supported and bad things
  may happen. When the oneAPI Construction Kit is built in debug mode, the
  environment variable `CA_HOST_TARGET_CPU` is also respected, which can help
  track down codegen differences among different machine targets. The caveats
  above apply, and this may result in an illegal instruction crash if your CPU
  doesn't support the generated instructions.
- `CA_USE_SPLIT_DWARF`: When building with gcc, enable split dwarf debuginfo.
  This significantly reduces binary size (especially when static linkning) and
  speeds up the link step. Requires a non-ancient toolchain.
- `CA_CL_TEST_STATIC_LIB`: Forces all of our CL executable targets to link the
  static CL library rather than the normal dynamic one, to force testing with
  the static library.
- `CA_MUX_TARGETS_TO_ENABLE`: A `;` separated list of `mux` targets that should
  be enabled. By default this is set to the `host` target.
- `CA_EXTERNAL_MUX_TARGET_DIRS`: A `;` separated list of external `mux` targets that
 should be built. The base directory name must be that of the target.
- `CA_EXTERNAL_MUX_COMPILER_DIRS`: A `;` separated list of external
  `compiler` targets that should be built. The base directory name must be that of
   the target.

[asan]: https://clang.llvm.org/docs/AddressSanitizer.html
[tsan]: https://clang.llvm.org/docs/ThreadSanitizer.html
[ubsan]: https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
[libfuzzer]: https://llvm.org/docs/LibFuzzer.html

### CMake Build Targets

* `all`: The default target, it builds everything which is enabled by default.
* `install`: First builds the `all` target, then installs the files to the path
  specified by `CMAKE_INSTALL_PREFIX`.
* `clean`: Removes all build artifacts, useful if a file exists which is
  suspected of being invalid.

#### oneAPI Construction Kit CMake Build Targets

* `ComputeAorta`: Build the OpenCL and Vulkan libraries, if present, and all
  their test suites.
* `check`/`check-<target>`: Build and run all short running test suites, this
  selection of testing is used by continuous integration to verify a baseline of
  correctness, individual test suites can also be tested in isolation by
  specifying the target to test.
* `internal_builtins`: Builds the compiler builtins functions, this target can
  be used even if automatically building the builtins was disabled with
  `CA_EXTERNAL_BUILTINS`, although this target will fail in cross compile
  builds.
* `doc_html`: Generates HTML documentation for the oneAPI Construction Kit project,
  currently this is only supported for OpenCL. Due to our dependency on the
  `breathe` package which has known memory leaks, building this target can take
  an excessive amount of time or fail with a python `MemoryError`. A workaround
  for this issue is to temporarily delete the file `api-reference.md` to reduce
  demands on the build.
* `format`: When `clang-format` is found by CMake the `format` target is added,
  this target, when invoked, automatically formats all C/C++ source code which
  has been editing and not yet committed to the repository.
* `tidy`/`tidy-<target>`: When `clang-tidy` is found by CMake a number of
  additional targets are added which invoke `clang-tidy` to perform static
  analysis. The `tidy` target runs `clang-tidy` on all targets adding using the
  `add_ca_{library,exectuable}` commands which also add an individual
  `tidy-<target>` target per library or executable.

##### oneAPI Construction Kit OpenCL CMake Build Targets

* `CL`: Build only the OpenCL library, only available when OpenCL is enabled.
* `UnitCL`: Build the UnitCL test suite, as well as the OpenCL library.
* `OpenCLCTS`: Build the OpenCL library and all the CTS binaries.
* `check-cl`: Build and run various OpenCL test suites, primarily UnitCL and
  selected short running OpenCL CTS tests.

##### oneAPI Construction Kit Vulkan CMake Build Targets

* `VK`: Build only the Vulkan library, only available when Vulkan is enabled.
* `UnitVK`: Build the UnitVK test suite, as well as the Vulkan library.
* `VKICDManifest`: Generates the Vulkan ICD manifest, Linux only.
* `check-vk`: Build and run UnitVK and spirv-ll lit tests.

## Compiling

### Compiling LLVM

oneAPI Construction Kit requires an [LLVM](https://github.com/llvm/llvm-project)
install that includes the `clang` project. First clone the LLVM repository.

```sh
git clone https://github.com/llvm/llvm-project.git --branch $LLVMBranch
cd llvm-project
```

#### Compiling LLVM from Upstream on Linux

Configure the build directory with CMake, ensuring to enable the `clang`
project using `LLVM_ENABLE_PROJECTS`. Run this command from the root of the
repository.

```sh
cmake llvm -GNinja \
  -Bbuild-x86_64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=$PWD/build-x86_64/install \
  -DLLVM_ENABLE_PROJECTS=clang \
  -DLLVM_TARGETS_TO_BUILD='X86;ARM;AArch64'
```

Now the build directory is configured, build the `install` target.

```sh
ninja -C build-x86_64 install
```

#### Compiling LLVM from Upstream on Windows

Configure the build directory with CMake, ensuring to enable the `clang`
project using `LLVM_ENABLE_PROJECTS`. Run this command from the root of the
repository. The `LLVM_TEMPORARILY_ALLOW_OLD_TOOLCHAIN` variable is needed when
building LLVM version 8.0 or later on Visual Studio toolchains prior to MSVC
version 19.1.

```bat
cmake llvm -G"Visual Studio 15 2017 Win64" ^
  -Bbuild-x86_64 ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_INSTALL_PREFIX=%CD%\build-x86_64\install ^
  -DLLVM_TARGETS_TO_BUILD="X86;ARM;AArch64" ^
  -DLLVM_ENABLE_PROJECTS=clang ^
  -DLLVM_TEMPORARILY_ALLOW_OLD_TOOLCHAIN=ON
```

> Note that using the Ninja generator, `-GNinja`, on Windows may be preferable
> for improve compilation times.

Now the build directory is configured, build the `install` target. This can be
done by opening the `llvm.sln` solution in Visual Studio and building the
`install` target via the GUI for the Release configuration. Alternatively we
can build on the command line using CMake as shown below.

```bat
cmake --build %CD%\build-x86_64 --target install --config Release
```

### Compiling LLVM-SPIRV

`llvm-spirv` is used to translate bitcode binaries into SPIR-V binaries.

The `SPIRV-LLMV-Translator` repository should be cloned into its own directory
and **not** into LLVM's `projects` sub-directory. In principle, the translator
can be cloned into the LLVM source tree and built as yet another LLVM
sub-project. However, this currently causes a CMake conflict in the oneAPI
Construction Kit on the `llvm-spirv` executable.

```console
git clone -b llvm_release_80 \
  https://github.com/KhronosGroup/SPIRV-LLVM-Translator.git
cd SPIRV-LLVM-Translator
mkdir build
cd build
cmake .. -GNinja \
  -DLLVM_DIR=/path/to/llvm80/build/install/lib/cmake/llvm/
ninja llvm-spirv
```

The executable `llvm-spirv` will be generated in the `build/tools/llvm-spirv/`
directory.

> NOTE: The LLVM release version and the `llvm-spirv` branch version must
> match. The vast majority of spir-v binaries have been translated using
> `llvm-spirv` based on LLVM 8.0.

### Compiling oneAPI Construction Kit

Compiling the oneAPI Construction Kit requires an LLVM install to link against
and to use the tools as part of the build process when the runtime compiler is
enabled, follow the [LLVM guide](#compiling-llvm) to build a suitable install.
In the following examples the `$LLVMInstall` variable specifies the directory
of the LLVM install. Alternatively, to compile the oneAPI Construction Kit with
the runtime compiler disabled follow the [without LLVM guide](#compiling-the-oneapi-construction-kit-without-llvm).

The examples provided should be sufficient to get up and running, for more fine
grained control of how to compile the oneAPI Construction Kit consult the list of
[CMake options](#oneapi-construction-kit-cmake-options).

```{warning}
the oneAPI Construction Kit must use the same ``NDEBUG`` configuration as LLVM.
For the most part this should be automatically detected, but if you pass custom
`CXXFLAGS` to cmake, this cannot be properly detected and may cause ABI issues
and subsequent crashes that are difficult to debug
```

#### Compiling oneAPI Construction Kit on Linux

To configure the oneAPI Construction Kit build run the following command from
the root of the oneAPI Construction Kit repository.

```sh
cmake . -GNinja \
  -Bbuild-x86_64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=$PWD/build-x86_64/install \
  -DCA_LLVM_INSTALL_DIR=$LLVMInstall
```

Now the build directory is configured, build the `install` target.

```sh
ninja -C build-x86_64 install
```

The `check` target will run the oneAPI Construction Kit's test suites to
ensure the build is working as expected.

```sh
ninja -C build-x86_64 check
```

##### Compiling Debug oneAPI Construction Kit on Linux

Non-Release build times of the oneAPI Construction Kit can benefit from using a
Release install of LLVM, this is because tools such as `clang`, `llvm-dis`,
`FileCheck`, and others from a Non-Release LLVM install are used as part of the
build. To enable faster build times set the `CA_BUILTINS_TOOLS_DIR` variable to
point to a Release install of LLVM, here `$LLVMReleaseInstall` specifies the path
to the root of the install.

```sh
cmake . -GNinja \
  -Bbuild-x86_64-Debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX=$PWD/build-x86_64-Debug/install \
  -DCA_LLVM_INSTALL_DIR=$LLVMInstall \
  -DCA_BUILTINS_TOOLS_DIR=$LLVMReleaseInstall/bin
```

Now the build directory is configured, you can build the oneAPI Construction Kit
and run the dchecks as above.

#### Compiling oneAPI Construction Kit on Windows

To configure a oneAPI Construction Kit build run the following command from the
root of the oneAPI Construction Kit repository.

```bat
cmake . -G"Visual Studio 15 2017 Win64" ^
  -Bbuild-x86_64 ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_INSTALL_PREFIX=%CD%\build-x86_64\install ^
  -DCA_LLVM_INSTALL_DIR=%LLVMInstall%
```

> Note that using the Ninja generator, `-GNinja`, on Windows may be preferable
> for improve compilation times.

Now the build directory is configured, build the `install` target. This can be
done by opening the `ComputeAorta.sln` solution in Visual Studio and building
the `install` target via the GUI for a Release configuration. Alternatively we
can build on the command line using CMake as shown below.

```bat
cmake --build %CD%\build-x86_64 --target install --config Release
```

The `check` target will run the oneAPI Construction Kit's test suites to ensure
the build is working as expected.

```bat
cmake --build %CD%\build-x86_64 --target check --config Release
```

#### Compiling Debug oneAPI Construction Kit on Windows

Non-Release build times of the oneAPI Construction Kit can benefit from using a
Release install of LLVM, this is because tools such as `clang`, `llvm-dis`,
`FileCheck`, and others from a Non-Release LLVM install are used as part of the
build. To enable faster build times set the `CA_BUILTINS_TOOLS_DIR` variable to
point to a Release install of LLVM, here `%LLVMReleaseInstall%` specifies the
path to the root of the install.

```bat
cmake . -G"Visual Studio 15 2017 Win64" ^
  -Bbuild-x86_64-Debug ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_INSTALL_PREFIX=%CD%\build-x86_64-Debug\install ^
  -DCA_LLVM_INSTALL_DIR=%LLVMInstall% ^
  -DCA_BUILTINS_TOOLS_DIR=%LLVMReleaseInstall%\bin
```

> Note that using the Ninja generator, `-GNinja`, on Windows may be preferable
> for improve compilation times.

Now the build directory is configured, you can build the oneAPI Construction Kit
and run the checks, changing the `--config` parameter to Debug.

```bat
cmake --build %CD%\build-x86_64-Debug --target install --config Debug
```

```bat
cmake --build %CD%\build-x86_64-Debug --target check --config Debug
```

### Compiling the oneAPI Construction Kit without LLVM

Compiling the oneAPI Construction Kit without LLVM is also referred to as an
offline-only configuration or disabling the runtime compiler. In this mode the
oneAPI Construction Kit does not include a JIT compiler for OpenCL, offline-only
is currently not supported for Vulkan and is disabled in the configuration.

#### Compiling the oneAPI Construction Kit without LLVM on Linux

The `clc` offline compiler, which includes LLVM, must be provided. To build
`clc` follow the [guide](#compiling-oneapi-construction-kit-on-linux) above, the
`$ONEAPI_CON_KIT_INSTALL` variable specifies the path to the root of this install.

To configure the oneAPI Construction Kit build without LLVM run following command
from the root of the oneAPI Construction Kit repository.

```sh
cmake . -GNinja \
  -Bbuild-offline \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=$PWD/build-offline/install \
  -DCA_RUNTIME_COMPILER_ENABLED=OFF \
  -DCA_EXTERNAL_CLC=$ONEAPI_CON_KIT_INSTALL/bin/clc \
  -DCA_ENABLE_API=cl
```

Now the build directory is configured, build the `install` target.

```sh
ninja -C build-offline install
```

#### Compiling oneAPI Construction Kit without LLVM on Windows

The `clc` offline compiler, which includes LLVM, must be provided. To build
`clc` follow the [guide](#compiling-oneapi-construction-kit-on-windows) above,
the `%ONEAPI_CON_KIT_INSTALL%` variable specifies the path to the root of this
install.

The configure the oneAPI Construction Kit build without LLVM run the following
command from the root of the oneAPI Construction Kit repository.

```bat
cmake . -G"Visual Studio 15 2017 Win64" ^
  -Bbuild-offline ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_INSTALL_PREFIX=%CD%\build-offline\install ^
  -DCA_RUNTIME_COMPILER_ENABLED=OFF ^
  -DCA_EXTERNAL_CLC=%ONEAPI_CON_KIT_INSTALL%\bin\clc ^
  -DCA_ENABLE_API=cl
```

Now the build directory is configured, build the `install` target. This can be
done by opening the `ComputeAorta.sln` solution in Visual Studio and building
the `install` target via the GUI for a Release configuration. Alternatively we
can build on the command line using CMake as shown below.

```bat
cmake --build %CD%\build-offline --target install --config Release
```

#### Compiling oneAPI Construction Kit without LLVM on Windows using MinGW

MinGW can be used instead of Visual Studio to compile the oneAPI Construction
Kit. On Windows, only offline-only the oneAPI Construction Kit can be built
with MinGW, so the build is of limited practical value. The build is configured
as follows:

```bat
cmake . -GNinja ^
  -DCMAKE_C_COMPILER=gcc.exe ^
  -DCMAKE_CXX_COMPILER=g++.exe ^
  -Bbuild-offline ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_INSTALL_PREFIX=%CD%\build-offline\install ^
  -DCA_RUNTIME_COMPILER_ENABLED=OFF ^
  -DCA_CL_ENABLE_OFFLINE_KERNEL_TESTS=OFF ^
  -DCA_ENABLE_API=cl
```

Then, install with:

```bat
cmake --build %CD%\build-offline --target install --config Release
```

> Note: MinGW must be installed with structured exception handling (SEH) and
> POSIX threads. The `choco` command is
> `choco install mingw -y -params "/exception:seh /threads:posix"`.

## Cross-compiling

> Note: Cross-compilation is only supported on Linux.

## Cross-platform building LLVM and oneAPI Construction Kit for Linux

All CMake cross-compilation configurations set `CMAKE_TOOLCHAIN_FILE` to inform
CMake how to compile for the target architecture, this sets up various CMake
variables which specify the locations of executables such as the C and C++
compilers, assembler, linker, target file system root, etc.

The examples provided should be sufficient to get up and running, for more fine
grained control of how to compile the oneAPI Construction Kit consult the list of
[CMake options](#oneapi-construction-kit-cmake-options).

Cross-compilation requires -- in most cases -- both a native build of LLVM and
a specialised build of of LLVM for the cross-compilation target. In this guide,
the following environment variables are assumed.

```sh
# path to upstream LLVM build
LLVMNativeBuild=/absolute/path/to/native_llvm/build

# usually equivalent to ${LLVMNativeBuild}/install
LLVMNativeInstall=${CMAKE_INSTALL_PREFIX}
```

### Cross-compiling LLVM

#### Cross-compiling LLVM from Upstream

To cross-compile LLVM the appropriate CMake toolchain file from the oneAPI
Construction Kit repository is required, the path to this will be specified
by the `$ONEAPI_CON_KIT` variable in the following examples.

##### Cross-compiling LLVM for Arm from Upstream

For cross-compilation targeting Arm only the `ARM` target back end is enabled.
Run the following command to configure an LLVM build targeting Arm from the root
of the repository.

```sh
cmake . -GNinja \
  -Bbuild-arm \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$ONEAPI_CON_KIT/scripts/toolchains/arm-toolchain.cmake \
  -DCMAKE_INSTALL_PREFIX=$PWD/build-arm/install \
  -DLLVM_TARGET_ARCH=ARM \
  -DLLVM_TARGETS_TO_BUILD=ARM \
  -DLLVM_HOST_TRIPLE=arm-unknown-linux-gnu \
  -DLLVM_DEFAULT_TARGET_TRIPLE=arm-unknown-linux-gnu \
  -DLLVM_ENABLE_ZLIB=OFF \
  -DLLVM_TABLEGEN=$LLVMNativeInstall/bin/llvm-tblgen \
  -DCLANG_TABLEGEN=$LLVMNativeBuild/bin/clang-tblgen
```

Now the build directory is configured, build the `install` target.

```sh
ninja -C build-arm install
```

##### Cross-compiling LLVM for AArch64 from Upstream

For cross-compilation targeting AArch64 only the `AArch64` target back end is
enabled. Run the following command to configure an LLVM build targeting Arm from
the root of the repository.

```sh
cmake . -GNinja \
  -Bbuild-aarch64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$ONEAPI_CON_KIT/scripts/toolchains/aarch64-toolchain.cmake \
  -DCMAKE_INSTALL_PREFIX=$PWD/build-aarch64/install \
  -DLLVM_TARGET_ARCH=AArch64 \
  -DLLVM_TARGETS_TO_BUILD=AArch64 \
  -DLLVM_HOST_TRIPLE=aarch64-unknown-linux-gnu \
  -DLLVM_DEFAULT_TARGET_TRIPLE=aarch64-unknown-linux-gnu \
  -DLLVM_ENABLE_ZLIB=OFF \
  -DLLVM_TABLEGEN=$LLVMNativeInstall/bin/llvm-tblgen \
  -DCLANG_TABLEGEN=$LLVMNativeBuild/bin/clang-tblgen
```

Now the build directory is configured, build the `install` target.

```sh
ninja -C build-aarch64 install
```

### Cross-compiling the oneAPI Construction Kit

Cross-compiling the oneAPI Construction Kit requires a LLVM install to link
against, follow the [LLVM guide](#cross-compiling-llvm) to build a suitable
install, the `$LLVMInstall` variable specifies the path to this install.

oneAPI Construction Kit uses tools from an LLVM install during the build
process. For a native x86_64 build no additional install is required, although
build times can be [improved](#compiling-debug-oneapi-construction-kit-on-linux)
by doing so. For cross-compilation a native LLVM install is also required, the
`$LLVMNativeInstall` variable specifies the path to this install.

#### Cross-compiling the oneAPI Construction Kit for Arm

Configure an Arm cross-compile build using the following command.

```sh
cmake . -GNinja \
  -Bbuild-arm \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/platform/arm-linux/arm-toolchain.cmake \
  -DCMAKE_INSTALL_PREFIX=$PWD/build-arm/install \
  -DCA_LLVM_INSTALL_DIR=$LLVMInstall \
  -DCA_BUILTINS_TOOLS_DIR=$LLVMNativeInstall/bin
```

Now the build directory is configured, build the `install` target.

```sh
ninja -C build-arm install
```

If `qemu-arm` is installed on the system `arm-toolchain.cmake` will
automatically detect it and set the `CMAKE_CROSSCOMPILING_EMULATOR` variable,
the oneAPI Construction Kit uses this to enable emulated testing using the
`check` target.

```sh
ninja -C build-arm check
```

#### Cross-compiling the oneAPI Construction Kit for AArch64

Configure an AArch64 cross-compile build using the following command.

```sh
cmake . -GNinja \
  -Bbuild-aarch64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/platform/arm-linux/aarch64-toolchain.cmake \
  -DCMAKE_INSTALL_PREFIX=$PWD/build-aarch64/install \
  -DCA_LLVM_INSTALL_DIR=$LLVMInstall \
  -DCA_BUILTINS_TOOLS_DIR=$LLVMNativeInstall/bin
```

Now the build directory is configured, build the `install` target.

```sh
ninja -C build-aarch64 install
```

If `qemu-aarch64` is installed on the system `aarch64-toolchain.cmake` will
automatically detect it and set the `CMAKE_CROSSCOMPILING_EMULATOR` variable,
the oneAPI Construction Kit uses this to enable emulated testing using the
`check` target.

```sh
ninja -C build-aarch64 check
```

#### Cross-compiling the oneAPI Construction Kit for Windows with the MinGW toolchain

oneAPI Construction Kit for Windows can be built on Linux using MinGW. This
requires LLVM that has been built with MinGW. Note that offline-compiled
kernels cannot be built, because there is no way to run the `clc` that is built.

```console
cmake -GNinja \
  -Bbuild-mingw \
  -DCMAKE_INSTALL_PREFIX=$PWD/build-mingw/install \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/platform/mingw-w64/mingw-w64-toolchain.cmake \
  -DCA_LLVM_INSTALL_DIR=/path/to/mingw/llvm/build \
  -DCMAKE_BUILD_TYPE=ReleaseAssert \
  -DCA_BUILTINS_TOOLS_DIR=/path/to/llvm_tools \
  -DCA_CL_ENABLE_OFFLINE_KERNEL_TESTS=OFF \
  .

cmake --build build-mingw --target ComputeAorta
```

## Testing

<!-- TODO: Document gtest suites -->
<!-- TODO: Document lit test suites -->
<!-- TODO: Document check targets -->

### Testing OpenCL

* UnitCL: Is our primary OpenCL test suite, it extensively tests the API part of
  OpenCL in various situations. It is composed of one self-contained binary.
  UnitCL also contains a series of kernel execution tests built to test the
  vectorizer as well as compilation stages. We add regression tests to UnitCL
  when we fix previously uncaught bugs and when we add new functionality to
  the oneAPI Construction Kit. See the [UnitCL documentation](source/cl/test/unitcl.md)
  for more details on UnitCL.
* OpenCL CTS: We have our own runner for the OpenCL CTS, you can refer to its
  `scripts/testing/README.md` for how to use it.
* CLSmith and C-Reduce: [CLSmith](https://github.com/ChrisLidbury/CLSmith) is a
  tool to generate random OpenCL kernels, accompanied by a helper application to
  run them. If a bug is found in a kernel,
  [C-Reduce](https://embed.cs.utah.edu/creduce/) can help reduce the code size
  of the test down to a manageable size.

#### Testing OpenCL with CLSmith

[CLSmith](https://github.com/ChrisLidbury/CLSmith) is a tool (based on
[CSmith](https://embed.cs.utah.edu/csmith/)) used to generate random OpenCL
kernels. It also provides a launcher application (`cl_launcher`) to run the
generated kernels and produce an output. Specifically, the output is generated
by hashing together all the variables in the kernel.

Build instructions for CLSmith are available on the Github repository. After
building it, the easiest way to start using it is with the `cl_setup_test.py`
script found in the `scripts` directory. The script accepts one argument, a
directory name, and it will create that directory in `$HOME`. In there it will
place all the files necessary for creating a running a kernel, such as the
`CLSmith` and `cl_launcher` executables, as well as some headers required by the
generated kernels. So, for example, in order to generate and run a random
kernel, one would do the following:

```sh
cd CLSmith/scripts
./cl_setup_test.py cltest
cd ~/cltest
./CLSmith
./cl_launcher -f CLProg.c -p 0 -d 0
```

This will run the random kernel on the first device of the first OpenCL platform
(`-p 0 -d 0`). It is also possible to specify local and global workgroup sizes
with the `-l` and `-g` arguments, such as

```sh
./cl_launcher -f CLProg.c -p 0 -d 0 -l 1,1,1 -g 1,1,1
```

This will run only one work item, which can help to make the testing faster.

`cl_launcher` itself is not a testing application, which means that the user can
write their own tests around it. For example, a simple test would be to detect
if the compilation failed or not. Another test can be constructed by running the
kernel on multiple OpenCL implementations and comparing the results. Finally,
it is possible to run with and without optimization and compare the results.

#### Testing OpenCL with C-Reduce

The kernels generated from CLSmith can be hundreds or thousands of lines long,
which can make narrowing down the issue difficult.
[C-Reduce](https://embed.cs.utah.edu/creduce/) is a tool that can take a source
file and a test script and reduce the size of the source file while making sure
that the test still fails.

```sh
creduce test.sh CLProg.c
```

C-Reduce is C-aware, using clang to perform C specific changes to the source
code. Since clang also supports OpenCL, it is possible to use C-Reduce with
OpenCL kernels, although it will not perform OpenCL specific transformation,
such as propagation of constant vectors etc.

The first argument for C-Reduce is called the "interestingness" test. This is
the test that determines if the transformed source code is still interesting,
i.e. if it still exhibits the issue that we are interested in debugging. A
simple interestingness test could be the following:

```sh
./cl_launcher -f CLProg.c -p 0 -d 0 -l 1,1,1 -g 1,1,1 || return 0
```

The interestingness test should return 0 if the test is interesting. The test
above runs the kernel and if `cl_launcher` fails for some reason, then it
returns 0 (`||` is a short-circuited boolean OR in bash). While bash scripts are
common, the test can be anything that can be executed, such as python scripts or
executables.

C-Reduce is a fairly simple application to use ([online
documentation](https://embed.cs.utah.edu/creduce/using/)), but there are some
gotchas:

1. The input file needs to be given as a relative path, while everything else
   needs to be in absolute paths. C-Reduce will copy the input file in a
   temporary directory and work on it from there, so all the paths need to be
   adjusted accordingly.
2. Multiple interestingness tests will be run in parallel, so special care needs
   to be taken when outputting into files etc.
3. C-Reduce will not do anything other than run the interestingness test to
   determine whether to keep a transformation or not. This can lead to
   transformations being applied that produce code that compiles but exhibits
   undefined behaviour. For this reason, it is also important to incorporate
   some sort of correctness test in the interestingness test. For example, if
   testing for runtime failures in OpenCL kernels, it is possible to use tools
   like Oclgrind to check the kernel for undefined behaviour.
4. The interestingness test has to be specific, because otherwise it is possible
   to switch to a different bug during the reduction, or produce code that is
   completely invalid. For example, it is better to check for a specific output
   or error message than just a compilation failure in general.
5. The test should be optimized for performance. During the reduction process,
   it might be run thousands of times, so if the test is slow the reduction
   process will also be extremely slow.
6. When it comes to files generated from CLSmith, it is easier to test them if
   they are passed though the C preprocessor before testing them, since this
   will eliminate all the include directives.

### Testing oneapi-construction-kit application examples using official Intel oneAPI Base Toolkit

Download the official Intel OneAPI Base Toolkit following the instructions mentioned [here](../README.md#compiling-oneapi-samples-vector-add-using-official-intel-oneapi-base-toolkit).

To compile the tests follow the steps below:

```sh
mkdir build_tests
cmake -GNinja -Bbuild_tests -DCMAKE_CXX_COMPILER=/path/to/intel_oneapi/bin/clang++ /path/to/oneapi-construction-kit/examples/applications -DOpenCL_LIBRARY=/path/to/build/lib/libCL.so -DOpenCL_INCLUDE_DIR=/path/to/build-riscv/include
ninja -C build_tests
```

To test the binaries compiled above, set the environment variables as follows:

```sh
export LD_LIBRARY_PATH=/path/to/build/lib:/path/to/intel_oneapi/lib/libsycl.so:/path/to/intel_oneapi/lib:$LD_LIBRARY_PATH
export CMAKE_CXX_COMPILER=/path/to/intel_oneapi/bin/clang++
export CMAKE_C_COMPILER=/path/to/intel_oneapi/bin/clang
export CA_HAL_DEBUG=1
export CA_PROFILE_LEVEL=3
export ONEAPI_DEVICE_SELECTOR=opencl:acc
export OCL_ICD_FILENAMES=/path/to/build/lib/libCL.so
# As the oneAPI basetoolkit release has a whitelist of devices, it filters out RefSi.
# To override it, as a temporary solution we can point SYCL_CONFIG_FILE_NAME to ``.
# This way it doesn't set the default sycl.conf.
export SYCL_CONFIG_FILE_NAME=""
```

The tests can be run using `ctest` command.
```sh
cd build_tests
ctest
```

The generated output should be as follows:
```sh
Test project /path/to/build_tests
    Start 1: simple_vector_add
1/7 Test #1: simple_vector_add ..................   Passed    0.06 sec
    Start 2: vector_addition-load-store
2/7 Test #2: vector_addition-load-store .........   Passed    0.04 sec
    Start 3: vector_addition-predicated
3/7 Test #3: vector_addition-predicated .........   Passed    0.03 sec
    Start 4: vector_addition-masked-store
4/7 Test #4: vector_addition-masked-store .......   Passed    0.04 sec
    Start 5: vector_addition-tiled-load-store
5/7 Test #5: vector_addition-tiled-load-store ...   Passed    0.03 sec
    Start 6: syclAvgPooling
6/7 Test #6: syclAvgPooling .....................   Passed    0.03 sec
    Start 7: clVectorAddition
7/7 Test #7: clVectorAddition ...................   Passed    0.04 sec

100% tests passed, 0 tests failed out of 7
```

More information can be gathered by passing `--verbose` to the `ctest` command.

## Providing extra options

The following environment variables are mostly used for testing and trying out
options without having to modify the source.

* `CA_EXTRA_COMPILE_OPTS`: This option is used to specify additional
  compile options when building a kernel.
* `CA_EXTRA_LINK_OPTS`: This option is used to specify additional link
  options in the same manner as `CA_EXTRA_COMPILE_OPTS`.
* `CA_LLVM_OPTIONS`: This environment variable allows the injection of LLVM
  flags **only** when either `NDEBUG` is not defined (i.e. `Debug` and
  `ReleaseAssert` build configurations) or when the
  `CA_ENABLE_LLVM_OPTIONS_IN_RELEASE` option is set in CMake. See
  [below](#debugging-the-llvm-compiler) for example of how this can be used.
* `CA_HOST_NUM_THREADS`: Sets the maximum number of threads the `host` device
  will create. `host` may create fewer threads than this value.

## Debugging the LLVM compiler

Developers can use a variety of methods to debug the running of LLVM compiler
pipelines _without having to recompile the oneAPI Construction Kit_. The
following suggestions involve passing additional options via `CA_LLVM_OPTIONS`
(detailed [above](#providing-extra-options)).

### -print-after-all (and -print-before-all)

This option prints the state of the compiler IR after (/before) every compiler
pass. This can be useful for:

* understanding compiler flow
* identifying the source of an unknown bug
* tracing a known bug through the pipeline

> **Note:** Passes print the _unit of IR_ on which they work: module passes
> print the whole module; function passes print just the function; loops print
> only the loop. This can occasionally interfere with bug discovery.

### -print-after=X (and -print-before=X)

This option prints the state of the compiler IR after (/before) a specific
compiler pass. When given a [pass name](#pass-names) or comma-separated list of
pass names, it prints the IR before or after every instance of those passes, on
every unit of IR:

```
> CA_LLVM_OPTIONS=-print-after=early-cse,mem2reg ...

*** IR Dump After EarlyCSEPass on foo ***
; Function Attrs: norecurse nounwind
define dso_local spir_kernel void @foo(ptr addrspace(1) align 4 %a, ptr addrspace(1) align 4 %b) #0 {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0) #3
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %a, i64 %call
  %0 = load i32, ptr addrspace(1) %arrayidx, align 4, !tbaa !10
  %mul = mul nsw i32 %0, 5
  %arrayidx1 = getelementptr inbounds i32, ptr addrspace(1) %b, i64 %call
  store i32 %mul, ptr addrspace(1) %arrayidx1, align 4, !tbaa !10
  ret void
}
*** IR Dump After EarlyCSEPass on bar ***
; Function Attrs: norecurse nounwind
define dso_local spir_kernel void @bar(ptr addrspace(1) align 4 %a, ptr addrspace(1) align 4 %b) #0 {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0) #3
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %a, i64 %call
  %0 = load i32, ptr addrspace(1) %arrayidx, align 4, !tbaa !10
  %mul = mul nsw i32 %0, 5
  %arrayidx2 = getelementptr inbounds i32, ptr addrspace(1) %b, i64 %call
  store i32 %mul, ptr addrspace(1) %arrayidx2, align 4, !tbaa !10
  ret void
}
*** IR Dump After PromotePass on foo ***
; Function Attrs: norecurse nounwind
define dso_local spir_kernel void @foo(ptr addrspace(1) align 4 %a, ptr addrspace(1) align 4 %b) local_unnamed_addr #0 {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0) #2
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %a, i64 %call
  %0 = load i32, ptr addrspace(1) %arrayidx, align 4, !tbaa !10
  %mul = mul nsw i32 %0, 5
  %arrayidx1 = getelementptr inbounds i32, ptr addrspace(1) %b, i64 %call
  store i32 %mul, ptr addrspace(1) %arrayidx1, align 4, !tbaa !10
  ret void
}
; <... and on>
```

The two options can be combined, e.g., to better inspect the result of a
specific pass:

```
> CA_LLVM_OPTIONS="-print-before=early-cse -print-after=early-cse" ...

*** IR Dump Before EarlyCSEPass on bar ***
; Function Attrs: norecurse nounwind
define dso_local spir_kernel void @bar(ptr addrspace(1) align 4 %a, ptr addrspace(1) align 4 %b) #0 {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0) #3
  %call1 = call spir_func i64 @_Z13get_global_idj(i32 0) #3
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %a, i64 %call1
  %0 = load i32, ptr addrspace(1) %arrayidx, align 4, !tbaa !10
  %mul = mul nsw i32 %0, 5
  %arrayidx2 = getelementptr inbounds i32, ptr addrspace(1) %b, i64 %call
  store i32 %mul, ptr addrspace(1) %arrayidx2, align 4, !tbaa !10
  ret void
}
*** IR Dump After EarlyCSEPass on bar ***
; Function Attrs: norecurse nounwind
define dso_local spir_kernel void @bar(ptr addrspace(1) align 4 %a, ptr addrspace(1) align 4 %b) #0 {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0) #3
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %a, i64 %call
  %0 = load i32, ptr addrspace(1) %arrayidx, align 4, !tbaa !10
  %mul = mul nsw i32 %0, 5
  %arrayidx2 = getelementptr inbounds i32, ptr addrspace(1) %b, i64 %call
  store i32 %mul, ptr addrspace(1) %arrayidx2, align 4, !tbaa !10
  ret void
}
```

This option is useful for visually inspecting the result of a pass, whether for
comprehension or to inspect a compiler bug narrowed down to a specific pass.

> **Note:** This uses the same output format as with
> `-print-after-all`/`-print-before-all` as noted above.

#### Pass names

The name of the pass can typically be found in any of the _pass registry
files_. The main ComputeMux pass registry is found at
`modules/compiler/source/base/source/base_module_pass_registry.def`, but
targets may define their own. LLVM also defines its own, found (through access
to the LLVM source code) at `llvm/lib/Passes/PassRegistry.def`.

Since LLVM ComputeMux use the same style of pass registration, both contain
lines such as:

```
MODULE_PASS("always-inline", AlwaysInlinerPass())

MODULE_PASS("add-sched-params", utils::AddSchedulingParametersPass())

FUNCTION_PASS_WITH_PARAMS("early-cse",
                          "EarlyCSEPass",
                           [](bool UseMemorySSA) {
                             return EarlyCSEPass(UseMemorySSA);
                           },
                          parseEarlyCSEPassOptions,
                          "memssa")
```

The pass name in each is the first token. For example, `always-inline`.

##### Pass name conventions

When adding a new pass, you should ensure its name is properly registered in
order that it is testable independently with tools such as `muxc`.

Use meaningful names for the string literal pass name following the conventions
already present in the global registry: terse pass names separated that
describe actions are most understandable; dashes separate words; lowercase
words separated by dashes where appropriate. This makes them easy to read,
understand, type, and require no shell quoting, e.g., `remove-fences`.

### -print-changed

This option works like
[-print-after-all](#-print-after-all-and--print-before-all) but only when the
pass makes a change to the IR.

```
*** IR Dump After CoroEarlyPass on [module] omitted because no change ***
*** IR Dump After LowerExpectIntrinsicPass on bar omitted because no change ***
*** IR Dump After SimplifyCFGPass on bar omitted because no change ***
*** IR Dump After EarlyCSEPass on bar ***
; Function Attrs: norecurse nounwind
define dso_local spir_kernel void @bar(ptr addrspace(1) align 4 %a, ptr addrspace(1) align 4 %b) #0 {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0) #3
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %a, i64 %call
  %0 = load i32, ptr addrspace(1) %arrayidx, align 4, !tbaa !9
  %mul = mul nsw i32 %0, 5
  %arrayidx2 = getelementptr inbounds i32, ptr addrspace(1) %b, i64 %call
  store i32 %mul, ptr addrspace(1) %arrayidx2, align 4, !tbaa !9
  ret void
}
```

Values can be passed to this option to control its behaviour:

* `-print-changed=quiet` - Only prints changed IR, suppressing all other
  messages including the starting IR

> **Note:** There are other values to this option that LLVM supports, like
> `diff` and `cdiff`, which print changes in a useful `diff`-compatible form.
> These options currently trigger crashes after certain passes used by the
> ComputeMux compiler so are not recommended for use.

### -verify-each

This option invokes LLVM's IR verifier before and after every pass. The default
behaviour is to verify the IR at the beginning and end of every compiler
pipeline, so may miss temporal verification errors that are masked by later
changes.

This option is useful for identifying whether a compiler pass is generating
invalid IR.

### -debug-pass-manager

This option gives an overview of each compiler pipeline, listing the passes and
analyses running at each point.

```
> CA_LLVM_OPTIONS=-debug-pass-manager

Running pass: ForceFunctionAttrsPass on [module]
Running analysis: InnerAnalysisManagerProxy<llvm::FunctionAnalysisManager, llvm::Module> on [module]
Running pass: compiler::SoftwareDivisionPass on bar (24 instructions)
  Running analysis: PreservedCFGCheckerAnalysis on bar
Running pass: compiler::StripFastMathAttrs on bar (24 instructions)
; <and on>
```

Values can be passed to this option to control its behaviour:

* `-debug-pass-manager=quiet` - Skips printing of analyses
  ```
  Running pass: ForceFunctionAttrsPass on [module]
  Running pass: compiler::SoftwareDivisionPass on bar (24 instructions)
  Running pass: compiler::StripFastMathAttrs on bar (24 instructions)
  ; <and on>
  ```
* `-debug-pass-manager=verbose` - Prints additional information about pass
  managers and adaptors.
  ```
  Running pass: ForceFunctionAttrsPass on [module]
  Running pass: ModuleToFunctionPassAdaptor on [module]
    Running analysis: InnerAnalysisManagerProxy<llvm::FunctionAnalysisManager, llvm::Module> on [module]
    Running pass: compiler::SoftwareDivisionPass on bar (24 instructions)
      Running analysis: PreservedCFGCheckerAnalysis on bar
  Running pass: ModuleToFunctionPassAdaptor on [module]
    Running pass: compiler::StripFastMathAttrs on bar (24 instructions)
  ; <and on>
  ```

### -time-passes

Prints timing information summaries at the end of each compiler pipeline, with
a breakdown of how long each individual pass took. This is useful for
understanding compile-time performance issues.

```
> CA_LLVM_OPTIONS=-time-passes

===-------------------------------------------------------------------------===
                      ... Pass execution timing report ...
===-------------------------------------------------------------------------===
  Total Execution Time: 0.0044 seconds (0.0044 wall clock)

   ---User Time---   --System Time--   --User+System--   ---Wall Time---  --- Name ---
   0.0013 ( 28.9%)   0.0000 (  0.0%)   0.0013 ( 28.9%)   0.0013 ( 28.8%)  ModuleInlinerWrapperPass
   0.0011 ( 24.6%)   0.0000 (  0.0%)   0.0011 ( 24.5%)   0.0011 ( 24.5%)  DevirtSCCRepeatedPass
   0.0005 ( 10.5%)   0.0000 (  0.0%)   0.0005 ( 10.5%)   0.0005 ( 10.5%)  SimplifyCFGPass
   0.0004 (  9.1%)   0.0000 (  0.0%)   0.0004 (  9.0%)   0.0004 (  9.0%)  TargetIRAnalysis
   ; <and on>
```

### Debugging passes with the `muxc` tool

``muxc`` is a tool, similar to ``opt``, which can be used to run compiler
pipelines made of the oneAPI Construction Kit utility passes or provided by
the target. This is detailed [here](modules/compiler/tools/muxc.rst).

## Running with extra debug support

In non-release mode environment variables can be used for debugging.
This can also be supported in release mode if the CMake option
`CA_ENABLE_DEBUG_SUPPORT` is set to `ON`.

On Bash or similar shells environment variables can be set as follows:

```
export CA_OCL_DEBUG_PRINT_KERNELS=1
```

On Windows console:

```
SET CA_OCL_DEBUG_PRINT_KERNELS=1
```

### Extracting Kernels From Tests

It is possible to extract the source of a kernel being compiled into a file
while running the the oneAPI Construction Kit compiler. This is, for example,
helpful for extracting kernels from failing test cases in a test suite. Setting
the `CA_OCL_DEBUG_PRINT_KERNELS` environment variable to `1` will enable the
feature.

The kernels will be printed in unique files named `cl_program_ID.cl` the next
time the compiler is run, where ID is an incremental numerical value padded with
zeroes. Note that the filenames are chosen simply to be unique without any
consideration of the contents of the file, so it is possible to get the same
kernel in different files, if the kernel is compiled multiple times.

## Perf Support for Linux CPU kernels

For Linux hosts which support perf hardware events, we can get various metrics
by setting the environment variable `CA_ENABLE_PERF_INTERFACE=1` and then
running the executable with perf. Since the kernel is being *JIT'ed*, on Linux
hosts.

1. The compiled kernel object will be placed in `/tmp/perf-$\{pid\}.o`
2. A map file with details about the kernel entry function required by perf,
   are placed in `/tmp/perf-${pid}.map`

It is possible to disassemble the object code in /tmp/perf-$\{pid\}.o using a
disassembler. For a list of hardware events that are supported by perf :

```sh
perf list
```

Some useful events that help with performance analysis are :

1. branch-instructions
2. branch-misses
3. cache-misses
4. cache-references
5. cpu-cycles
6. instructions
7. mem-loads
8. mem-stores

To record hardware events for perf, for all the above events,

```sh
CA_ENABLE_PERF_INTERFACE=1 perf record \
-e branch-instructions:u,branch-misses:u,cache-misses:u,cache-references:u\
,cpu-cycles:u,instructions:u,mem-loads:u,mem-stores:u <executable> <options>
```

After recording the profile, you can view the statistics using perf report

> **Note :** Be aware that if you run  *perf report* with the *-a* option to
> enable profiling on all the CPUs,  all processes running on the OS will be
> profiled and percentage calculations will take all of them into account. This
> is most likely **Not** what you want.

## Reducing Rebuild Times With `ccache`

On supported platforms, such a Linux distribution, it is possible to use
[`ccache`](https://ccache.samba.org/) to reduce rebuilds times.

> `ccache` is a compiler cache. It speeds up recompilation by caching previous
> compilations and detecting when the same compilation is being done again.

It can be installed on Ubuntu as follows:

```sh
sudo apt install ccache
```

Once installed `ccache` is not enabled by default, to use it the `PATH`
environment variable must be updated.

```sh
export PATH=/usr/lib/ccache:$PATH
```

> Note that to make this change permanent add the command above to your shell's
> configuration file, for example `~/.bashrc`.

The `/usr/lib/ccache` directory contains a number of symbolic links which alias
common names for compilers installed on the system including; `cc`, `c++`,
`gcc`, `g++`, `clang` and `clang++`. These all point to the `/usr/bin/ccache`
executable which handles the caching, when compilation is required `ccache`
dispatches to the actual compiler executable.

If you have installed `clang` from LLVM's [apt
repositories](https://apt.llvm.org/) `ccache` will not cache compilation because
the executables have a version suffix such as `clang-9`. There are two choices
to enable caching:

Using `update-alternatives` to manage which `clang` executable `/usr/bin/clang`
targets, you can do this using:

```sh
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-9 600
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-9 600
```

Adding symbolic link targeting `/usr/bin/ccache` called `clang-9`, in this
example lets assume `~/.local/bin` is at the beginning of `PATH` and will be
found before `/usr/bin/clang`.

```sh
ln -s /usr/bin/ccache $HOME/.local/bin/clang-9
ln -s /usr/bin/ccache $HOME/.local/bin/clang++-9
```

By default `ccache` has a cache size of 5 Gigabytes which can fill up quickly
when working with multiple debug build directories, to increase the cache size
to 20 Gigabytes:

```sh
ccache -M 20G
```

With `ccache` installed any existing build directories will need to be deleted
because the originals will not be using the `ccache` symbolic links. New build
directories can be configured as normal however please verify that
`/usr/lib/ccache` is found in the `PATH` before doing so:

```sh
echo $PATH
```

Now everything is set up we can verify `ccache` is working as expected using the
`watch` command in combination with `ccache -s`, which provides cache
statistics, in another terminal whilst a build is in progress:

```sh
watch ccache -s
```

If in the rare event `ccache` may result in bad builds, such as in the event of
a hash collision in the cache, the cache can be cleared:

```sh
ccache -C
```

> Unfortunately it is not possible to cache all the oneAPI Construction Kit build
> steps such as building bitcode for the [builtins](modules/builtins.md) module,
> this is due to `ccache` not being aware of the compiler flags passed to `clang` to
> generate these outputs.

## Enhanced GDB Debugging

### Pretty Printers

oneAPI Construction Kit makes use of many non-standard C++ types, such as those in
[cargo][cargo], which do not produce helpful output when used with the GDB
[print][gdb-print] command in a debugging session. To aid in such situations
[GDB][gdb] is extensible using the [Python API][gdb-python], this can be used to
register custom pretty printers for C++ types. [cargo][cargo] provides a set of
pretty printers for the types it defines, these can be found in
`modules/cargo/scripts/gdb/prettyprinters.py`. To enable them, issue the
following command in a [GDB][gdb] session:

```
(gdb) source modules/cargo/scripts/gdb/prettyprinters.py
```

[cargo]: api-reference.md#cargo-module
[gdb]: https://sourceware.org/gdb/onlinedocs/gdb
[gdb-print]: https://sourceware.org/gdb/onlinedocs/gdb/Data.html#index-print
[gdb-python]: https://sourceware.org/gdb/onlinedocs/gdb/Python-API.html#Python-API

## Tracer Guards

Limited internal profiling can be achived with tracer guards. If enabled a
.trace file is produced which can be viewed inside chrome, by typing
`chrome://tracing` in the address bar space, clicking `Load` and selecting your
trace file.

You must do two things to get a trace. First build with the any of the following
flags (or all of them) `-DCA_TRACE_CL=1`, `-DCA_TRACE_CORE=1`, and
`-DCA_TRACE_IMPLEMENTATION=1`. The tracer flags can be set manually in
`tracer.h` or built as part of CMake. Each flag will enable a trace for that
layer of the oneAPI Construction Kit.

And second set the environment variable `CA_TRACE_FILE=/path/to/save/your.trace`
E.g: `export CA_TRACE_FILE=/tmp/ca.trace`. Now when your run your application
a .trace file should be written to the location specified.

By default on Linux tracer will stream to a temporary fixed size file using an
atomic counter, this removes the need for a mutex on the file. Setting the
environment variable `CA_TRACE_FILE_BUFFER_MB` you can override the default
buffer size (1GB). It also has a max size of 75GB which represents the largest
tested value.

## Benchmarking driver performance with Flamegraphs

1) Ensure that symbol information is retained when building the oneAPI
   Construction Kit. This can be done by passing the
   `-DCA_ENABLE_DEBUG_BACKTRACE=ON` CMake option. See
   `source/cl/CMakeLists.txt:180` for more information. Without this step, the
   `perf` report will not contain anything useful.

2) Build the oneAPI Construction Kit and the benchmark you'll be using to
   benchmark the driver (for example, a benchmark from PerfCL).

3) Clone the Flamegraphs repository: https://github.com/brendangregg/FlameGraph

4) cd to your benchmark, then use the `perf` tool to execute your benchmark and
   record some stack samples:

  `perf record -g --call-graph dwarf ./jacobi1D` (if running the `jacobi1D`
    benchmark from PerfCL)

  Don't forget to ensure the the oneAPI Construction Kit CL driver is being
  loaded correctly by the ICD, or you use `OCL_ICD_FILENAMES` to override the
  CL driver:

  `export OCL_ICD_FILENAMES=<path-to-install>/lib/libCL.so`

  * TIP: `perf script` is extremely slow on Debian and Ubuntu because it relies
    on forking to `addr2line` for each stack captured (hundreds of MB's of data).
    If you build `perf` from source with `libbfd` installed, it will be up to
    60x faster. See https://eighty-twenty.org/2021/09/09/perf-addr2line-speed-improvement
    for more information.

5) Follow the instructions in the FlameGraph repository to generate a nice SVG:

   * `perf script > out.perf`
   * `$HOME/Work/FlameGraph/stackcollapse-perf.pl out.perf > out.folded`
   * `$HOME/Work/FlameGraph/flamegraph.pl out.folded > framegraph.svg`
