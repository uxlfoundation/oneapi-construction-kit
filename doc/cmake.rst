CMake Development
=================

`CMake <https://cmake.org/>`_ is the industry standard build tool for
multi-platform C++ codebases used by the oneAPI Construction Kit. This document
covers best practices for CMake development and conventions within the project,
but is not intended as a CMake tutorial.

oneAPI Construction Kit uses *modern* CMake, regarded as version 3.0 and later,
with the minimum required version enforced in the root ``CMakeLists.txt`` by
`cmake_minimum_required`_.

.. seealso::

 For general tips on writing CMake see the internal Codeplay knowledge sharing
 talk by Morris Hafner "Improve Your CMake With These 17 Weird Tricks".

.. _cmake_minimum_required:
 https://cmake.org/cmake/help/latest/command/cmake_minimum_required.html

CA Modules
###########

Modules are files containing CMake code named ``<modulename>.cmake``, which get
loaded and run by ``CMakeLists.txt`` files using the `include`_ command.
The oneAPI Construction Kit provides the following modules to aid build system
development.

.. toctree::
   :maxdepth: 2

   cmake/AddCA
   cmake/Bin2H
   cmake/CAOption
   cmake/CAPlatform
   cmake/ConfigureFileScript
   cmake/Coverage
   cmake/DetectLLVMMSVCCRT
   cmake/FindModules
   cmake/Format
   cmake/ImportLLVM
   cmake/ReleaseAssert
   cmake/Sanitizers
   source/cl/cmake
   source/vk/cmake
   modules/mux/cmake

.. _include:
   https://cmake.org/cmake/help/latest/command/include.html#command:include

Toolchain Files
###############

For non-native builds CMake supports a `toolchain file`_ mechanism which
defines the path to the cross-compilation toolchain and location of non-native
system libraries. Without using a `toolchain file`_ setting flags directly in
CMake modules can be error prone. For example, setting the ``-m32`` flag
modifies CPU architecture after CMake has detected a 64-bit system, leading to
inconsistencies such as ``CMAKE_SIZE_OF_VOID`` being ``8`` rather than ``4``,
and CMake looking for 64-bit libraries in the native path. Using a
`toolchain file`_ will also implicitly set the `CMAKE_CROSSCOMPILING`_ flag
if the module sets `CMAKE_SYSTEM_NAME`_ as ours do, pruning the need for an
extra user passed commandline option.

The oneAPI Construction Kit stores toolchain files in the root ``platform``
directory for all of the cross-compilation platforms the project supports. Our
Arm Linux platform makes use of the `CMAKE_CROSSCOMPILING_EMULATOR`_ CMake
feature with `QEMU`_ to emulate 64-bit and 32-bit Arm architectures as part of
``platform/arm-linux/aarch64-toolchain.cmake`` and
``platform/arm-linux/arm-toolchain.cmake``. Utilizing an emulator allows us to
run our check targets natively to verify cross-compiled builds, which although
slower and more memory constrained than native, is valuable option when
hardware is unavailable.

.. _toolchain file:
 https://cmake.org/cmake/help/latest/variable/CMAKE_TOOLCHAIN_FILE.html
.. _CMAKE_CROSSCOMPILING:
 https://cmake.org/cmake/help/latest/variable/CMAKE_CROSSCOMPILING.html
.. _CMAKE_SYSTEM_NAME:
 https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_NAME.html
.. _CMAKE_CROSSCOMPILING_EMULATOR:
 https://cmake.org/cmake/help/latest/variable/CMAKE_CROSSCOMPILING_EMULATOR.html
.. _QEMU:
 https://www.qemu.org

Shared Library Naming and Versioning
####################################

Both OpenCL and Vulkan APIs are designed to work through an ICD loader, a
shared library named ``OpenCL`` and ``Vulkan`` respectively in the form
``lib<name>.so`` or ``<name>.dll``, depending on if the platform is Linux or
Windows. The ICD loaders gets linked to application code so that at runtime the
ICD loader will select the implementation of the standard to run
and load the chosen shared library, allowing multiple platforms to coexist on
the same system.

To avoid naming collisions with the ICD our OpenCL and Vulkan builds by default
are named ``CL`` and ``VK``, however these can be overridden via the
``CA_CL_LIBRARY_NAME`` and ``CA_VK_LIBRARY_NAME`` options. Providing customer
teams with the flexibility to deliver on embedded systems without an ICD loader,
where only our platform will exist and can be linked to directly by the
application.

To act as the ICD loader, as well mimicking the library name we also need to
replicate the library version. Shared libraries in CMake have two
versioning properties, a `VERSION`_ property for the build version and a
`SOVERSION`_ property for the API version. We set `VERSION`_ in the form
``major.minor``, where the major component is incremented on API breaking
changes and minor for non-breaking API changes, e.g bug fixes. The `SOVERSION`_
property can then be set as the major component of `VERSION`_. We default
`VERSION`_ for both OpenCL and Vulkan to our oneAPI Construction Kit
`PROJECT_VERSION`_, but provide the ``CA_CL_LIBRARY_VERSION`` option to
override that behavior for OpenCL. We don't provide an equivalent CMake
library version option for Vulkan as simulating the ICD loader is a use
case that we have yet to encounter.

On Linux an OpenCL build with our default options will result in the following
symbolic links being created to the fully qualified shared library.

.. code-block:: console

 $ ls -l build/lib/libCL.so*
 libCL.so -> libCL.so.major
 libCL.major -> libCL.major.minor
 libCL.major.minor

.. _SOVERSION:
 https://cmake.org/cmake/help/latest/prop_tgt/SOVERSION.html
.. _VERSION:
 https://cmake.org/cmake/help/latest/prop_tgt/VERSION.html
.. _PROJECT_VERSION:
 https://cmake.org/cmake/help/latest/variable/PROJECT_VERSION.html

Generator Expression Usage
##########################

The deferred evaluation of `generator expressions`_ from configuration time
until the point of build system generation provides benefits in terms of
expressibility. For example, the multi-configuration nature of MSVC generators
we support means that we don't know what the build type is at configure time,
unlike single-configuration generators which can rely on `CMAKE_BUILD_TYPE`_.
By using `generator expressions`_ we can check the ``CONFIG`` `variable query`_
on MSVC to discover the build type and change our settings accordingly.

The `variable query`_ expressions also provides a concise syntax for
conditionally including items in a list, particularly compared to appending
inside nested ``if()/else()`` control flow. We often use this paradigm for
setting compiler flags, see :doc:`/cmake/AddCA`, using `conditional expressions`_
to set the appropriate flags for the various combinations of build
configurations.

However, the exception to this is using `generator expressions`_ with a list of
source files. This is not supported by multi-configuration MSVC generators
where files must be known by CMake at configure time, and can't be deferred for
later optional inclusion. A possible workaround for this is defining a separate
library which is only linked into the target when the condition expression
evaluates to True.

.. warning::

  Using generator expressions for source files will result in the MSVC error
  message "*Target <target name> has source files which vary by
  configuration.*"

.. _generator expressions:
 https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html
.. _conditional expressions:
 https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html#conditional-expressions
.. _variable query:
 https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html#id2
.. _CMAKE_BUILD_TYPE:
 https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html
