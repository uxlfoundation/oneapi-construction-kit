# spirv-ll lit tests

To invoke this test suite use the `check-ock-spirv-ll-lit` CMake target to run
[lit][lit] on the configured test inputs residing in the build directory.

## Modes

You can build this suite in one of two modes: online and offline (the default)

### Offline Mode

In offline mode - the default, or when `CA_ASSEMBLE_SPIRV_LL_LIT_TESTS_OFFLINE`
is enabled in cmake - the test suite will build its SPIR-V binaries ahead of
time.

This mode requires `spirv-as` to build the `spvasm` test binaries, and
`glslangValidator` to build the `glsl` test binaries. Both are optional: if
either tool is not found the tests will be disabled at runtime.

### Online Mode

In offline mode - when `CA_ASSEMBLE_SPIRV_LL_LIT_TESTS_OFFLINE`
is disable in cmake - the test suite will build its SPIR-V binaries at test
runtime. Each test builds its own binary.

This mode is suited to systems where the tools are not necessarily known at
build time. It also enables quick turn-around time for developers when
iterating on tests, as there are no dependencies on test execution except the
test files themselves.

### Switching Between Modes

It is possible to switch between modes at runtime by setting the
``spirv-ll-online`` parameter:

```
> # With ca-lit
> ca-lit --param spirv-ll-online=0 modules/compiler/spirv-ll/test
> # Or LLVM's lit
> /usr/bin/lit --param spirv-ll-online=1 build/modules/compiler/spirv-ll/test
> # Or with the check target via the LIT_OPTS environment variable
> LIT_OPTS="--param spirv-ll-online=1" ninja check-spirv-ll-lit
```

Explicitly enabling the option with a truthy value forcibly enables the online
mode. Explicitly disabling the option with a falsy value forcibly enables
offline mode. Omitting the option leaves the behaviour to the default enabled
by CMake.

[lit]: https://pypi.org/project/lit/
