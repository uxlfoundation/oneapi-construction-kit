UnitCL
======

UnitCL is an OpenCL API unit test suite, built on the Google Test unit test
framework. It tests all aspects of an OpenCL implementation, including API call
correctness, kernel compilation, kernel execution, and extension support.

UnitCL focuses on both positive and negative testing. It tests that the
implementation behaves as specified when given valid inputs, and that it fails
correctly and gracefully when given invalid inputs. Conformant OpenCL
implementations are permitted to fail (even catastrophically) with invalid
inputs. However, UnitCL has been designed with the pragmatic assumption that it
is much better for an OpenCL implementation to fail gracefully and not to
segfault.

UnitCL is capable of testing both OpenCL 1.2 and OpenCL 3.0 specifications.

.. note::

  OpenCL 3.0 support is not yet complete.

Target-Specific UnitCL Tests
----------------------------

If you have additional tests that you would like to be run as part of UnitCL,
then these can be added using the following steps. This is done to avoid
conflicts and to allow you to keep project specific tests separate.

There is just one cmake variable that needs to be set:

.. cmake:variable:: ${MUX_TARGET}_EXTERNAL_UNITCL_SRC

  The list of source files that you want to add into UnitCL.

Where ``${MUX_TARGET}`` is the same registered name in
``${MUX_TARGET_LIBRARIES}`` for the particular project. UnitCL will search all
the names and append all source files related to them into UnitCL.

An example of a project specific test would be one related to the testing of a
vendor extension. If your test is generic OpenCL, it is preferable to add this
to the main UnitCL test suite so that it can benefit all targets.

.. code:: cmake

  # The list of source files that you want to add into UnitCL.
  set(${MUX_TARGET}_EXTERNAL_UNITCL_SRC
    dir/to/your_test_source.cpp
    dir/to/additional_test_source.cpp)

If your test requires additional include directories, then these can be
appended to ``${${MUX_TARGET}_EXTERNAL_UNITCL_INC}``. These directories are
added to the include search path when UnitCL is built. If, for example, your
test is for a vendor extension that adds an API entry point, then the external
include directories will contain the header for the entry point.

If your test uses a kernel in a separate file (i.e., the test's kernel is not
an in-line string), then you can add the file by appending to
``${${MUX_TARGET}_UNITCL_KERNEL_FILES}`` similarly to how you would do it for
source files. If you are using a kernel in a separate file, then you are
`Writing Kernel Execution Tests`_.

If your tests should only be run on your device, then add a device name check
to either the relevant ``SetUp()`` functions or at the top of the individual
tests.  The ``isDevice_<MUX_TARGET>()`` functions from ``Device.h`` make this
easy. They are automatically generated from device names registered in
``{MUX_TARGET_LIBRARIES}``. A test that can only run on ``host`` might start
like this:

.. code:: cpp

  #include "Device.h"
  // ...
  if (!UCL::isDevice_host(UCL::getDevices()[0])) {
    GTEST_SKIP();
  }

Executing UnitCL
----------------

To run UnitCL to test against your application, you can simply run the following
and it will run all of the test cases!

.. code:: console

  $ <build>/bin/UnitCL

The above will use the system configured OpenCL, generally this will run via the
ICD. If you would like to link with a particular OpenCL then for example on
Windows you can copy the ``libOpenCL.dll`` into the local folder, or on Linux
you can use a command similar to:

.. code:: console

  $ LD_LIBRARY_PATH=<search/path/for/lib> <build>/bin/UnitCL

Kernel execution tests currently use a relative directory and as such require
UnitCL to be called from the root of the build directory to find the kernel
folder (located within ``bin``). If you'd prefer you can specify
``--unitcl_kernel_directory`` and point this to wherever the kernel directory
is.

.. code:: console

  $ <build>/bin/UnitCL --unitcl_kernel_directory=<path/to/kernels>

UnitCL will skip test cases that are not applicable to the particular OpenCL
device. For example, if the device does not support the ``cl_khr_fp16``
extension, then test cases that use half precision floating point are skipped.
When the device does not have a compiler (which may be the case for some
devices that only support the embedded profile), then a large number of test
cases will be skipped. Many test cases depend on kernels from source, and these
require a compiler.

UnitCL Options
--------------

As ``UnitCL`` is built upon Google Test, any of the existing options that
Google Test allows are valid. Below is the output from ``UnitCL -h`` showing
the flags that are supported by Google Test;

This program contains tests written using Google Test. You can use the
following command line flags to control its behavior:

Test Selection
^^^^^^^^^^^^^^

* ``--gtest_list_tests`` - List the names of all tests instead of running them,
  The name of ``TEST(Foo, Bar)`` is ``Foo.Bar``.
* ``--gtest_filter=POSITIVE_PATTERNS[-NEGATIVE_PATTERNS]`` - Run only the tests
  whose name matches one of the positive patterns but none of the negative
  patterns; ``?`` matches any single character; ``*`` matches any substring;
  and ``:`` separates two patterns.
* ``--gtest_also_run_disabled_tests`` - Run all disabled tests too.

Test Execution
^^^^^^^^^^^^^^

* ``--gtest_repeat=[COUNT]`` - Run the tests repeatedly; use a negative count
  to repeat forever.
* ``--gtest_shuffle`` - Randomize tests' orders on every iteration.
* ``--gtest_random_seed=[NUMBER]`` - Random number seed to use for shuffling
  test orders (between 1 and 99999, or 0 to use a seed based on the current
  time).

Test Output
^^^^^^^^^^^

* ``--gtest_color=(yes|no|auto)`` - Enable/disable colored output. The default
  is auto.
* ``--gtest_print_time=0`` - Don't print the elapsed time of each test.
* ``--gtest_output=xml[:DIRECTORY_PATH/|:FILE_PATH]`` - Generate an XML report
  in the given directory or with the given file name. ``FILE_PATH`` defaults to
  ``test_details.xml``.
* ``--gtest_stream_result_to=HOST:PORT`` - Stream test results to the given
  server.

Assertion Behavior
^^^^^^^^^^^^^^^^^^

* ``--gtest_death_test_style=(fast|threadsafe)`` - Set the default death test
  style.
* ``--gtest_break_on_failure`` - Turn assertion failures into debugger
  break-points.
* ``--gtest_throw_on_failure`` - Turn assertion failures into C++ exceptions.
* ``--gtest_catch_exceptions=0`` - Do not report exceptions as test failures.
  Instead, allow them to crash the program or throw a pop-up (on Windows).

Except for ``--gtest_list_tests``, you can alternatively set the corresponding
environment variable of a flag (all letters in upper-case). For example, to
disable colored text output, you can either specify ``--gtest_color=no`` or set
the ``GTEST_COLOR`` environment variable to no.

For more information, please read the `Google Test documentation`_. If you find
a bug in Google Test (not one in your own code or tests), please `report it`_.

.. _Google Test documentation: https://github.com/google/googletest
.. _report it: googletestframework@googlegroups.com

UnitCL-Specific Options
^^^^^^^^^^^^^^^^^^^^^^^

UnitCL also parses some extra command line options above and beyond what Google
Test provides:

* ``--gtest_filter=-*UNSPECIFIED*`` - Disable all tests for behavior not
  mandated by the specification.
* ``--unitcl_platform=<vendor>`` - Provide an OpenCL platform vendor to use for
  testing.
* ``--unitcl_device=<device_name>`` - Provide an OpenCL device name to use for
  testing.
* ``--unitcl_test_include=<path>`` - Provide the path to the supplied
  ``test_include`` directory. Current setting: ``test/UnitCL/test_include``
* ``--unitcl_kernel_directory=<device_name>`` - Provide the path to the
  supplied kernels directory
* ``--unitcl_build_options=<option_string>`` - Provide compilation options to
  pass to ``clBuildProgram`` when compiling kernels in the 'kernels' directory.
* ``--unitcl_seed=<unsigned>`` - Provide an unsigned integer to seed the random
  number generator with.
* ``--unitcl_math={quick, wimpy, full}`` - Run math builtins tests over an
  increasing data size, defaults to wimpy.
* ``--vecz-check`` - Mark tests as failed if the vectorizer did not vectorize
  them.
* ``--opencl_info`` - Print OpenCL platform and devices info

Default Test Variations
-----------------------

When running UnitCL via the oneAPI Construction Kit ``check-ock-UnitCL*`` CMake
targets, various compiler configurations will be run. For example, there is a
configuration that sets ``-cl-opt-disable`` (``check-ock-UnitCL-opt-disable``),
one that sets ``-cl-wfv=always`` (``check-ock-UnitCL-vecz``), etc. Some of
these variations may have no effect on some customer targets. E.g., if a
ComputeMux target does not use Vecz, then the ``check-ock-UnitCL*`` CMake
targets testing Vecz will effectively duplicate the ``check-ock-UnitCL*`` CMake
targets that don't test Vecz.

To reduce the amount of duplicated testing, many of these ``check-ock-UnitCL*``
CMake targets run UnitCL with a filter of common compiler-related words.  This
way, only tests that match one of these keywords are run more than once.  The
filter used is a good match for the default UnitCL tests, but it is only
convention. It is possible that additional tests are added by a customer team
for a specific ComputeMux target that *do* test the compiler but *do not* match
any of the filter words.

.. warning::

  If the intent of a UnitCL test is to test the compiler and the various
  compiler configurations, then the test name **must** include one of the
  compiler-related keywords. Otherwise, the ``check-ock-UnitCL*`` CMake targets
  will not run the test.

.. note::

  The filter of compiler-related keywords is stored in the CMake variable
  ``CA_CL_COMPILER_TEST_FILTER``. It currently selects tests that have any of
  ``Compile``, ``Link``, ``Build``, ``Execution``, or ``print`` in the name.

Kernel Execution Tests
----------------------

Regular UnitCL tests are primarily intended to verify the behavior of OpenCL
API entry points. These tests check various parameter combinations to API
calls, check that correct error codes are returned, etc. Kernel execution
tests, on the other hand, test that kernels are compiled and executed
correctly. Execution tests test compiler corner cases, the precision of math
functions, and so on. The key difference between the two types of tests is that
the kernels used by execution tests are always stored in separate files.
Regular UnitCL tests sometimes don't even need kernels, when they do the
kernels are usually defined in-line, and separate kernel files are only rarely
used.

There is some obvious overlap between the two types of tests, since regular
tests sometimes need to execute kernels, and execution tests rely on API entry
points functioning correctly.

By default, writing a single execution test produces six different tests in
UnitCL, each one exercising a different code path. The six test types are:

* ``Execution`` - The driver compiles the OpenCL-C kernel to binary and
  executes it.
* ``OfflineExecution`` - The driver executes a binary kernel that has
  previously been compiled using ``clc``.
* ``SpirvExecution`` - The driver compiles a SPIR-V version of the kernel to
  binary and executes it.
* ``OfflineSpirvExecution`` - The driver executes a binary kernel that has
  previously been compiled from SPIR-V using ``clc``.

The test type is used as the prefix for the test name in UnitCL. Writing a test
``foo`` will produce tests named ``Execution.foo``, ``OfflineExecution.foo``,
etc. As far as GoogleTest is concerned, there is nothing special about these
generated tests, and they can be filtered just like regular tests:

.. code:: console

  # Run all six variations of the foo test
  ./UnitCL --gtest_filter=*foo

  # Run all SPIR-V execution tests
  ./UnitCL --gtest_filter=SpirvExecution*

The compilation-to-binary step of the three types of offline tests happens
during UnitCL build time using ``clc``. Since ``clc`` is based on the same code
as the OpenCL driver itself, even just building UnitCL partially tests the
OpenCL implementation.

The following sections provide more details on the process of writing execution
tests and how UnitCL is built. See `Generating SPIR-V`_ for creating SPIR-V
versions of the kernel.

Writing Kernel Execution Tests
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To write a Kernel Execution Test, you will need to write two things: a C++ part
that will include the test in the test suite and define the execution
environment of the kernel (inputs, expected outputs, work size etc.), as well
as the actual OpenCL-C kernel that you want to test.

Files related to Kernel Execution Tests may show references to the name KTS.
These are remnants from when it was an entirely separate test suite known as the
Kernel Test Suite (KTS).

The C++ test and the OpenCL-C kernel are linked through their names only, the
name of the file and of the kernel are inferred from the name of the test in the
following way:

* Format for test names: ``TestSet_X_KernelName``, where X is a two digit
  number and where ``_X_`` is the last underscore two digit combination that
  occurs in the test name.
* Example name: ``Task_01_01_Copy``.
* Kernel file to load: ``task_1.01_copy.cl``.
* Kernel name: ``copy``.

.. note::

  Kernel files must be located in the ``UnitCL/kernels`` directory. The kernel
  files are copied into the build directory at build time by the `copy-kernels
  target`_ , so you must add your kernel to ``kernels/CMakeLists.txt``. You can
  also use the ``--unitcl_kernel_directory`` argument to specify in which
  directory to look for the kernel files.

To write the C++ test and set up the execution environment of your kernel, KTS
provides convenient utility functions:

* ``RunGeneric1D``: Will launch the kernel, it takes the global size as a first
  parameter and the local size as an optional second parameter. The default
  local size is 0. KTS provide ``kts::N`` and ``kts::localN`` as defaults
  global and local size that can be used in tests.
* ``RunGenericND``: Will launch the kernel, it takes the number of dimensions
  as a first parameter, a pointer to the global dimensions as the second and a
  pointer to the local dimensions as an third parameter.
* ``AddInputBuffer``: Will add an input buffer to the kernel, its first
  parameter is the size of the buffer, and the second is a ``kts::Reference1D``
  function that will be used to generate the content of the buffer.
* ``AddOutputBuffer``: Will add an output buffer to the kernel, its first
  parameter is the size of the buffer, and the second is a ``kts::Reference1D``
  function that should generate the expected output of the kernel, it will be
  used to check the actual output of the kernel.
* ``AddInOutBuffer``: Will add an input and output buffer to the kernel. The
  first parameter is the size of the buffers. The second is a
  ``kts::Reference1D`` that will generate the content of the input buffer
  whilst the third is a ``kts::Reference1D`` that will be used to check the
  actual output of the kernel.
* ``AddPrimitive``: Will add a by value parameter to the kernel, and it takes
  only one parameter, the actual value to pass to the kernel.
* ``AddMacro``: Will add a macro in the kernel, the first parameter of this
  function is the name of the macro, and the second is an unsigned value
  representing the value of the macro.

.. note::

  The order in which the functions to set the parameters are called must match
  the order of the matching kernel arguments.

To generate the input and output of a kernel, KTS requires the user to provide
functions matching the type of ``kts::Reference1D``, this is a templated type
that matches a function taking an ``int`` and returning a value of the template
parameter type. The ``int`` value passed to the function is the global id for
which we want to generate a value.

``kts::BuildVecXReference1D`` (with X a vector size in 2,3,4) is provided to
allow extrapolation of a ``kts::Reference1D`` for a vector type from a
``kts::Reference1D`` for the matching scalar type.

A set of common ``kts::Reference1D`` functions is also provided for use with
kernel execution tests, available in ``source/kts_reference_functions.h``.

The ``TEST_P(Execution, TEST_NAME)`` macro is used to create execution tests.
The ``Execution`` fixture is parameterized over the source type of the OpenCL
program to test:

.. cpp:enum:: kts::ucl::SourceType

   .. cpp:enumerator:: OPENCL_C

      Loads an OpenCL C (``.cl``) source  file from disk.

   .. cpp:enumerator:: SPIRV

      Loads a SPIR-V (``.spv32``/``.spv64``) file from disk.

   .. cpp:enumerator:: OFFLINE

      Loads a  pre-compiled ``.cl`` binary (``.bin``) file from disk.

   .. cpp:enumerator:: OFFLINESPIRV

      Loads a pre-compiled ``.spv32``/``.spv64`` (``.bin``) file from disk.

Sometimes it may be necessary to skip one or more source types. For example, a
test might trigger a known OFFLINE bug so the OFFLINE versions need to be
skipped, or a test might only be valid when compiled just-in-time from OpenCL
C. In these cases the :cpp:func:`kts::ucl::isSourceTypeIn()` utility function
should be used to determine if a test should be skipped:

.. code:: cpp

   TEST_P(Execution, Test) {
     if (!isSourceTypeIn({OPENCL_C, SPIRV}) {
       GTEST_SKIP();
     }
   }

.. hint::
   In addition to the ``Execution`` fixture the following more specific fixtures
   are also available:

   * ``ExecutionOpenCLC`` - for :cpp:enumerator:`kts::ucl::SourceType::OPENCL_C`
     and :cpp:enumerator:`kts::ucl::SourceType::OFFLINE`
   * ``ExecutionSPIRV`` - for :cpp:enumerator:`kts::ucl::SourceType::SPIRV` and
     :cpp:enumerator:`kts::ucl::SourceType::OFFLINESPIRV` source types

Writing Parameterized Tests
^^^^^^^^^^^^^^^^^^^^^^^^^^^

When a parameterized test is needed, the ``ExecutionWithParam`` fixture template
should be used in conjunction with the ``UCL_EXECUTION_TEST_SUITE_P`` macro to
instantiate the test suite.

.. code:: cpp

   using MyExecutionWithInt = ExecutionWithParam<int>;

   // Instantiates the test suite over all source types and 3 int values.
   UCL_EXECUTION_TEST_SUITE_P(MyExecutionWithInt,
                              testing::ValuesIn(kts::ucl::getSourceTypes()),
                              testing::Values(23, 42, 88));

   TEST_P(MyExecutionWithInt, Test) {
     // Use getParam() instead of GetParam() to access the int parameter, this
     // is provided by the ExecutionWithParam fixture to make accessing the non
     // source type parameter easier.
     int i = getParam();
     // ...
   }

.. note::

   When using macros in your test, you will need to add a ``// CLC OPTIONS:
   -D<...>`` comment to the OpenCL C kernel so that the macro definitions are
   known when the offline versions of the kernel are compiled. If the macros are
   dynamically specific at runtime, the offline variables should be disabled.

.. _SPIRV: https://www.khronos.org/registry/spir-v/
.. _OFFLINE: ../tools.html

Offline Execution Testing through CLC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As part of our offline testing, the UnitCL build target will compile OpenCL-C
and SPIR-V kernels through ``clc`` to produce output binaries that are later
loaded by UnitCL. To control how these are handled, you can add the following
comments to the OpenCL-C kernel. These are parsed by CMake.

* ``// CLC OPTIONS:`` - A semicolon-separated list of values that are to be
  passed to ``clc`` as options. For example ``// CLC OPTIONS:
  -cl-mad-enable;-DDEF1=7;-DOPTION_2=ON``
* ``// REQUIRES:`` - A semi-colon separated list of values representing the
  requirements of the kernel. For example ``// REQUIRES: double; images``

.. note::

  The options defined with ``CLC OPTIONS`` are also passed to clang as part of
  the `regenerate-spirv target`_.

Here is the full list of supported requirements for ``// REQUIRES:``:

* ``noclc`` - Skip all ``clc`` steps for this kernel. Use this when you have a
  kernel that is not supported by ``clc``. ``OfflineExecution`` and
  ``OfflineSpirvExecution`` tests will not work for this kernel if this
  requirement is set.
* ``nospirv`` - Skip all SPIR-V generation steps for this kernel. This will
  only have an effect if you are running the
  :ref:`regenerate-spirv<regenerate-spirv>` target. ``SpirvExecution`` and
  ``OfflineSpirvExecution`` tests will not work for this kernel if this
  requirement is set.
* ``double`` - If your kernel requires doubles (i.e., the ``cl_khr_fp64``
  extension), then CMake will only compile the kernel for targets with the
  ``fp64`` capability.
* ``half`` - If your kernel requires halfs (i.e., the ``cl_khr_fp16``
  extension), then CMake will only compile the kernel for targets with the
  ``fp16`` capability.
* ``images`` - If your kernel requires image support, then CMake will only
  compile the kernel when ``host`` has images enabled.
* ``parameters`` - Currently tests that are parameterized using macros do not
  support any offline compilation (either to IR or to executable). This
  requirement disables all but the ``Execution`` test type.
* ``mayfail`` - Indicates that ``clc`` may fail to compile this kernel. The
  offline binaries are therefore optional.

.. warning::

  ``// REQUIRES:`` is used to disable build steps. It has no effect on which
  tests UnitCL attempts to run. When ``noclc`` or ``nospirv`` are used, then
  the test (in C++) must set the corresponding ``TEST_F_EXECUTION_OPTIONAL``
  fields to ``KTSDISABLE`` so the test isn't generated. For tests requiring
  doubles, halfs, or images, it is common for the test (in C++) to
  conditionally skip itself if the target doesn't support the feature.
  Otherwise, UnitCL will attempt to run the test, and the test will fail due to
  a missing kernel file.

.. note::

  CMake handles image support differently from double and half support;
  checking for ``host`` image support is a work-around.

When a requirement is not met, CMake will not run ``clc``. Instead, it will
create a stub file with the same name that ``clc`` would have created. The stub
file contains the text ``Skipped due to <reason>``. Stub files allow CMake to
check for stale files; if stub files were not used, CMake would need to always
re-parse skipped kernel files in case the requirements had been changed.

The ``install`` target looks for the ``Skipped`` text to determine if a kernel
file is a stub.

When UnitCL is built, ``clc`` is called by the
`UnitCL-offline-execution-kernels target`_.

The SPIR-V kernels passed to ``clc`` are generated by the `regenerate-spirv
target`_ .

Running tests using an offline CL driver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When ``UnitCL`` is running against an offline OpenCL driver i.e. no compiler is
available, then tests which require a compiler are skipped. There are two
methods of test an offline version of CL:

* Build the oneAPI Construction Kit with the CMake option
  :cmake:variable:`CA_RUNTIME_COMPILER_ENABLED` set to ``OFF``.

* Build the oneAPI Construction Kit with the CMake options
  :cmake:variable:`CA_RUNTIME_COMPILER_ENABLED` set to ``ON``, and
  :cmake:variable:`CA_COMPILER_ENABLE_DYNAMIC_LOADER` set to ``ON``, then set
  the environment variable :envvar:`CA_COMPILER_PATH` to the empty string to
  disable loading the compiler at runtime.

``copy-kernels`` Target
^^^^^^^^^^^^^^^^^^^^^^^

The ``copy-kernels`` CMake target is built as part of UnitCL. Its purpose is to
copy kernels from the source directory to the build directory so that they are
available in a known location when UnitCL is run.

.. note::

  ``copy-kernels`` copies all kernels, including those used for regular (not
  execution) UnitCL tests.

.. note::

  ``copy-kernels`` copies stub kernels (stub files that were generated by the
  `regenerate-spirv target`_) for CMake dependency
  tracking reasons. The ``install`` target contains logic to prevent stub
  kernels from being installed.

The following diagram shows how the ``copy-kernels`` target works:

.. include:: diagram-copy-kernels.rst

* File paths are relative to the ``UnitCL/`` directory. Exceptions are paths
  with ``<target>``, which will be somewhere in the ComputeMux target's
  directory tree, and paths with ``${PROJECT_BINARY_DIR}`` or
  ``${CMAKE_INSTALL_PREFIX}``, which point to CMake's build or install
  directories.
* There are currently no hand-written ``.bc32`` or ``.bc64`` default kernels in
  UnitCL (yellow dashed ellipse), but these are theoretically supported.
* All ``.cmake`` scripts are in ``UnitCL/cmake/``.
* The ``install`` target is separate from the ``copy-kernels`` target, but it's
  shown for completeness.

``UnitCL-offline-execution-kernels`` Target
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``UnitCL-offline-execution-kernels`` CMake target is built as part of
UnitCL. It wraps ``clc`` and compiles the kernels used by ``OfflineExecution``
and ``OfflineSpirvExecution`` tests for each target device.

The following diagram shows how the ``UnitCL-offline-execution-kernels`` target
works:

.. include:: diagram-offline-kernels.rst

* See the diagram in `copy-kernels Target`_ for how the
  ``${${MUX_TARGET}_UNITCL_KERNEL_FILES}`` list is populated.
* The ``install`` target is separate from the
  ``UnitCL-offline-execution-kernels`` target, but it's shown for completeness.

Generating SPIR-V
-----------------

IR execution tests --- ``SpirvExecution`` and ``OfflineSpirvExecution`` --- use
SPIR-V kernels. The IR kernels do not change often, so they are committed to
the oneAPI Construction Kit repository. To ensure consistency, the IR kernels
are always generated with exactly the same tools.

The `regenerate-spirv target`_ is used to rebuild IR kernels. It is also
possible to build individual IR kernels `manually <Manually generate
SPIR-V_>`_, but since the process is error-prone, committing manually generated
IR kernels to the repository is discouraged.

.. _regenerate-spirv:

``regenerate-spirv`` Target
^^^^^^^^^^^^^^^^^^^^^^^^^^^

There is a custom target - ``regenerate-spirv`` - which can be used for
generating the appropriate SPIR-V ASM files from all the kernels we have in
UnitCL.

To use the ``regenerate-spirv`` target you will require the ``llvm-spirv``
tool.

* `llvm-spirv`_ (See `Compiling LLVM-SPIRV`_ for build instructions)

.. _`llvm-spirv`: https://github.com/KhronosGroup/SPIRV-LLVM-Translator/
.. _`Compiling LLVM-SPIRV`: ../../../developer-guide.html#compiling-llvm-spirv

At the time of writing (2019-11-14), the ``llvm-spirv`` tool used to generate
SPIR-V from OpenCL C was built from the the ``llvm_release_80`` branch of
SPIRV-LLVM.

As part of your CMake command set the following values:

* ``CA_EXTERNAL_LLVM_SPIRV`` the absolute path to the ``llvm-spirv``.

With these set, calling the ``regenerate-spirv`` target will build SPIR-V for
all the kernels.

.. note::

  ``regenerate-spirv`` compiles IR kernels into the source directory, **not**
  the build directory. However, temporary files are placed into the build
  directory (and later deleted).

``regenerate-spirv`` will regenerate all binaries for all kernels, *even if
those binaries are not required*. If a binary is not tracked by ``git``, then
that is probably for a good reason. For example, tests for that binary might
not yet exist. It is recommended not to add binaries to ``git`` unless that is
specifically what you intend to do. Untracked binaries can be removed with

.. code:: console

  # In UnitCL
  git clean -f source/cl/test/UnitCL/kernels/
  # In host, if present
  git clean -f modules/mux/targets/host/test/UnitCL/kernels/

The following diagram shows how the ``regenerate-spirv`` target works:

.. include:: diagram-regenerate.rst

* See the diagram in `copy-kernels Target`_ for how the
  ``${${MUX_TARGET}_UNITCL_KERNEL_FILES}`` list is populated.
* All ``.cmake`` scripts are in ``UnitCL/cmake/``.

Manually generate SPIR-V
^^^^^^^^^^^^^^^^^^^^^^^^

The ``regenerate-spirv`` target is most useful for automating generation of all
the kernels. However, sometimes you might prefer to generate these yourself by
hand.

To generate SPIR-V binaries we use ``llvm-spirv``, which works on bitcode files
similar to those generated above. Since ``llvm-spirv`` is derived from modern
versions of llvm (at the time of writing the earliest maintained version is
based on llvm 7.0) a matching version of clang should be used to generate
different files for SPIR-V generation. The invocation for this is slightly
different:

.. code:: console

  clang -c -emit-llvm -target spir-unknown-unknown -cl-std=CL1.2   \
        -Xclang -finclude-default-header                           \
        <compile options>                                          \
        -o kernel_name.bc32                                        \
           kernel_name.cl
  clang -c -emit-llvm -target spir64-unknown-unknown -cl-std=CL1.2 \
        -Xclang -finclude-default-header                           \
        <compile options>                                          \
        -o kernel_name.bc64                                        \
           kernel_name.cl

This new bitcode file can then be translated into a SPIR-V binary with the
following command:

.. code:: console

  llvm-spirv kernel_name.bc32  \
          -o kernel_name.spv32
  llvm-spirv kernel_name.bc64  \
          -o kernel_name.spv64

We don't commit the ``spv`` files to the repository since they don't commit
well and our workflow is setup assuming we have ``spvasm`` instead. ``spvasm``
is the form of the SPIR-V binary which we can get by using the ``spirv-dis``
tool.

.. code:: console

  spirv-dis kernel_name.spv32    \
         -o kernel_name.spvasm32
  spirv-dis kernel_name.spv64    \
         -o kernel_name.spvasm64
