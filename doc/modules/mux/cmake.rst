ComputeMux CMake
================

The CMake for ComputeMux aims to make it as easy as possible for customer teams
to integrate their compiler and runtime target into the oneAPI Construction Kit
build.

``modules/mux/CMakeLists.txt`` creates a static library target ``mux`` using
:cmake:command:`add_ca_library` that frontend APIs implemented by the oneAPI
Construction Kit, such as OpenCL, build on. ComputeMux library code
provides entry points to the Mux API which perform implementation agnostic error
checking of argument invalid usage, and then direct control flow through to a
desired Mux implementation. The :cmake:variable:`CA_MUX_ENABLE_SHARED` option can
be set to 'TRUE' if ``mux`` should be built as a shared library.

To allow customer teams to hook their ComputeMux implementation into
the oneAPI Construction Kit `ComputeMux CMakeLists`_ provides the functions
:cmake:command:`add_mux_target` and :cmake:command:`add_mux_compiler_target`
which can be invoked by the customer code.

.. seealso::
  See :ref:`modules/mux:Runtime CMake Integration` for a detailed description
  on adding a target to Mux.

Once Mux targets have been integrated by hooking into our CMake, the variable
:cmake:variable:`MUX_TARGET_LIBRARIES` will contain a list
of Mux implementations to include in the build. So that our ``mux`` static
library code can detect and select between these libraries we generate header
files  `config.h <#mux-config-script>`_ and
`select.h <#mux-api-generate-cmake-target>`_.

.. seealso::
 The root oneAPI Construction Kit ``CMakeLists.txt`` provides
 :cmake:command:`ca_option` :cmake:variable:`CA_MUX_TARGETS_TO_ENABLE` to overwrite
 ``MUX_TARGET_LIBRARIES`` with forced Mux targets. Option defaults to an empty
 string, enabling all targets found in the source tree.

ComputeMux CMakeLists
---------------------

``modules/mux/targets/CMakeLists.txt`` provides the following variables
and functions to be used when incorporating a ComputeMux target into the build.

.. cmake-module:: ../../../modules/mux/targets/CMakeLists.txt

``modules/compiler/targets/CMakeLists.txt`` provides the following variables
and functions to be used when incorporating a ComputeMux compiler into the
build.

.. cmake-module:: ../../../modules/compiler/targets/CMakeLists.txt

Host ComputeMux Target CMake
----------------------------

``modules/mux/source/host/CMakeLists.txt`` integrates our :doc:`/modules/host`
Mux target into the build and defines the following options implemented
through :cmake:command:`add_ca_option`.

.. cmake-module:: ../../../modules/mux/targets/host/CMakeLists.txt

mux-config Script
------------------

``modules/mux/cmake/mux-config.cmake`` contains a CMake script that is
run as part of the ``mux-config`` custom target as a dependency of the
``mux`` target.

.. cmake-module:: ../../../modules/mux/cmake/mux-config.cmake

ComputeMux Runtime API Generate CMake Target
--------------------------------------------

``modules/mux/tools/api/CMakeLists.txt`` defines a target
``mux-api-generate`` for generating C++ header file based on available Mux
targets.

It is a convenient target for the oneAPI Construction Kit developers making Mux
API changes to run. Developers can modify the ``mux.xml`` schema and then run
the target, invoking Python scripts that update generated C++ header files, bump
Mux version numbers, and add a stub ``TODO`` in the Mux ``changes.md`` document
for the developer to replace.

.. cmake-module:: ../../../modules/mux/tools/api/CMakeLists.txt
