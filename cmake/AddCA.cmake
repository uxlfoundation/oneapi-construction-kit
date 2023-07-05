# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#[=======================================================================[.rst:
``AddCA.cmake`` exposes wrappers to common CMake functions, providing a single
location to add ComputeAorta specific build options. These options may be
platform dependent, or conditional on user provided build flags.

Using these ``add_ca*`` wrappers helps ensure that customer teams' targets
get built with the same options as ComputeAorta's own targets.

To access the following commands and variables in this module:

.. code:: cmake

  include(AddCA)
#]=======================================================================]
include(CMakeParseArguments)

set(CMAKE_C_STANDARD 99)              # Enable C99 mode
set(CMAKE_C_STANDARD_REQUIRED ON)     # Require C99 support
set(CMAKE_C_EXTENSIONS OFF)           # Disable C language extensions

set(CMAKE_CXX_STANDARD 17)            # Enable C++17 mode
message(STATUS "oneAPI Construction Kit using C++${CMAKE_CXX_STANDARD}")
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # Require explicit C++XX support
set(CMAKE_CXX_EXTENSIONS OFF)         # Disable C++ language extensions

set(CA_CL_STANDARD_INTERNAL 300)
set(CA_CL_PLATFORM_VERSION_MAJOR 3)
set(CA_CL_PLATFORM_VERSION_MINOR 0)

if(NOT MSVC AND (CA_BUILD_32_BITS OR CMAKE_SIZEOF_VOID_P EQUAL 4) AND
    (CMAKE_SYSTEM_PROCESSOR STREQUAL x86 OR
        CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 OR
        CMAKE_SYSTEM_PROCESSOR STREQUAL AMD64))
  # Enable 32 bit builds when requested or detected and enable sse3
  # instructions.
  set(BUILD_32_BIT_FLAG "-m32 -msse3 -mfpmath=sse")
  string(APPEND CMAKE_C_FLAGS " ${BUILD_32_BIT_FLAG}")
  string(APPEND CMAKE_CXX_FLAGS " ${BUILD_32_BIT_FLAG}")
  string(APPEND CMAKE_ASM_FLAGS " ${BUILD_32_BIT_FLAG}")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " ${BUILD_32_BIT_FLAG}")
  string(APPEND CMAKE_MODULE_LINKER_FLAGS " ${BUILD_32_BIT_FLAG}")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " ${BUILD_32_BIT_FLAG}")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  # Use relative RPATH for ComputeAorta install's so that executables,
  # especially test suites, linking against shared libraries do not require
  # setting LD_LIBRARY_PATH.
  string(APPEND CMAKE_INSTALL_RPATH :$ORIGIN/../lib)

  if(CA_ENABLE_DEBUG_BACKTRACE)
    # For DEBUG_BACKTRACE to function symbols must be placed in the dynamic
    # symbol section, this is only enabled when requested by the user.
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -rdynamic")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS " -rdynamic")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -rdynamic")
  endif()
endif()

if(MSVC)
  # Remove flags enabling RTTI and exceptions that CMake sets by default.
  string(REPLACE "/GR" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  string(REPLACE "/EHs" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  string(REPLACE "/EHc" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

  # Work around for Jenkins failure: MT failed. with 31 by not generating a
  # manifest for runtime objects.
  string(APPEND CMAKE_EXE_LINKER_FLAGS " /MANIFEST:NO")
  string(APPEND CMAKE_MODULE_LINKER_FLAGS " /MANIFEST:NO")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " /MANIFEST:NO")
endif()

if(ANDROID)
  # Fixup the android compile flags
  string(TOUPPER ${CMAKE_BUILD_TYPE} CmakeBuildType)
  if(CmakeBuildType STREQUAL DEBUG)
    # The toolchain file is setting the -DDEBUG flag but this is breaking with
    # LLVM which defines a DEBUG macro, so just remove the flag
    string(REPLACE "-DDEBUG" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
    string(REPLACE "-DDEBUG" "" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
    string(REPLACE "-DDEBUG" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "-DDEBUG" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  endif()
endif()

#[=======================================================================[.rst:
.. cmake:variable:: CA_COMPILE_OPTIONS

  Compile options to be added in ComputeAorta wrapper commands as ``PRIVATE``
  :cmake-command:`target_compile_options` for targets. Options are both
  platform and toolchain specific, and determined based on
  `generator expressions`_.

  .. _generator_expressions:
    https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html
#]=======================================================================]
set(CA_LINK_OPTIONS)
if(CA_USE_LINKER)
  # FIXME: These can't be a generator expression until cmake 3.13 where we can
  # use `target_link_options`
  if(UNIX OR ANDROID OR MINGW)
    string(APPEND CA_LINK_OPTIONS " -fuse-ld=${CA_USE_LINKER}")
  else()
    message(WARNING "CA_USE_LINKER ignored for non-unix targets")
  endif()
endif()
if(CMAKE_SYSTEM_NAME STREQUAL Linux OR ANDROID OR MINGW)
  # 1. We don't need executable stacks and we don't want them infecting consuming
  # programs which might have strict security requirements. This ensures we're
  # not the cause of an executable stack further down the line
  # 2. text relocations confound performance on all platforms and are illegal on
  # some. Ensure we don't have them; we should never need them
  string(APPEND CA_LINK_OPTIONS " -Wl,-znoexecstack -Wl,-ztext")
endif()

add_compile_options(
  $<$<CXX_COMPILER_ID:Clang,AppleClang>:-fcolor-diagnostics>
  $<$<CXX_COMPILER_ID:GNU>:-fdiagnostics-color=always>)

set(CA_COMPILE_OPTIONS
  $<$<OR:$<BOOL:${UNIX}>,$<BOOL:${ANDROID}>,$<BOOL:${MINGW}>>:
    $<$<STREQUAL:${CMAKE_SOURCE_DIR},${PROJECT_SOURCE_DIR}>:
      -Werror             # Enable warnings as errors when not a subproject
    >
    -Wno-error=deprecated-declarations  # Disable: use of deprecated functions

    -Wall -Wextra         # Enable more warnings
    -Wno-variadic-macros  # Disable: warnings about variadic macros
    -Wformat              # Enable printf format warnings

    $<$<NOT:$<BOOL:${MINGW}>>:
      -fPIC               # Emit position-independent code
                          # Not relevant on Windows and can cause warnings on
                          # GCC-5
    >

    -ffunction-sections   # Place each function or data item into its own
    -fdata-sections       # section in the output file if the target
                          # supports arbitrary sections. The name of the
                          # function or the name of the data item determines
                          # the section's name in the output file

    $<$<COMPILE_LANGUAGE:CXX>:
      -fno-rtti             # Disable Run-Time Type Identification
      -fno-exceptions       # Disable exceptions
    >

    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
      -Wno-return-type-c-linkage # Over-zealous clang-specific warning
      $<$<OR:$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86>,$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>>:
        $<$<OR:$<BOOL:${CA_BUILD_32_BITS}>,$<EQUAL:${CMAKE_SIZEOF_VOID_P},4>>:
          -fmax-type-align=8    # Prevent emitting instructions that require 16-
                                # byte alignment on x86, because it causes seg
                                # faults
        >
      >
    >

    $<$<CXX_COMPILER_ID:AppleClang>:
      -Wno-ignored-attributes   # Disable: warn when an attribute is ignored
    >

    $<$<CXX_COMPILER_ID:GNU>:
      -Wformat-signedness   # Enable printf format signedness warnings

      $<$<AND:$<BOOL:${CA_USE_SPLIT_DWARF}>,$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>>:
        -gsplit-dwarf   # Enable generating debug info in separate .dwo files
      >
      $<$<VERSION_GREATER:$<CXX_COMPILER_VERSION>,4.8>:
        $<$<VERSION_LESS:$<CXX_COMPILER_VERSION>,5>:
          # GCC 4.8.x warns when initializing structs that contain data memeber
          # with `= {}` which is a valid usage pattern
          -Wno-missing-field-initializers
        >
      >
      $<$<VERSION_GREATER:$<CXX_COMPILER_VERSION>,5.5>:
        # GCC 6 introduced warnings when the compiler ignores attributes on
        # types which are ignored in template parameters.
        -Wno-ignored-attributes     # Disable: warn when an attribute is ignored
      >
      -Wno-psabi    # Disable: notes about ABI changes.
      $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,9>>:
        # Mimic LLVM: disable -Wredundant-move and -Wpessimizing-move on
        # GCC>=9. GCC wants to remove std::move in code like "A
        # foo(ConvertibleToA a) { return std::move(a); }", but this code does
        # not compile (or uses the copy constructor instead) on older GCCs.
        -Wno-redundant-move
        -Wno-pessimizing-move
      >
      $<$<VERSION_GREATER:$<CXX_COMPILER_VERSION>,10>:
        # GCC has become awful complainy about passing the address of an
        # uninitialized variable to another function which performs the
        # initialization itself, reconsider disabling this warning if
        # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96564 is addressed.
        -Wno-maybe-uninitialized
      >
    >
  >

  $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
    -Wthread-safety  # Enable clang thread safety analysis
  >

  $<$<BOOL:${MINGW}>:
    -Wa,-mbig-obj              # Increase number of sections in .obj file.
    -Wno-stringop-truncation   # Disable warnings on strncpy
  >

  $<$<BOOL:${MSVC}>:
    /GR-      # Disable Run-Time Type Information
    /EHs-     # Disable C++ exception handling and assume extern "C" functions
    /EHc-     # never throws a C++ exception

    $<$<OR:$<CONFIG:Release>,$<CONFIG:ReleaseAssert>>:
      /Gy       # Enable function-level linking
      /Ox       # Enable full optimization favouring execution speed over
                # smaller size
    >

    /bigobj   # Increase number of sections in .obj file.

    -WX       # Warnings as errors.

    # Warnings disabled by ComputeAorta.
    -wd4350   # Disable: behavior change
    -wd4514   # Disable: unreferenced inline function has been removed
    -wd4530   # Disable: C++ exception handler used, but unwind semantics are
              # not enabled.
    -wd4623   # Disable: default constructor could not be generated because a
              # base class default constructor is inaccessible or deleted
    -wd4625   # Disable: copy constructor could not be generated because a base
              # class copy constructor is inaccessible or deleted
    -wd4626   # Disable: assignment operator could not be generated because a
              # base class assignment operator is inaccessible or deleted
    -wd4820   # Disable: 'x' bytes of padding added after
    -wd4996   # Disable: use of deprecated functions
    # Warnings disabled by LLVM, ComputeAorta includes LLVM headers so we use
    # the same set of warnings.
    -wd4141   # Disable: 'modifier' : used more than once.
    -wd4146   # Disable: unary minus operator applied to unsigned type, result
              # still unsigned.
    -wd4180   # Disable: qualifier applied to function type has no meaning;
              # ignored.
    -wd4244   # Disable: 'argument' : conversion from 'type1' to 'type2',
              # possible loss of data.
    -wd4258   # Disable: 'var' : definition from the for loop is ignored; the
              # definition from the enclosing scope is used.
    -wd4267   # Disable: 'var' : conversion from 'size_t' to 'type', possible
              # loss of data.
    -wd4291   # Disable: 'declaration' : no matching operator delete found;
              # memory will not be freed if initialization throws an exception.
    -wd4345   # Disable: behavior change: an object of POD type constructed
              # with an initializer of the form () will be default-initialized.
    -wd4351   # Disable: new behavior: elements of array 'array' will be
              # default initialized.
    -wd4355   # Disable: 'this' : used in base member initializer list.
    -wd4456   # Disable: declaration of 'var' hides local variable.
    -wd4457   # Disable: declaration of 'var' hides function parameter.
    -wd4458   # Disable: declaration of 'var' hides class member.
    -wd4459   # Disable: declaration of 'var' hides global declaration.
    -wd4503   # Disable: 'identifier' : decorated name length exceeded, name
              # was truncated.
    -wd4624   # Disable: 'derived class' : destructor could not be generated
              # because a base class destructor is inaccessible.
    -wd4722   # Disable: function' : destructor never returns, potential memory
              # leak.
    -wd4800   # Disable: 'type' : forcing value to bool 'true' or 'false'
              # (performance warning).
    -wd4100   # Disable: unreferenced formal parameter.
    -wd4127   # Disable: conditional expression is constant.
    -wd4512   # Disable: assignment operator could not be generated.
    -wd4505   # Disable: unreferenced local function has been removed.
    -wd4610   # Disable: <class> can never be instantiated.
    -wd4510   # Disable: default constructor could not be generated.
    -wd4702   # Disable: unreachable code.
    -wd4245   # Disable: signed/unsigned mismatch.
    -wd4706   # Disable: assignment within conditional expression.
    -wd4310   # Disable: cast truncates constant value.
    -wd4701   # Disable: potentially uninitialized local variable.
    -wd4703   # Disable: potentially uninitialized local pointer variable.
    -wd4389   # Disable: signed/unsigned mismatch.
    -wd4611   # Disable: interaction between '_setjmp' and C++ object
              # destruction is non-portable.
    -wd4805   # Disable: unsafe mix of type <type> and type <type> in operation.
    -wd4204   # Disable: nonstandard extension used : non-constant aggregate
              # initializer.
    -wd4577   # Disable: noexcept used with no exception handling mode
              # specified; termination on exception is not guaranteed.
    -wd4091   # Disable: typedef: ignored on left of '' when no variable is
              # declared.
    -wd4324   # Disable: structure was padded due to __declspec(align()).
    -we4238   # Promote: nonstandard extension used : class rvalue used as
              # lvalue.
    -we4101   # Promote: unreferenced formal parameter.
  >)

#[=======================================================================[.rst:
.. cmake:variable:: CA_COMPILE_DEFINITIONS

  Compile definitions to be added in ComputeAorta wrapper commands as
  ``PRIVATE`` :cmake-command:`target_compile_definitions` for targets.
  Definitions are platform specific, and determined based on
  `generator expressions`_.

  .. _generator_expressions:
    https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html
#]=======================================================================]
set(CA_COMPILE_DEFINITIONS
  $<$<OR:$<BOOL:${UNIX}>,$<BOOL:${ANDROID}>,$<BOOL:${MINGW}>>:
    __STDC_CONSTANT_MACROS    # Define stdint.h constant macros.
    __STDC_LIMIT_MACROS       # Disable: stdint.h limit macros.
  >

  $<$<CXX_COMPILER_ID:GNU>:
    __STDC_FORMAT_MACROS      # Enable printf format macros for GCC.
  >

  $<$<BOOL:${MINGW}>:
    __USE_MINGW_ANSI_STDIO=1  # Improve printf format string support
                              # There is some confusion on the internet about
                              # the long-term support for this define, but it's
                              # not clear if there is a better alternative
  >

  $<$<BOOL:${MSVC}>:
    _CRT_SECURE_NO_WARNINGS   # Disable deprecation warnings for standard C.
    _SCL_SECURE_NO_WARNINGS   # Disable deprecation warnings for standard C++.
    WIN32_LEAN_AND_MEAN       # Reduces number of files included.
    NOMINMAX                  # Removes Windows.h min and max macros.
    $<$<BOOL:${CA_DISABLE_DEBUG_ITERATOR}>:
      _ITERATOR_DEBUG_LEVEL=0 # Disable STL iterator debugging.
    >
  >

  __CA_BUILTINS_DOUBLE_SUPPORT  # Abacus on host is always built with doubles.

  $<$<BOOL:${CA_ENABLE_DEBUG_SUPPORT}>:
    CA_ENABLE_DEBUG_SUPPORT  # Enable debug support.
  >)

#[=======================================================================[.rst:
.. cmake:variable:: CA_COMPILER_COMPILE_DEFINITIONS

  Variable for propagating user options from root ComputeAorta
  ``CMakeLists.txt`` as ``PRIVATE`` compile definitions to be set using
  :cmake-command:`target_compile_definitions` inside wrapper commands.

  Options propagated as compile definitions:

  * ``CA_RUNTIME_COMPILER_ENABLED``
  * ``CA_ENABLE_DEBUG_SUPPORT``
  * ``CA_ENABLE_LLVM_OPTIONS_IN_RELEASE``
#]=======================================================================]
set(CA_COMPILER_COMPILE_DEFINITIONS
  $<$<BOOL:${CA_RUNTIME_COMPILER_ENABLED}>:
    CA_RUNTIME_COMPILER_ENABLED   # Enable runtime compiler.

    $<$<BOOL:${CA_ENABLE_DEBUG_SUPPORT}>:
      CA_ENABLE_DEBUG_SUPPORT   # Enable debug support.
    >

    $<$<BOOL:${CA_ENABLE_LLVM_OPTIONS_IN_RELEASE}>:
      CA_ENABLE_LLVM_OPTIONS_IN_RELEASE   # Enable support for CA_LLVM_OPTIONS.
    >
  >)

# Find clang-tidy for static analysis.
if(TARGET ClangTools::clang-tidy)
  # If clang-tidy is found, tell CMake to export the compilation database.
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  # And add the tidy target to invoke static analysis.
  add_custom_target(tidy COMMENT "Tidy")

#[=======================================================================[.rst:
.. cmake:command:: add_ca_tidy

  The ``add_ca_tidy()`` function creates a tidy target which invokes
  ``clang-tidy`` on every C or C++ file passed in. The first argument is
  assumed to be the target name. When an argument does not exist on the
  filesystem or is not a C or C++ file it is ignored, this allows passing the
  same list of arguments as :cmake:command:`add_ca_library` and
  :cmake:command:`add_ca_executable`.

  Arguments:
    * ``ARGV0`` - Target name to tidy with ``tidy-${ARGV0}``
    * ``ARGN`` - C/C++ files to tidy.

  .. seealso::
    This function consumes the ``CA_CLANG_TIDY_FLAGS`` user option.
#]=======================================================================]
  function(add_ca_tidy)
    if(TARGET tidy)
      foreach(entry ${ARGN})
        get_filename_component(ext ${entry} EXT)
        if(EXISTS ${entry} AND ext MATCHES "^\.c(pp)?$")
          # In order to create a dependency graph for clang-tidy targets we
          # must use a symbolic file, this is a file which doesn't exist other
          # than to setup target dependencies.
          set(symbolic_file ${entry}.tidy)
          set_source_files_properties(${symbolic_file} PROPERTIES SYMBOLIC ON)
          list(APPEND symbolic_files ${symbolic_file})
          file(RELATIVE_PATH relative ${CMAKE_SOURCE_DIR} ${entry})
          # Add the custom clang-tidy command for the source file.
          add_custom_command(OUTPUT ${symbolic_file}
            COMMAND ClangTools::clang-tidy -quiet ${CA_CLANG_TIDY_FLAGS}
            -p ${CMAKE_BINARY_DIR}/compile_commands.json ${entry}
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR} VERBATIM
            DEPENDS ClangTools::clang-tidy COMMENT "Tidy ${ARGV0} ${relative}")
        endif()
      endforeach()
      # Add the tidy-<target> target which depends on the list of symbolic
      # files to create a parallelizable dependency graph.
      add_custom_target(tidy-${ARGV0}
        DEPENDS ${symbolic_files} COMMENT "Tidy ${ARGV0}")
      add_dependencies(tidy tidy-${ARGV0})
    endif()
  endfunction()
endif()

#[=======================================================================[.rst:
.. cmake:command:: set_ca_target_output_directory

  The ``set_ca_target_output_directory()`` macro specifies the output
  directories for static or shared libraries and executables to consistent
  locations relative to the projects binary directory.

  Arguments:
    * ``target`` - Named target to set output directory properties on.
#]=======================================================================]
macro(set_ca_target_output_directory target)
  set_target_properties(${target} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endmacro()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_library

  The ``add_ca_library()`` macro acts exactly like the default CMake
  :cmake-command:`add_library` command except that it automatically adds
  project wide compiler options and definitions to the target.
#]=======================================================================]
macro(add_ca_library)
  add_library(${ARGV})
  target_compile_options(${ARGV0}
    PRIVATE ${CA_COMPILE_OPTIONS})
  target_compile_definitions(${ARGV0}
    PRIVATE ${CA_COMPILE_DEFINITIONS} ${CA_COMPILER_COMPILE_DEFINITIONS})
  set_ca_target_output_directory(${ARGV0})
  if(COMMAND add_ca_tidy)
    add_ca_tidy(${ARGV})
  endif()
  ca_target_link_options(${ARGV0} PRIVATE "${CA_LINK_OPTIONS}")
  if(CA_ENABLE_DEBUG_BACKTRACE AND NOT ${ARGV0} STREQUAL debug-backtrace)
    # Link the debug support library into all targets when it is enabled so
    # that DEBUG_BACKTRACE can be used effectively, this is only enabled when
    # requested by the user.
    target_link_libraries(${ARGV0} PUBLIC debug-backtrace)
  endif()
endmacro()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_interface_library

  The ``add_ca_interface_library()`` macro acts exactly like our
  :cmake:command:`add_ca_library` command except that it is specialised for
  ``INTERFACE`` libraries, such as header only libraries.
#]=======================================================================]
macro(add_ca_interface_library)
  add_library(${ARGV} INTERFACE)
  target_compile_options(${ARGV0}
    INTERFACE ${CA_COMPILE_OPTIONS})
  target_compile_definitions(${ARGV0}
    INTERFACE ${CA_COMPILE_DEFINITIONS} ${CA_COMPILER_COMPILE_DEFINITIONS})
endmacro()

#[=======================================================================[.rst:
.. cmake:command:: ca_target_link_options

   ``ca_target_link_options()`` acts like a simpler
   `target_link_options` from CMake 3.13+, but not in its full generality. Once
   the minimum required version is 3.13, we can swap out all uses for the
   builtin command. This command ensure the linker is called with ``flags`` for
   all build types. To specialize flags for a particular build configuration you
   must still set `LINK_FLAGS_{RELEASE|MINSIZEREL|RELEASEASSERT}` manually with
   `set_target_properties`, being mindful not to overwrite what is already
   there.
#]=======================================================================]

function(ca_target_link_options target visibility flags)
  get_target_property(tgt_link_flags ${target} LINK_FLAGS)
  get_target_property(tgt_link_flags_release ${target} LINK_FLAGS_RELEASE)
  get_target_property(tgt_link_flags_minsizerel ${target} LINK_FLAGS_MINSIZEREL)
  get_target_property(
    tgt_link_flags_releaseassert ${target} LINK_FLAGS_RELEASEASSERT)
  if(NOT tgt_link_flags)
    set(tgt_link_flags)
  endif()
  if(NOT tgt_link_flags_release)
    set(tgt_link_flags_release)
  endif()
  if(NOT tgt_link_flags_minsizerel)
    set(tgt_link_flags_minsizerel)
  endif()
  if(NOT tgt_link_flags_releaseassert)
    set(tgt_link_flags_releaseassert)
  endif()

  set_target_properties(${target} PROPERTIES
    LINK_FLAGS "${tgt_link_flags} ${flags}"
    LINK_FLAGS_RELEASE "${tgt_link_flags_release} ${flags}"
    LINK_FLAGS_MINSIZEREL "${tgt_link_flags_minsizerel} ${flags}"
    LINK_FLAGS_RELEASEASSERT "${tgt_link_flags_releaseassert} ${flags}"
  )
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_executable

  The ``add_ca_executable()`` macro acts exactly like the default CMake
  :cmake-command:`add_executable` command except that it automatically adds
  project wide compiler options and definitions to the target.
#]=======================================================================]
macro(add_ca_executable)
  add_executable(${ARGV})
  target_compile_options(${ARGV0}
    PRIVATE ${CA_COMPILE_OPTIONS})
  target_compile_definitions(${ARGV0}
    PRIVATE ${CA_COMPILE_DEFINITIONS} ${CA_COMPILER_COMPILE_DEFINITIONS})
  set_target_properties(${ARGV0} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
  if(COMMAND add_ca_tidy)
    add_ca_tidy(${ARGV})
  endif()
  ca_target_link_options(${ARGV0} PRIVATE "${CA_LINK_OPTIONS}")
  if(CA_ENABLE_DEBUG_BACKTRACE)
    # Link the debug-backtrace library into all targets when enabled so that
    # the DEBUG_BACKTRACE macro can be used effectively.
    target_link_libraries(${ARGV0} PUBLIC debug-backtrace)
  endif()
endmacro()

#[=======================================================================[.rst:
.. cmake:command:: target_ca_sources

  The ``target_ca_sources`` macro acts exactly like the default CMake
  :cmake-command:`target_sources` macro, but it also adds the source files to
  the tidy target as ``tidy-${target_name}``.

  Arguments:
    * ``target_name`` - Named target.
#]=======================================================================]
macro(target_ca_sources target_name)
  target_sources(${ARGV})

  # As the `tidy-${target_name}` target will already exist, and you cannot use
  # `add_dependencies` for symbolic files, we need to create a new target name
  # an make the original `tidy-${target_name}` target depend on it.
  if(COMMAND add_ca_tidy)
    set(subtarget_name ${target_name}_)
    while(TARGET tidy-${subtarget_name})
      set(subtarget_name ${subtarget_name}_)
    endwhile()
    add_ca_tidy(${subtarget_name} ${ARGN})
    add_dependencies(tidy-${target_name} tidy-${subtarget_name})
  endif()
endmacro()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_subdirectory

  The ``add_ca_subdirectory`` macro acts exactly like the default CMake
  :cmake-command:`add_subdirectory` macro except that it automatically adds
  project wide compiler options and definitions to the target.

  Arguments:
    * ``directory`` - Path of directory to add.
#]=======================================================================]
function(add_ca_subdirectory directory)
  # Extract base directory name from the path and upper case it.
  string(TOUPPER ${directory} prefix)
  string(REPLACE "\\" "/" prefix ${prefix})
  string(REGEX REPLACE ".*/(.*)" "\\1" prefix "${prefix}")
  # Set variables containing compile options and definitions.
  set(${prefix}_COMPILE_OPTIONS ${CA_COMPILE_OPTIONS})
  set(${prefix}_COMPILE_DEFINITIONS ${CA_COMPILE_DEFINITIONS})
  add_subdirectory(${directory})
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_example_subdirectory

  The ``add_ca_example_subdirectory()`` function delays inclusion of a
  subdirectory containing targets relating to OpenCL or Vulkan examples
  until after those API source directories have been added to the CMake tree.

  Arguments:
    * ``directory`` - Path to example directory.

  Variables:

  .. cmake:variable:: CA_EXAMPLE_DIRS

    Internally cached variable holding a list of example source directories.
    Used by root ComputeAorta ``CMakeLists.txt`` as input to
    :cmake-command:`add_subdirectory` once the API dependencies have been
    satisfied.
#]=======================================================================]
set(CA_EXAMPLE_DIRS "" CACHE INTERNAL "List example source directories")
function(add_ca_example_subdirectory directory)
  set(exampleSourceDir ${CMAKE_CURRENT_SOURCE_DIR}/${directory})
  list(APPEND CA_EXAMPLE_DIRS "${exampleSourceDir}")
  set(CA_EXAMPLE_DIRS ${CA_EXAMPLE_DIRS}
    CACHE INTERNAL "List example source directories")
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: get_target_link_libraries

  The ``get_target_link_libraries()`` macro queries, optionally recursively,
  the given targets link libraries. This is useful for checking the library
  dependencies of a target.

  Arguments:
    * ``variable`` - The CMake variable name to store the result in.
    * ``target`` - The target to get the link libraries from.

  Keyword Arguments:
    * ``RECURSIVE`` - Option to enable recursively getting the target link
      libraries.
#]=======================================================================]
macro(get_target_link_libraries variable target)
  cmake_parse_arguments(args "RECURSIVE" "" "" ${ARGN})

  macro(get_target_link_libraries_recursive subtarget)
    if(TARGET ${subtarget})
      # Check if the subtarget is imported.
      get_target_property(imported ${subtarget} IMPORTED)
      # cannot access LINK_LIBRARIES on INTERFACE_LIBRARY target, as old
      # cmake versions consider it an error
      get_target_property(subtarget_type ${subtarget} TYPE)
      if(imported OR (subtarget_type STREQUAL "INTERFACE_LIBRARY"))
        # If subtarget is imported only get the interface link libraries.
        get_target_property(libraries ${subtarget} INTERFACE_LINK_LIBRARIES)
      else()
        # Otherwise get all the link libraries.
        get_target_property(libraries ${subtarget} LINK_LIBRARIES)
      endif()
      if(NOT libraries MATCHES NOTFOUND)
        foreach(library ${libraries})
          # Ensure only get the link libraries of a target once.
          if(${library} IN_LIST ${variable})
            continue()
          endif()
          # Append to the list of all link libraries for the root target.
          list(APPEND ${variable} ${library})
          # Only recurse if the user requested it.
          if(args_RECURSIVE)
            get_target_link_libraries_recursive(${library})
          endif()
        endforeach()
      endif()
    endif()
  endmacro()

  set(${variable})
  get_target_link_libraries_recursive(${target})
endmacro()

# Add the check target to run all registered checks, see add_ca_check() below.
add_custom_target(check-ca COMMENT "ComputeAorta checks.")

if(CMAKE_CROSSCOMPILING AND NOT CMAKE_CROSSCOMPILING_EMULATOR)
  message(WARNING "ComputeAorta check targets disabled as "
    "CMAKE_CROSSCOMPILING_EMULATOR was not specified")
endif()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_check

  The ``add_ca_check()`` macro takes a list of arguments which form a command
  to run a check. A new target called ``check-${name}`` is created and a
  dependency for ``check-${name}`` is added to the check target. To run an
  individual check build the ``check-${name}`` target and to run all checks
  build the ``check`` target. All checks are executed with a working directory
  of ``${PROJECT_SOURCE_DIR}`` and a comment of the form "Running ${name}
  checks" is displayed by the build system during execution.

  Arguments:
    * ``name`` - Target name suffix for the check, this will create a target
      called ``check-${name}``.

  Keyword Arguments:
    * ``NOEMULATE`` - Flag to specify that the first argument of the
      ``COMMAND`` should not be emulated using
      :cmake-variable:`CMAKE_CROSSCOMPILING_EMULATOR`, this should be set if
      the executable driving the check is not cross-compiled.
    * ``NOGLOBAL`` - Flag to specify that ``check-${name}`` should not be added
      to the global check target.
    * ``GTEST`` - Flag to specify that this check uses GoogleTest and that
      :cmake-variable:`CA_GTEST_LAUNCHER` should be used, if set, to launch the
      check executable.
    * ``USES_TERMINAL`` - Flag to specify that the check will be given access
      to the terminal if possible.
    * ``COMMAND`` - Keyword after which one or more arguments should be
      specified to define the command the check target will execute.
    * ``CLEAN`` - Keyword after which one or more filenames should be listed
      as addition files to be clean up by the ``clean`` target.
    * ``DEPENDS`` - Keyword after which one or more target dependencies can be
      specified.
    * ``ENVIRONMENT`` - Keyword after which one or more environment variables
      can be specified, each must be of the form: "VAR=<value>"
#]=======================================================================]
function(add_ca_check name)
  cmake_parse_arguments(args
    "NOEMULATE;NOGLOBAL;GTEST;USES_TERMINAL" ""
    "CLEAN;COMMAND;DEPENDS;ENVIRONMENT" ${ARGN})
  # Get the target from the first item in the command list and, if it is a
  # target, replace it with the path to the target file.
  set(command ${args_COMMAND})
  list(GET command 0 target)
  if(TARGET ${target})
    list(REMOVE_AT command 0)
    list(INSERT command 0 $<TARGET_FILE:${target}>)
  elseif(NOT EXISTS ${target})
    message(FATAL_ERROR
      "First argument in COMMAND list must be a target or executable path")
  endif()
  if(NOT args_NOEMULATE AND CMAKE_CROSSCOMPILING AND
      CMAKE_CROSSCOMPILING_EMULATOR)
    # Prefix the emulator command to execute the cross-compiled executable.
    list(INSERT command 0 ${CMAKE_CROSSCOMPILING_EMULATOR})
  endif()
  if(args_GTEST AND CA_GTEST_LAUNCHER)
    list(INSERT command 0 ${CA_GTEST_LAUNCHER})
  endif()
  if(CA_USE_SANITIZER MATCHES "Address|Thread")
    if(CA_USE_SANITIZER STREQUAL Address)
      set(SanitizerOptions "ASAN_OPTIONS=")
    elseif(CA_USE_SANITIZER STREQUAL Thread)
      # Set ThreadSanitizer suppression file when enabled.
      set(SuppressionsFile
        "${PROJECT_SOURCE_DIR}/scripts/jenkins/tsan_suppressions.txt")
      set(SanitizerOptions
        "TSAN_OPTIONS=suppressions=${SuppressionsFile} ")
    endif()
    # Force sanitizer allocators to return null instead of aborting on failure.
    string(APPEND SanitizerOptions "allocator_may_return_null=1")
    # Append sanitizer options to the execution environment.
    list(APPEND args_ENVIRONMENT "${SanitizerOptions}")
  endif()
  # FileCheck is really unhelpful by default. This goes some way to
  # debuggability
  list(APPEND args_ENVIRONMENT FILECHECK_DUMP_INPUT_ON_FAILURE=1)
  # Prefix the CMake command to setup the desired environment.
  list(INSERT command 0 ${CMAKE_COMMAND} -E env ${args_ENVIRONMENT})
  if(args_CLEAN)
    # ADDITIONAL_MAKE_CLEAN_FILES only works for the Makefiles generator and is
    # deprecated in CMake 3.15, ADDITIONAL_CLEAN_FILES works for everything but
    # wasn't added until CMake 3.15.
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY
      ADDITIONAL_MAKE_CLEAN_FILES ${args_CLEAN} APPEND)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY
      ADDITIONAL_CLEAN_FILES ${args_CLEAN} APPEND)
  endif()
  # Add a custom target, which runs the test, to the test target.
  if(args_USES_TERMINAL)
    add_custom_target(check-${name}
      COMMAND ${command} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      USES_TERMINAL
      DEPENDS ${args_DEPENDS} COMMENT "Running ${name} checks")
  else()
    add_custom_target(check-${name}
      COMMAND ${command} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      DEPENDS ${args_DEPENDS} COMMENT "Running ${name} checks")
  endif()
  if(NOT args_NOGLOBAL)
    add_dependencies(check-ca check-${name})
  endif()
  if(CA_ENABLE_COVERAGE AND (CA_RUNTIME_COMPILER_ENABLED OR
      (NOT CA_RUNTIME_COMPILER_ENABLED AND ${name} STREQUAL "UnitCL")))
    # Only UnitCL suite for offline coverage - for now
    add_coverage_test_suite(${name}
      COMMAND ${args_COMMAND} ENVIRONMENT ${args_ENVIRONMENT})
  endif()
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_check_group

  The ``add_ca_check_group()`` function creates a named target for a group of
  targets and/or checks, this is useful to setup check dependencies and for
  having a single named check for a set of disparate test suites. As with
  :cmake:command:`add_ca_check` the name is used to generate a target called
  ``check-${name}``.

  Arguments:
    * ``name`` - Target name suffix for the check group, this will create a
      target called ``check-${name}``.

  Keyword Arguments:
    * ``DEPENDS`` - A list of targets this check group will depends on, any
      CMake target can be specified.

      .. note::
        The full target name including the ``check-`` prefix should be
        specified for dependent check targets.

  Here's an example:

  .. code:: CMake

    add_ca_check_group(foo DEPENDS bar check-foo)
#]=======================================================================]
function(add_ca_check_group name)
  cmake_parse_arguments(args "" "" "DEPENDS" ${ARGN})
  if(args_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "add_ca_check_group invalid arguments: ${args_UNPARSED_ARGUMENTS}")
  endif()
  # Add a custom target, which depends on all listed targets.
  add_custom_target(check-${name}
    DEPENDS ${args_DEPENDS}
    COMMENT "Running ${name} group checks")
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_library_import

  The ``add_ca_library_import()`` macro adds a target referring to an external
  library not produced as part of the ComputeAorta build.

  Arguments:
    * ``target`` - Target name of library.
    * ``type`` - Type of library to be created in
      :cmake-command:`add_library`, one of ``STATIC``, ``SHARED``, or
      ``MODULE``.
    * ``location`` - Location on disk to set for the `IMPORTED_LOCATION`_
      property.

.. _IMPORTED_LOCATION:
  https://cmake.org/cmake/help/latest/prop_tgt/IMPORTED_LOCATION.html
#]=======================================================================]
macro(add_ca_library_import target type location)
  if(NOT EXISTS ${location})
    message(FATAL_ERROR "${target} library path `${location}` does not exist")
  endif()
  add_library(${target} ${type} IMPORTED GLOBAL)
  set_target_properties(${target} PROPERTIES
    IMPORTED_LOCATION ${location})
endmacro()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_executable_import

  The ``add_ca_executable_import()`` macro adds a target referring to an
  external executable not produced as part of the ComputeAorta build.

  Arguments:
    * ``target`` - Target name of executable.
    * ``location`` - Location on disk to set for `IMPORTED_LOCATION`_ property.

.. _IMPORTED_LOCATION:
  https://cmake.org/cmake/help/latest/prop_tgt/IMPORTED_LOCATION.html
#]=======================================================================]
macro(add_ca_executable_import target location)
  if(NOT EXISTS ${location})
    message(FATAL_ERROR
      "${target} executable path `${location}` does not exist")
  endif()
  add_executable(${target} IMPORTED GLOBAL)
  set_target_properties(${target} PROPERTIES
    IMPORTED_LOCATION ${location})
endmacro()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_configure_file

  The ``add_ca_configure_file()`` function adds a custom command which calls
  CMake's builtin :cmake-command:`configure_file` command at build time, this
  is useful when the only method of getting a path to a build target is by
  using generator expressions.

  Arguments:
    * ``input`` - Input file configuration description.
    * ``output`` - Output file to be configured.

  Keyword Arguments:
    * ``DEFINED`` - Specify a list of definitions taking the form
      "VAR=${value}" these will then be passed to to CMake's script mode as
      ``-DVAR=${value}``.
    * ``DEPENDS`` - Specify a list of dependencies.

  Here's an example:

  .. code:: CMake

    add_ca_configure_file(path/to/input path/to/output
      DEFINED TARGET_EXECUTABLE=$<TARGET_FILE:target>
      DEPENDS target)
#]=======================================================================]
function(add_ca_configure_file input output)
  cmake_parse_arguments(args "" "" "DEFINED;DEPENDS" ${ARGN})
  get_filename_component(input_dir ${input} DIRECTORY)
  set(ConfigureFileScript
    ${ComputeAorta_SOURCE_DIR}/cmake/ConfigureFileScript.cmake)
  foreach(define ${args_DEFINED})
    list(APPEND defines -D${define})
  endforeach()
  file(RELATIVE_PATH relOut ${CMAKE_BINARY_DIR} ${output})
  add_custom_command(OUTPUT ${output}
    COMMAND ${CMAKE_COMMAND} -DINPUT=${input} -DOUTPUT=${output} ${defines}
    -P ${ConfigureFileScript}
    DEPENDS ${input} ${ConfigureFileScript} ${args_DEPENDS}
    WORKING_DIRECTORY ${input_dir}
    COMMENT "Configuring file ${relOut}")
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_copy_file

This function creates a custom command to copy an input file (usually in the
source directory) to the output file (usually in the binary directory). It's
effectively a simpler version of :cmake:command:`add_ca_configure_file`, as it
does not process the file in any way.

.. seealso::
  :cmake:command:`add_ca_configure_file`

Keyword Arguments:
  * ``INPUT`` - The input file
  * ``OUTPUT`` - The output file
#]=======================================================================]
function(add_ca_copy_file)
  cmake_parse_arguments(args "" "" "INPUT;OUTPUT" ${ARGN})

  # Check arguments
  list(LENGTH args_INPUT len)
  if(NOT len EQUAL 1)
    message(FATAL_ERROR "add_ca_copy_file requires exactly one input")
  endif()
  list(LENGTH args_OUTPUT len)
  if(NOT len EQUAL 1)
    message(FATAL_ERROR "add_ca_copy_file requires exactly one output")
  endif()
  if(args_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "add_ca_copy_file does not take positional arguments")
  endif()

  file(RELATIVE_PATH relOut ${CMAKE_BINARY_DIR} ${args_OUTPUT})
  add_custom_command(
    OUTPUT ${args_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E copy ${args_INPUT} ${args_OUTPUT}
    DEPENDS ${args_INPUT}
    COMMENT "Copying file ${relOut}")
endfunction()

if((EXISTS ${PROJECT_SOURCE_DIR}/source/cl AND
  (CA_ENABLE_API STREQUAL "" OR CA_ENABLE_API MATCHES cl)) OR CA_NATIVE_CPU)
  # Cleared to ensure reconfigures behave correctly.
  set(CA_CL_RUNTIME_EXTENSION_TAGS ""
    CACHE INTERNAL "List of runtime extension names.")

#[=======================================================================[.rst:
.. cmake:command:: add_ca_cl_runtime_extension

  The ``add_ca_cl_runtime_extension()`` function adds a set of runtime
  extensions to the OpenCL library build.

  Arguments:
    * ``tag`` - Unique name for this set of extensions, it is used for
      accessing the extension information later in the build, must be a valid
      CMake variable name.

  Keyword Arguments:
    * ``EXTENSIONS`` - List of OpenCL extension names to be added.
    * ``HEADER`` - Public OpenCL extension header to be installed.
    * ``INCLUDE_DIRS`` - List of include directories required to build the
      extensions.
    * ``SOURCES`` - List of source files required to build the extensions.

  Variables:
    .. cmake:variable:: CA_CL_RUNTIME_EXTENSION_TAGS

      ``${tag}`` is appended to the list, then is internally cached.

    .. cmake:variable::  ${tag}_RUNTIME_EXTENSIONS

      Internally cached list of ``${tag}`` extensions.

    .. cmake:variable::  ${tag}_RUNTIME_HEADER

      Internally cached extension header for ``${tag}`` extensions.

    .. cmake:variable::  ${tag}_RUNTIME_INCLUDE_DIRS

      Internally cached include directory for ``${tag}`` extensions.

    .. cmake:variable::  ${tag}_RUNTIME_SOURCES

      Internally cached List of source files for ``${tag}`` extensions.
#]=======================================================================]
  function(add_ca_cl_runtime_extension tag)
    cmake_parse_arguments(args "" ""
      "EXTENSIONS;HEADER;INCLUDE_DIRS;SOURCES" ${ARGN})
    if(args_UNPARSED_ARGUMENTS)
      message(FATAL_ERROR "invalid arguments: ${args_UNPARSED_ARGUMENTS}")
    endif()
    # Add the tag to the list of runtime extension tags.
    list(APPEND CA_CL_RUNTIME_EXTENSION_TAGS ${tag})
    set(CA_CL_RUNTIME_EXTENSION_TAGS ${CA_CL_RUNTIME_EXTENSION_TAGS}
      CACHE INTERNAL "List of runtime extension names.")
    # Cache the extension information for later use.
    set(${tag}_RUNTIME_EXTENSIONS ${args_EXTENSIONS}
      CACHE INTERNAL "List of ${tag} extensions.")
    set(${tag}_RUNTIME_HEADER ${args_HEADER}
      CACHE INTERNAL "Extension header for ${tag} extensions.")
    set(${tag}_RUNTIME_INCLUDE_DIRS ${args_INCLUDE_DIRS}
      CACHE INTERNAL "Include directory for ${tag} extensions.")
    set(${tag}_RUNTIME_SOURCES ${args_SOURCES}
      CACHE INTERNAL "List of source files for ${tag} extensions.")
  endfunction()

  # Cleared to ensure reconfigures behave correctly.
  set(CA_CL_COMPILER_EXTENSION_TAGS ""
    CACHE INTERNAL "List of compiler extension tags.")

#[=======================================================================[.rst:
.. cmake:command:: add_ca_cl_compiler_extension

  The ``add_ca_cl_compiler_extension()`` function adds a set of compiler
  extensions to the OpenCL library build.

  Arguments:
    * ``tag`` - Unique name for this set of extensions, it is used to make
      accessing the other information later in the build and must be a valid
      CMake variable name.

  Keyword Arguments:
    * ``EXTENSIONS`` - List of OpenCL extension names to be added.
    * ``HEADER`` - Public OpenCL extension header to be installed.
    * ``INCLUDE_DIRS`` - List of include directories required to build the
      extensions.
    * ``SOURCES`` - List of source files required to build the extensions.

  Variables:
    .. cmake:variable:: CA_CL_COMPILER_EXTENSION_TAGS

      ``${tag}`` is appended to the list, then internally cached.

    .. cmake:variable:: ${tag}_COMPILER_EXTENSIONS

      Internally cached list of ``${tag}`` compiler extensions.

    .. cmake:variable::  ${tag}_COMPILER_HEADER

      Internally cached extension header for ``${tag}`` extensions.

    .. cmake:variable::  ${tag}_COMPILER_INCLUDE_DIRS

      Internally cached include directory for ``${tag}`` extensions.

    .. cmake:variable::  ${tag}_COMPILER_SOURCES

      Internally cached list of source files for ``${tag}`` extensions.
#]=======================================================================]
  function(add_ca_cl_compiler_extension tag)
    cmake_parse_arguments(args "" ""
      "EXTENSIONS;HEADER;INCLUDE_DIRS;SOURCES" ${ARGN})
    if(args_UNPARSED_ARGUMENTS)
      message(FATAL_ERROR "invalid arguments: ${args_UNPARSED_ARGUMENTS}")
    endif()
    # Add the tag to the list of compiler extension tags.
    list(APPEND CA_CL_COMPILER_EXTENSION_TAGS ${tag})
    set(CA_CL_COMPILER_EXTENSION_TAGS ${CA_CL_COMPILER_EXTENSION_TAGS}
      CACHE INTERNAL "List of compiler extension names.")
    # Cache the compiler extension information for later use.
    set(${tag}_COMPILER_EXTENSIONS ${args_EXTENSIONS}
      CACHE INTERNAL "List of ${tag} compiler extensions.")
    set(${tag}_COMPILER_HEADER ${args_HEADER}
      CACHE INTERNAL "Extension header for ${tag} extensions.")
    set(${tag}_COMPILER_INCLUDE_DIRS ${args_INCLUDE_DIRS}
      CACHE INTERNAL "Include directory for ${tag} extensions.")
    set(${tag}_COMPILER_SOURCES ${args_SOURCES}
      CACHE INTERNAL "List of source files for ${tag} extensions.")
  endfunction()
endif()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_install_components

  The ``add_ca_install_components()`` function creates a custom install target
  that will install all the specified components in the specified install
  directory.

  Arguments:
    * ``ARGN`` - Target name for custom install target.

  Keyword Arguments:
    * ``INSTALL_DIR`` - Directory to set for
      :cmake-variable:`CMAKE_INSTALL_PREFIX`.
    * ``COMPONENTS`` - One of more components to set for
      ``CMAKE_INSTALL_COMPONENT``.
    * ``DEPENDS`` - One or more target dependencies.

  For example to install only the OpenCL library and ``clVectorAddition`` in a
  directory called ``pkg``:

  .. code:: CMake

    add_ca_install_components(install-cl-clVectorAdd
      INSTALL_DIR ${CMAKE_BINARY_DIR}/pkg
      COMPONENTS OCL OCLExamples
      DEPENDS CL clVectorAddition)
#]=======================================================================]
function(add_ca_install_components)
  cmake_parse_arguments(args "" "INSTALL_DIR" "COMPONENTS;DEPENDS" ${ARGN})
  foreach(component ${args_COMPONENTS})
    set(command_file "${args_UNPARSED_ARGUMENTS}_${component}")
    set_source_files_properties(${command_file} PROPERTIES SYMBOLIC ON)
    add_custom_command(OUTPUT ${command_file}
      COMMAND ${CMAKE_COMMAND}
        -DCMAKE_INSTALL_PREFIX=${args_INSTALL_DIR}
        -DCMAKE_INSTALL_COMPONENT=${component}
        -P ${CMAKE_BINARY_DIR}/cmake_install.cmake)
    list(APPEND commands ${command_file})
  endforeach()

  add_custom_target(${args_UNPARSED_ARGUMENTS}
    DEPENDS ${commands} ${args_DEPENDS})
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_force_header

  The ``add_ca_force_header()`` function adds a per-device force-header into
  the CA build.

  Keyword Arguments:
    * ``PREFIX`` - A unique prefix for the internally generated files and
      variables
    * ``DEVICE_NAME`` - The name of the core device.

      .. important::
        ``DEVICE_NAME`` must match ``device_name`` from ``add_core_target``
        in ``modules/core/source/CMakeLists.txt``.

      .. todo::
        Create generated documentation for ``add_core_target`` to reference
        in above admonishment. JIRA CA-2757.

    * ``PATH`` - The file path of the force-include header
#]=======================================================================]
function(add_ca_force_header)
  cmake_parse_arguments(args "" "PREFIX;DEVICE_NAME;PATH" "" ${ARGN})

  # TODO: Replace prefixes with filenames
  if("${args_PREFIX}" STREQUAL "builtins")
    message(FATAL_ERROR
      "add_ca_force_header PREFIX \"builtins\" is reserved\n")
  endif()

  list(APPEND CA_FORCE_HEADERS_PREFIXES ${args_PREFIX})
  list(REMOVE_DUPLICATES CA_FORCE_HEADERS_PREFIXES)
  list(APPEND CA_FORCE_HEADERS_DEVICE_NAMES ${args_DEVICE_NAME})
  list(REMOVE_DUPLICATES CA_FORCE_HEADERS_DEVICE_NAMES)
  list(APPEND CA_FORCE_HEADERS_PATHS ${args_PATH})
  list(REMOVE_DUPLICATES CA_FORCE_HEADERS_PATHS)

  list(LENGTH CA_FORCE_HEADERS_PREFIXES num_headers)
  list(LENGTH CA_FORCE_HEADERS_DEVICE_NAMES num_names)
  list(LENGTH CA_FORCE_HEADERS_PATHS num_paths)
  if((NOT ${num_headers} EQUAL ${num_names}) OR
      (NOT ${num_headers} EQUAL ${num_paths}))
    message(FATAL_ERROR
      "CA_FORCE_HEADERS_PREFIXES, CA_FORCE_HEADERS_DEVICE_NAMES, and\n"
      "CA_FORCE_HEADERS_PATHS must have the same lengths.\n"
      "add_ca_force_header() may have been called more than once for the\n"
      "same core device, which is unsupported. Changes to the parameters to\n"
      "add_ca_force_header() requires a clean rebuild.\n")
  endif()

  set(CA_FORCE_HEADERS_PREFIXES ${CA_FORCE_HEADERS_PREFIXES} CACHE INTERNAL
    "List of force-header name prefixes")
  set(CA_FORCE_HEADERS_DEVICE_NAMES ${CA_FORCE_HEADERS_DEVICE_NAMES} CACHE
    INTERNAL "List of core device names that have force-headers")
  set(CA_FORCE_HEADERS_PATHS ${CA_FORCE_HEADERS_PATHS} CACHE INTERNAL
    "List of paths to force-header files")
endfunction()
# Reset the lists when reconfiguring to avoid errors.
set(CA_FORCE_HEADERS_PREFIXES "" CACHE INTERNAL
  "List of force-header name prefixes")
set(CA_FORCE_HEADERS_DEVICE_NAMES "" CACHE
  INTERNAL "List of core device names that have force-headers")
set(CA_FORCE_HEADERS_PATHS "" CACHE INTERNAL
  "List of paths to force-header files")

#[=======================================================================[.rst:
.. cmake:command:: add_ca_configure_lit_site_cfg

  This function provides an automatic way to 'configure'-like generate a file
  based on a set of common and custom variables, specifically targeting the
  variables needed for the 'lit.site.cfg' files. This function bundles the
  common variables that any Lit instance is likely to need, and custom
  variables can be passed in.

  On success, a custom target called ``name``-lit will be created. This
  function may fail if certain required key LLVM tool components are not found,
  in which case the custom target will not have been created.

  .. note::

    Copied and stripped down from LLVM's ``configure_lit_site_cfg``, found in
    AddLLVM.cmake.

  Arguments:
    * ``name`` - The name of the test suite.
    * ``site_in`` - The input path to the lit.site.cfg.in-like file
    * ``site_out`` - The output path to the generated lit.site.cfg file

  Keyword Arguments:
    * ``MAIN_CONFIG`` - Path to the main lit.cfg to load. Can be empty, in
      which case a lit.cfg sourced from the same directory as ``site_in`` is
      used.
    * ``DEFINED`` - Extra defines, passed to ``add_ca_configure_file``.
    * ``PATHS`` - The keyword PATHS is followed by a list of cmake variable
      names that are mentioned as `path("@varname@")` in the lit.cfg.py.in
      file. Variables in that list are treated as paths that are relative to
      the directory the generated lit.cfg.py file is in, and the `path()`
      function converts the relative path back to absolute form. This makes it
      possible to move a build directory containing lit.cfg.py files from one
      machine to another.
#]=======================================================================]
function(add_ca_configure_lit_site_cfg name site_in site_out)
  cmake_parse_arguments(ARG "" "" "MAIN_CONFIG;DEFINED;PATHS" ${ARGN})

  if("${ARG_MAIN_CONFIG}" STREQUAL "")
    get_filename_component(INPUT_DIR ${site_in} DIRECTORY)
    set(ARG_MAIN_CONFIG "${INPUT_DIR}/lit.cfg")
  endif()

  string(TOUPPER ${CMAKE_BUILD_TYPE} UPPER_BUILD_TYPE)

  string(CONCAT AUTOGENERATED_MESSAGE
        "This file was autogenerated by cmake. To make changes, edit ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in")

  add_ca_configure_file(${site_in} ${site_out}
    DEFINED
    AUTOGENERATED_MESSAGE=${AUTOGENERATED_MESSAGE}
    CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
    CMAKE_CROSSCOMPILING=${CMAKE_CROSSCOMPILING}
    CMAKE_SIZEOF_VOID_P=${CMAKE_SIZEOF_VOID_P}
    CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
    PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
    # cmake doesn't correctly expand CMAKE_CURRENT_BINARY_DIR in -P mode - even
    # when passed on the command line. We have to call it something else
    # because it's special
    CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
    CA_LLVM_TOOLS_DIR=${CA_LLVM_INSTALL_DIR}/bin
    CA_BUILTINS_TOOLS_DIR=${CA_BUILTINS_TOOLS_DIR}
    CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS_${UPPER_BUILD_TYPE}}
    LLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR}
    LLVM_VERSION_MINOR=${LLVM_VERSION_MINOR}
    LLVM_VERSION_PATCH=${LLVM_VERSION_PATCH}
    LLVM_ENABLE_ASSERTIONS=${LLVM_ENABLE_ASSERTIONS}
    LLVM_FORCE_ENABLE_STATS=${LLVM_FORCE_ENABLE_STATS}
    LLVM_TARGETS_TO_BUILD=${TARGETS_TO_BUILD}
    CA_ENABLE_DEBUG_SUPPORT=${CA_ENABLE_DEBUG_SUPPORT}
    CA_BUILD_32_BITS=${CA_BUILD_32_BITS}
    CA_ENABLE_LLVM_OPTIONS_IN_RELEASE=${CA_ENABLE_LLVM_OPTIONS_IN_RELEASE}
    CA_COMMON_LIT_SOURCE_PATH=${PROJECT_SOURCE_DIR}/modules/lit
    CA_COMMON_LIT_BINARY_PATH=${PROJECT_BINARY_DIR}/modules/lit
    ${ARG_DEFINED})

  if (EXISTS "${ARG_MAIN_CONFIG}")
    # Remember main config / generated site config for ca-lit.in.
    get_property(CA_LIT_CONFIG_FILES GLOBAL PROPERTY CA_LIT_CONFIG_FILES)
    list(APPEND CA_LIT_CONFIG_FILES "${ARG_MAIN_CONFIG}" "${site_out}")
    set_property(GLOBAL PROPERTY CA_LIT_CONFIG_FILES ${CA_LIT_CONFIG_FILES})
  endif()

  add_custom_target(${name}-lit DEPENDS ${site_out} ca-common-lit)
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_lit_check

  The ``add_ca_lit_check(target comment)`` raw function provides an automatic
  way to set up a check target to run a suite of LIT tests. Note that users are
  advised to use ``add_ca_lit_testsuite`` instead.

  Arguments:
    * ``target`` - Named target to set output directory properties on. A target
      ``check-${target}-lit`` will be created which runs LIT tests producing XML
      results in ``${target}-lit.xml``.
    * ``comment`` - A comment to display to the terminal when running the check
      target.

  Keyword Arguments:
    * ``NOGLOBAL`` - Flag to specify that ``check-${target}`` should not be
      added to the global check target.
    * ``PARAMS`` - Keyword after which one or more additional parameters to the
      llvm-lit command can be specified. Each parameter is automatically
      prepended with --param.
    * ``DEPENDS`` - Keyword after which one or more target dependencies can be
      specified.
    * ``ARGS`` - Keyword after which one or more arguments to the llvm-lit
      command can be specified.
#]=======================================================================]
function(add_ca_lit_check target comment)
  cmake_parse_arguments(args "NOGLOBAL" "" "PARAMS;DEPENDS;ARGS" ${ARGN})
  set(CA_LIT_PATH "${PROJECT_BINARY_DIR}/modules/lit/ca-lit")

  find_package(Lit)
  if(NOT Lit_FOUND)
    message(WARNING "${target}-lit tests will not be runnable: lit not found "
      "Please install https://pypi.python.org/pypi/lit")
    return()
  endif()

  # The target name has an extra suffix
  string(APPEND target "-lit")

  # We could probably offer more flexibility here
  set(LIT_ARGS "${args_ARGS} -sv --param skip_if_missing_tools=1 --xunit-xml-output=${PROJECT_BINARY_DIR}/${target}.xml")

  # Propagate on NOGLOBAL
  if ("${args_NOGLOBAL}")
    list(APPEND EXTRA_ARGN NOGLOBAL)
  endif()

  separate_arguments(LIT_ARGS)

  if(CMAKE_CROSSCOMPILING AND CMAKE_CROSSCOMPILING_EMULATOR)
    list(APPEND EXTRA_ARGN "NOEMULATE")
    list(APPEND LIT_ARGS "--param" emulator="${CMAKE_CROSSCOMPILING_EMULATOR}")
  endif()

  set(LIT_COMMAND "${PYTHON_EXECUTABLE};${CA_LIT_PATH}")
  list(APPEND LIT_COMMAND ${LIT_ARGS})
  foreach(param ${args_PARAMS})
    list(APPEND LIT_COMMAND --param ${param})
  endforeach()

  if(args_UNPARSED_ARGUMENTS)
    add_ca_check(${target}
      COMMAND ${LIT_COMMAND} ${args_UNPARSED_ARGUMENTS}
      CLEAN ${PROJECT_BINARY_DIR}/${target}.xml
      USES_TERMINAL
      COMMENT "${comment}"
      DEPENDS ${args_DEPENDS} ${EXTRA_ARGN})
  else()
    add_ca_check(${target} NOEMULATE
      COMMAND ${CMAKE_COMMAND} -E echo "${target} does nothing, no tools built.")
    message(STATUS "${target} does nothing.")
  endif()

endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_lit_testsuite

  The ``add_ca_lit_testsuite(name)`` function creates a new lit test suite. A
  new target called ``check-${name}-lit`` is created and a dependency for
  ``check-${name}-lit`` is added to the global check target. To run an
  individual check build the ``check-${name}-lit`` target and to run all checks
  build the ``check`` target. All checks are executed with a working directory
  of ``${PROJECT_SOURCE_DIR}`` and a comment of the form "Running ${name}
  checks" is displayed by the build system during execution.

  If ``EXCLUDE_FROM_UMBRELLAS`` is not set, the test suite will also be added
  to all open umbrella targets (see ``ca_umbrella_lit_testsuite_open()`` and
  ``ca_umbrella_lit_testsuite_close()``).

  Arguments:
    * ``name`` - Target name suffix for the check, this will create a target
      called ``check-${name}-lit``.

  Keyword Arguments:
    * ``NOGLOBAL`` - Flag to specify that ``check-${target}-lit`` should not be
      added to the global check target.
    * ``EXCLUDE_FROM_UMBRELLAS`` - Flag to specify that ``${name}``
      should not be added to any currently open test-suite umbrellas.
    * ``TARGET`` - Keyword after which a target name can be specified. If
      set, the test suite will be appended to a global set of test suites
      relating to that target. A global ``check-${target}-lit`` target will be
      created, comprised of all test suites relating to ``target``. Has no
      effect if ``EXCLUDE_FROM_UMBRELLAS`` is set.
    * ``PARAMS`` - Keyword after which one or more additional parameters to the
      llvm-lit command can be specified. Each parameter is automatically
      prepended with --param.
    * ``DEPENDS`` - Keyword after which one or more target dependencies can be
      specified.
    * ``ARGS`` - Keyword after which one or more arguments to the llvm-lit
      command can be specified.

#]=======================================================================]
function(add_ca_lit_testsuite name)
  # Note that NOGLOBAL is handled through UNPARSED_ARGUMENTS
  cmake_parse_arguments(args "EXCLUDE_FROM_UMBRELLAS" "TARGET"
                             "PARAMS;DEPENDS;ARGS" ${ARGN})

  # EXCLUDE_FROM_UMBRELLAS excludes the test ${target} out of all umbrella
  # suites.
  if(NOT args_EXCLUDE_FROM_UMBRELLAS)
    get_property(umbrellas GLOBAL PROPERTY CA_LIT_UMBRELLAS)
    list(REMOVE_DUPLICATES umbrellas)

    # If the testsuite is specifically a named target suite, log that one too.
    if(args_TARGET)
      string(TOUPPER ${args_TARGET} target)
      list(APPEND umbrellas ${target})
      # Keep track of this in the global list of open target suites
      set_property(GLOBAL APPEND PROPERTY CA_TARGET_LIT_TESTSUITES ${target})
    endif()

    foreach(name ${umbrellas})
      # Register the testsuites, params and depends for the umbrella check rule.
      set_property(GLOBAL APPEND PROPERTY CA_${name}_LIT_TESTSUITES ${args_UNPARSED_ARGUMENTS})
      set_property(GLOBAL APPEND PROPERTY CA_${name}_LIT_PARAMS ${args_PARAMS})
      set_property(GLOBAL APPEND PROPERTY CA_${name}_LIT_DEPENDS ${args_DEPENDS})
      set_property(GLOBAL APPEND PROPERTY CA_${name}_LIT_EXTRA_ARGS ${args_ARGS})
    endforeach()
  endif()

  string(TOLOWER ${name} name)
  add_ca_lit_check(${name}
    "Running ${name} checks"
    ${args_UNPARSED_ARGUMENTS}
    PARAMS ${args_PARAMS}
    DEPENDS ${args_DEPENDS}
    ARGS ${args_ARGS})

endfunction()

#[=======================================================================[.rst:
.. cmake:command:: ca_umbrella_lit_testsuite_open

  The ``ca_umbrella_lit_testsuite_open(target)`` function opens a new umbrella
  lit test suite.

  All subsequent calls to ``add_ca_lit_testsuite`` (which don't pass
  ``EXCLUDE_FROM_UMBRELLAS``) are implicitly added to this umbrella suite. The
  umbrella suite is open until a corresponding call to
  ``ca_umbrella_lit_testsuite_close`` is made with the same ``target``.

  For example, given:

  ``ca_umbrella_lit_testsuite_open(all)``

  ``*  add_ca_lit_testsuite(foo)``

  ``*  ca_umbrella_lit_testsuite_open(compiler)``

  ``*  - add_ca_lit_testsuite(bar)``

  ``*  - add_ca_lit_testsuite(baz)``

  ``*  - add_ca_lit_testsuite(special EXCLUDE_FROM_UMBRELLAS)``

  ``*  ca_umbrella_lit_testsuite_close(compiler)``

  ``ca_umbrella_lit_testsuite_close(all)``

  Produces the following check targets, from most outermost to innermost:

  ``check-all-lit:      foo, bar, baz``

  ``check-compiler-lit: bar, baz``

  ``check-foo-lit:      foo``

  ``check-bar-lit:      bar``

  ``check-baz-lit:      baz``

  ``check-special-lit:  special``

#]=======================================================================]

function(ca_umbrella_lit_testsuite_open target)
  string(TOUPPER "${target}" target)
  set_property(GLOBAL APPEND PROPERTY CA_LIT_UMBRELLAS ${target})
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: ca_umbrella_lit_testsuite_close

  The ``ca_umbrella_lit_testsuite_close(target)`` function closes an open umbrella
  lit test suite.

  A new check target with the name ``check-${target}-lit`` will be created
  using test suites previously registered with ``add_ca_lit_testsuite`` while
  the umbrella was open.

  See ``ca_umbrella_lit_testsuite_open`` for more details.

#]=======================================================================]

function(ca_umbrella_lit_testsuite_close target)
  string(TOUPPER "${target}" target)

  get_property(testsuites GLOBAL PROPERTY CA_${target}_LIT_TESTSUITES)
  get_property(params GLOBAL PROPERTY CA_${target}_LIT_PARAMS)
  get_property(depends GLOBAL PROPERTY CA_${target}_LIT_DEPENDS)
  get_property(extra_args GLOBAL PROPERTY CA_${target}_LIT_EXTRA_ARGS)

  # Remove the target from the open list of umbrellas
  get_property(umbrellas GLOBAL PROPERTY CA_LIT_UMBRELLAS)
  list(REMOVE_ITEM umbrellas "${target}")
  set_property(GLOBAL PROPERTY CA_LIT_UMBRELLAS ${umbrellas})

  string(TOLOWER "${target}" target)
  add_ca_lit_check(${target}
    "Running ${target} regression tests"
    ${testsuites}
    NOGLOBAL
    PARAMS ${params}
    DEPENDS ${depends}
    ARGS ${extra_args}
    )
endfunction()
