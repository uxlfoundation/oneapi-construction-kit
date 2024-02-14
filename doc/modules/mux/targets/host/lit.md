# Host lit test suite

A test suite based around LLVM's lit tool for executing tests and summarizing
their results. Focusing on testing compiler output for OpenCL kernels built
through the `oclc` frontend or through LLVM's `opt` for our host Mux target.

## Writing tests

When adding tests to the test suite which test functionalities not currently
covered, a new directory should be created inside `host/test/lit`. `llvm-lit`
will discover tests contained in it automatically.

To reduce overhead from copying files, rather than having separate source files
and test transcripts, each new test should combine the input and `FileCheck`
directives. These directives are inserted as comments in the input file and
begin with the `RUN` command using a `%s` substitution representing the path to
the file currently being run. As a convention the `RUN` directives should appear
at the beginning of the file after the copyright notice.

`llvm-lit` looks for tests based on a file extension, which is defined in a
config file `lit.cfg` under variable `config.suffixes`. `llvm-lit` locates this
config by searching upwards from the input path until it finds a file called
`lit.cfg`.

Inside each test after compilation with `oclc`, or passing thought `opt`, the
`FileCheck` tool from LLVM is used to verify that the generated output matches
the expectations from the test transcript. `FileCheck` asserts this using
pattern matching with the `CHECK` directive.

## Building

While the lit tests themselves do not need to be compiled, their configuration
files are configured through CMake in order to set up correct paths and other
parameters. The tests are built together with the other test suites when the
`cl` API is enabled via the `CA_ENABLE_API` CMake option, but they
can also be built individually through the `host-lit` target.

In order for the tests to be built, the `opt`, `FileCheck`, and `lit` tools need
to be in a path (E.G. the `PATH`) that CMake can find them. If CMake is not able
to find them, the user needs to provide the paths manually, through the
`CA_LLVM_{OPT,FILECHECK,LIT}_PATH` variables. If these files are not found,
**all** the lit tests will be disabled.

* The `opt` and `FileCheck` are built by LLVM. `opt` will additionally be
  installed by default in the install directory specified at build time. For
  `FileCheck` to also be installed alongside `opt`, the `LLVM_INSTALL_UTILS`
  option needs to be set when running the CMake for building LLVM.
  Alternatively, the tool can probably be found in the `bin` folder in the LLVM
  build directory.
* The `lit` tool is a Python package and can be installed through pip or any
  other Python package manager. The `llvm-lit` tool is used internally by LLVM
  and it **will not** be installed alongside `FileCheck`. It can be found in the
  same build directory as `FileCheck` though.

## Executing

To run host lit tests simply invoke the `lit` tool from the command line,
passing in either a directory of tests or an individual test to run. To see more
information about test failures you can pass the `--verbose` flag to `lit`. Or
to debug the test suite itself `--debug` can be set.

Sample usage:

```console
$ lit host/test/lit/ --xunit-xml-output=lit_junit.xml
-- Testing: 172 tests, 4 threads --
```

Alternative, you can use the `llvm-lit` wrapper, which can also generate JUnit
XML output with the `--xunit-xml-output` option.

Sample usage:

```console
$ llvm-lit host/test/lit/ --xunit-xml-output=lit_junit.xml
-- Testing: 172 tests, 4 threads --
```

A third way is by invoking the CMake `check` target.

Sample usage:

```console
$ ninja check-ock-host-lit
[1/1] Running host-lit checks
```

Note that the `host/test/lit` directory is the one found in the build directory
and not the one in the source directory. Attempting to run the tests in the
source directory will fail as the paths of the files required will not have been
set.
