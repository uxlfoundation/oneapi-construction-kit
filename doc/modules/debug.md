# Debug Module

This module is optionally compiled, by default it is disabled for all build
types to avoid exposing all symbols by default. This can be controlled using
the `CA_ENABLE_DEBUG_SUPPORT` CMake option.

## Debug Pass Instrumentations

The Debug module contains helper pass instrumentations to facilitate the
debugging of the oneAPI Construction Kit at runtime using environment variables.

The list of available variables can be seen in the [developer
guide](../developer-guide.md#debugging-the-llvm-compiler).

## Debug Backtrace

The debug module provides the `DEBUG_BACKTRACE` macro which prints a backtrace
of the call stack to `stderr` including file and line information about where
the macro was placed. This is especially useful when debugging a bug which only
shows itself in Release mode.

> Debug backtrace requires `CA_ENABLE_DEBUG_BACKTRACE=ON` in order to be enable
> building the `debug-backtrace` library, `CA_ENABLE_DEBUG_SUPPORT=ON` is not
> required. Backtrace support is kept separate as using it requires the
> `debug-backtrace` library to be force include in all targets to function
> correctly, this also disables symbol stripping from API shared libraries which
> can cause linking errors downstream.

To print a backtrace, include the header then add the macro inside a function
body as shown below for `function_of_interest`.

```cpp
#include <debug/backtrace.h>

void function_of_interest(state_t state) {
  DEBUG_BACKTRACE
  // ...
}
```
