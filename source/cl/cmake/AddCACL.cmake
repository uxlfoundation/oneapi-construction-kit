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
OpenCL specific CMake utilities are provided by the
``source/cl/cmake/AddCACL.cmake`` module, exposing utilities in a similar way
to the :doc:`/cmake/AddCA`, and in some cases working on top of it. For
example, the :cmake:command:`add_ca_cl_executable` function calls
:cmake:command:`add_ca_executable` before adding OpenCL specific compiler
definitions to the target.

If we're testing through the ICD loader then environment variables that may be
needed for testing are also added to the target properties here.
:cmake:command:`add_ca_cl_check` sets the :envvar:`OCL_ICD_FILENAMES`
environment var to the name of our :ref:`OpenCL library <cmake:Shared Library
Naming and Versioning>`, and the :envvar:`OCL_ICD_VENDORS` environment
variable to set to some arbitrary path to prevent system installed drivers from
being found.

To use this module with :cmake:command:`add_ca_example_subdirectory`:

.. code:: cmake

  include(${ComputeAorta_SOURCE_DIR}/source/cl/cmake/AddCACL.cmake)
#]=======================================================================]

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_COMPILE_DEFINITIONS

  A CMake variable containing list of compile definitions which provides a
  single place for setting OpenCL specific pre-processor flags such as
  ``CA_TARGET_OPENCL_VERSION``.
#]=======================================================================]
# ComputeAorta uses the unified headers, inform the headers of the target
# version, this only adds the macro definition to
# :cmake:command: `add_ca_library` and :cmake:command: `add_ca_executable`
# calls starting here in the CMake tree.
set(CA_CL_COMPILE_DEFINITIONS
  CL_TARGET_OPENCL_VERSION=${CA_CL_STANDARD_INTERNAL})

#[=======================================================================[.rst:
.. cmake:command:: target_link_ca_cl

  A CMake command which specifies the required link libraries for building
  libraries or executables which require OpenCL entry points. Either ``CL`` or
  ``OpenCL`` is linked automatically baased on the
  :cmake:variable:`CL_CA_ENABLE_ICD_LOADER` option.

  Arguments:
    * ``target`` Library or executable target to link OpenCL into.
#]=======================================================================]
function(target_link_ca_cl target)
  if(CA_CL_TEST_STATIC_LIB)
    target_link_libraries(${target} PRIVATE CL-static)
    add_dependencies(${target} CL-static)
  elseif(CA_CL_ENABLE_ICD_LOADER)
    target_link_libraries(${target} PRIVATE OpenCL)
    add_dependencies(${target} CL)
  else()
    target_link_libraries(${target} PRIVATE CL)
  endif()
  target_include_directories(${target}
    PRIVATE ${CA_CL_EXTENSION_INCLUDE_DIRS}
    SYSTEM PRIVATE ${CL_INCLUDE_DIR})
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_cl_executable

  A CMake command which acts acts exactly like
  :cmake:command:`add_ca_executable` except it selectively links against one of
  the ``CL`` or ``OpenCL`` targets depending on the value of the
  :cmake:variable:`CA_CL_ENABLE_ICD_LOADER` option.

  Arguments:
    * ``target`` Name of the executable target to add.
#]=======================================================================]
function(add_ca_cl_executable target)
  add_ca_executable(${target} ${ARGN})
  if(CA_CL_TEST_STATIC_LIB)
    target_resources(${target} NAMESPACES ${BUILTINS_NAMESPACES})
  endif()
  target_compile_options(${target} PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-deprecated-declarations>)
  target_compile_definitions(${target} PRIVATE ${CA_CL_COMPILE_DEFINITIONS})
  target_link_ca_cl(${target})
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_cl_library

  A CMake command which acts exactly like :cmake:command:`add_ca_library`
  except it selectively links against one of the ``CL`` or ``OpenCL`` targets
  depending on the value of :cmake:variable:`CA_CL_ENABLE_ICD_LOADER` option.

  Arguments:
    * ``target`` Name of the library target to add.
#]=======================================================================]
function(add_ca_cl_library target)
  add_ca_library(${target} ${ARGN})
  target_compile_definitions(${target} PUBLIC ${CA_CL_COMPILE_DEFINITIONS})
  target_link_ca_cl(${target})
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_cl_icd_file

  A CMake command which generates two ``.icd`` files,
  the first is used during development and points to an OpenCL library which
  exists in the build directory, and the second is installed and points to the
  installed path of the OpenCL library.

  Arguments:
    * ``target`` The CMake target of the OpenCL library.

  Keyword Arguments:
    * ``NAME`` The name, without the .icd extension, for the output file.
      Defaults to ``${target}``.
#]=======================================================================]
function(add_ca_cl_icd_file target)
  cmake_parse_arguments(args "" "NAME" "" ${ARGN})
  if(NOT args_NAME)
    set(args_NAME ${target})
  endif()
  set(outputDir ${PROJECT_BINARY_DIR}/share/OpenCL/vendors)
  if(NOT IS_DIRECTORY ${outputDir})
    file(MAKE_DIRECTORY ${outputDir})
  endif()
  add_ca_configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/source/posix.icd.in
    ${outputDir}/${args_NAME}.icd
    DEFINED CA_OPENCL_LIBRARY_PATH=$<TARGET_FILE:${target}>)
  add_custom_target(${target}-icd-file ALL
    DEPENDS ${outputDir}/${args_NAME}.icd)
  install(FILES DESTINATION share/OpenCL/vendors COMPONENT CLIcd)
endfunction()

get_ock_check_name(check_cl_name cl)
if(NOT TARGET ${check_cl_name})
  add_ca_check_group(cl NOGLOBAL)
endif()

#[=======================================================================[.rst:
.. cmake:command:: add_ca_cl_check

  A CMake command which acts exactly like :cmake:command:`add_ca_check` except
  it also sets environment variables to specify the use of the ``CL`` target
  library. If :cmake:variable:`CA_CL_ENABLE_ICD_LOADER` is ``ON`` the
  :envvar:`OCL_ICD_FILENAMES` environment variable is set when executing the
  check, and the :envvar:`OCL_ICD_VENDORS` environment variable to set to some
  arbitrary path (that does not need to exist) to prevent system installed
  drivers from being found. If :cmake:variable:`CA_CL_ENABLE_INTERCEPT_LAYER`
  is ``ON`` the :envvar:`CLI_OpenCLFileName` environment variable is set.

    .. note::
    
      If  :cmake:variable:`CA_ENABLE_TESTS` is set to ``OFF`` this function does
      nothing.

  Arguments:
    * ``name`` A unique name for the check, should include the name of the
      target being checked.

  Keyword Arguments:
    * ``NOGLOBALCL`` - Flag to specify that the new target should not be
      added to the CL-wide check target.
#]=======================================================================]
function(add_ca_cl_check name)
  if (NOT CA_ENABLE_TESTS)
    return()
  endif()
  get_ock_check_name(check_name ${name})
  cmake_parse_arguments(args "NOGLOBALCL" "" "ENVIRONMENT" ${ARGN})
  set(environment ${args_ENVIRONMENT})
  if(CA_CL_ENABLE_ICD_LOADER)
    # Disable detection of system ICD vendors and specify the path to the CL
    # target library for the OpenCL-ICD-Loader to load for testing.
    list(INSERT environment 0
      "OCL_ICD_VENDORS=/dev/null"
      "OCL_ICD_FILENAMES=$<TARGET_FILE:CL>")

    if(CA_CL_ENABLE_INTERCEPT_LAYER)
      # The OpenCL-ICD-Loader is built as an ExternalProject when the
      # OpenCL-Intercept-Layer is enabled as both create an OpenCL target, so
      # we need to tell the OpenCL-Intercept-Layer the OS dependent shared
      # library filename of the OpenCL-ICD-Loader.
      set(icdLoaderInstallDir
        ${PROJECT_BINARY_DIR}/source/cl/external/OpenCL-ICD-Loader/install)
      if(CMAKE_SYSTEM_NAME STREQUAL Linux)
        set(openCLFileName ${icdLoaderInstallDir}/lib/libOpenCL.so)
      elseif(CMAKE_SYSTEM_NAME STREQUAL Windows)
        set(openCLFileName ${icdLoaderInstallDir}/bin/OpenCL.dll)
      endif()
      # Specify the path to the OpenCL-ICD-Loader and the directory to dump
      # program binaries.
      set(dumpDirectory "${PROJECT_BINARY_DIR}/CLInterceptDump/${name}")
      list(INSERT environment 0
        "CLI_DumpDir=${dumpDirectory}"
        "CLI_OpenCLFileName=${openCLFileName}")

      # Add the dump target, this is stage 1 where the binaries are written to
      # the filesystem for later injection.
      set(dumpEnvironment ${environment})
      list(INSERT dumpEnvironment 0
        "CLI_OmitProgramNumber=1"
        "CLI_DumpProgramBinaries=1")
      add_ca_check(${name}-dump NOGLOBAL
        ${args_UNPARSED_ARGUMENTS} ENVIRONMENT ${dumpEnvironment})

      # Add the prepare target, this is stage 2 where the binaries are copies
      # to the Inject subdirectory of DumpDir and renamed.
      set(injectPrepareBins
        ${PROJECT_SOURCE_DIR}/scripts/testing/inject-prepare-bins.py)
      add_custom_target(${check_name}-prepare
        COMMAND ${PYTHON_EXECUTABLE}
          ${injectPrepareBins} --clean ${dumpDirectory}
        DEPENDS ${check_name}-dump
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Running ${name}-prepare checks")

      # Enable injecting program binaries for the check target. This is stage 3
      # which uses the same target as the non OpenCL-Intercept-Layer path.
      list(INSERT environment 0 "CLI_InjectProgramBinaries=1")
    endif()
  endif()
  add_ca_check(${name}
    ${args_UNPARSED_ARGUMENTS} ENVIRONMENT ${environment})
  if(NOT ${args_NOGLOBALCL})
    add_dependencies(${check_cl_name} ${check_name})
  endif()
  if(CA_CL_ENABLE_INTERCEPT_LAYER)
    add_dependencies(${check_name} ${check_name}-prepare)
  endif()
endfunction()
