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
.. cmake:variable:: MUX_TARGET_LIBRARIES

  Variable containing a list of ComputeMux target libraries
#]=======================================================================]
# Clear the list of target libraries before inclusion, enables reconfiguring a
# build directory with a different set of targets.
set(MUX_TARGET_LIBRARIES "" CACHE INTERNAL
  "List of ComputeMux target libraries." FORCE)

#[=======================================================================[.rst:
.. cmake:variable:: CA_HOST_CL_DEVICE_NAME_PREFIX

  ComputeAorta host CPU device names are always prefixed.
  The prefix together with the device type enables detection if a
  device is a CPU host device.

  This variable is exported to
  ``${CMAKE_BINARY_DIR}/source/cl/include/cl/config.h``
  by ``source/cl/CMakeLists.txt``. It has to be defined here to be
  available when Mux target ``CMakeLists.txt`` files are processed.
#]=======================================================================]
set(CA_HOST_CL_DEVICE_NAME_PREFIX "ComputeAorta" CACHE INTERNAL
  "ComputeAorta internal prefix used for host CPU device names" FORCE)

#[=======================================================================[.rst:
.. cmake:command:: get_ca_host_arch

  A CMake function that sets the given variable in the parent scope to the
  name of the host architecture.

  Arguments:
    * ``target`` Library or executable target to link OpenCL into.

  The following arguments are required:
    * ``parent_variable_name`` Name of the variable in the function's parent
      scope to set to the architecture name.

  The following keyword arguments are available:
    * ``ARCH <architecture>`` - (optional) Use the provided architecture
      instead of the native architecture CMake targets. The string is
      case-independent. "host" is supported in addition to the handled
      architectures listed below.

      Handled architectures:
        * Arm
        * AArch64
        * x86
        * x86_64
        * riscv64
#]=======================================================================]
function(get_ca_host_arch parent_variable_name)
  cmake_parse_arguments(args "" "ARCH" "" ${ARGN})
  if(args_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "get_ca_host_arch invalid arguments: ${args_UNPARSED_ARGUMENTS}")
  endif()

  # If function called with ARCH that isn't the native arch, then use it instead
  # of the CMake system architecture.
  # If ARCH is `host` then use the native architecture CMake targets.
  if(args_ARCH AND NOT args_ARCH STREQUAL "host")
    string(TOUPPER "${args_ARCH}" ARCH)
  else()
    string(TOUPPER "${CMAKE_SYSTEM_PROCESSOR}" ARCH)
  endif()

  if(ARCH STREQUAL "ARM")
    set(${parent_variable_name} "Arm" PARENT_SCOPE)
  elseif(ARCH STREQUAL "ARM64" OR ARCH STREQUAL "AARCH64")
    set(${parent_variable_name} "AArch64" PARENT_SCOPE)
  elseif(ARCH STREQUAL "X86")
    set(${parent_variable_name} "x86" PARENT_SCOPE)
  elseif(ARCH STREQUAL "X86_64" OR ARCH STREQUAL "AMD64")
    if(CA_BUILD_32_BITS OR ${CMAKE_SIZEOF_VOID_P} EQUAL 4)
      # We are building on an x86_64 platform, but might have forced 32-bit or
      # be using an x86 compiler.
      set(${parent_variable_name} "x86" PARENT_SCOPE)
    else()
      set(${parent_variable_name} "x86_64" PARENT_SCOPE)
    endif()
  elseif(ARCH STREQUAL "RISCV32")
    set(${parent_variable_name} "riscv32" PARENT_SCOPE)
  elseif(ARCH STREQUAL "RISCV64")
    set(${parent_variable_name} "riscv64" PARENT_SCOPE)
  else()
    message(FATAL_ERROR
      "Unhandled architecture value: ${ARCH}")
  endif()
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: get_ca_host_device_name

  A CMake function that sets the given variable in the parent scope to the
  name of the host device.

  The value stored in the parent scope variable can be passed to
  :cmake:command:`add_mux_target` as their ``DEVICE_NAMES`` parameter.

  .. important::
    This function **MUST** only be used for host CPU devices.

  The following arguments are required:
    * ``parent_variable_name`` - name of the variable in the function's parent
      scope to set to the architecture name.

  The following keyword arguments are available:
    * ``ARCH <architecture>`` - (optional) use the provided architecture instead
      of the native architecture CMake targets. The string is case-independent.
      "host" is supported in addition to the handled architectures listed below
      and results in a name that ``clc`` can disambiguate from the normal host
      architecture device name.

      Handled architectures:
        * Arm
        * AArch64
        * x86
        * x86_64
#]=======================================================================]
function(get_ca_host_device_name parent_variable_name)
  cmake_parse_arguments(args "" "ARCH" "" ${ARGN})
  if(args_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "get_ca_host_device_name invalid arguments: ${args_UNPARSED_ARGUMENTS}")
  endif()

  set(ARCH_NAME)
  if(args_ARCH)
    get_ca_host_arch(ARCH_NAME ARCH "${args_ARCH}")
  else()
    get_ca_host_arch(ARCH_NAME)
  endif()

  # If ARCH is `host` then create a special device name that will not contain
  # the normal host architecture device name to not have substring matching
  # in `clc`.
  if(args_ARCH AND args_ARCH STREQUAL "host")
    set(HOST_CL_DEVICE_NAME
      "${CA_HOST_CL_DEVICE_NAME_PREFIX} Host Compiler ${ARCH_NAME}")
  else()
    set(HOST_CL_DEVICE_NAME "${CA_HOST_CL_DEVICE_NAME_PREFIX} ${ARCH_NAME}")
  endif()

  # Cross-builds for/from Windows are not usable on Linux. Differentiate the
  # device names to express this difference.
  if(CA_PLATFORM_WINDOWS)
    set(HOST_CL_DEVICE_NAME "${HOST_CL_DEVICE_NAME} Windows")
  endif()

  set(${parent_variable_name} "${HOST_CL_DEVICE_NAME}" PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_mux_target

  The ``add_mux_target`` function makes mux aware of the existence of a mux
  target, ensuring that mux has the required information to properly integrate
  the target into the build. It also creates a custom target to generate an API
  header file for a mux implementation.

  The following arguments are required:
    * ``target`` - the mux implementation CMake target.

  The following keyword arguments are available:

  * ``HEADER_DIR <path>`` - (required) the output directory for the generated
    header file.
  * ``CAPABILITIES`` - (required) the target's supported capabilities.
  * ``DEVICE_NAMES`` - (required) the names of the target's devices, one item
    per device the mux target returns from ``<target>GetDevices()``.
  * ``CLANG_FORMAT_EXECUTABLE <path>`` - (optional) the path to a
    ``clang-format`` executable, use this to override the default
    ``clang-format`` configuration used by ComputeAorta e.g. if an alternate
    version is present than the one required by ComputeAorta.

  The following CMake cached variables are set:
    * ``<target>_CAPABILITES``
    * ``<target>_DEVICE_NAMES``

  The following CMake targets are added:
    * ``<target>-api-generate``
#]=======================================================================]
function(add_mux_target target)
  cmake_parse_arguments(args "" "HEADER_DIR;CLANG_FORMAT_EXECUTABLE"
    "CAPABILITIES;DEVICE_NAMES" ${ARGN})
  if(args_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "add_mux_target invalid arguments: ${args_UNPARSED_ARGUMENTS}")
  endif()

  if(args_HEADER_DIR STREQUAL "")
    message(FATAL_ERROR "add_mux_target HEADER_DIR not set")
  endif()
  if(NOT IS_DIRECTORY ${args_HEADER_DIR})
    message(FATAL_ERROR
      "add_mux_target HEADER_DIR does not exist: ${args_HEADER_DIR}")
  endif()
  if(NOT args_CAPABILITIES)
    message(FATAL_ERROR "add_mux_target CAPABILITIES not set")
  endif()
  if(NOT args_DEVICE_NAMES)
    message(FATAL_ERROR "add_mux_target DEVICE_NAMES not set")
  endif()

  # Append to the list of Mux target libraries, the cache MUST be updated.
  list(APPEND MUX_TARGET_LIBRARIES ${target})
  set(MUX_TARGET_LIBRARIES ${MUX_TARGET_LIBRARIES}
    CACHE INTERNAL "List of ComputeMux target libraries.")

  # Set the target capabilities variable.
  set(${target}_CAPABILITIES "${args_CAPABILITIES}" CACHE INTERNAL
    "Capabilities of the '${target}' target" FORCE)

  # Set the targets device names variable.
  set(${target}_DEVICE_NAMES "${args_DEVICE_NAMES}" CACHE INTERNAL
    "List of ${target} device names." FORCE)

  set(Schema ${MUX_SOURCE_DIR}/tools/api/mux.xml)
  set(Generator ${MUX_SOURCE_DIR}/../mux/external/api-generator/generate.py)
  set(Header ${args_HEADER_DIR}/${target}.h)

  if(NOT "${args_CLANG_FORMAT_EXECUTABLE}" STREQUAL "")
    set(ClangFormatExe ${args_CLANG_FORMAT_EXECUTABLE})
  elseif(TARGET ClangTools::clang-format)
    set(ClangFormatExe ${ClangTools_clang-format_EXECUTABLE})
  else()
    message(WARNING
      "clang-format missing, ${target}-api-generate disabled")
    return()
  endif()

  # Generate header file and format it.
  add_custom_target(${target}-api-generate
    COMMAND ${PYTHON_EXECUTABLE} ${Generator}
    -f ${target} -F -p mux -i mux/mux.h ${Schema} > ${Header}
    COMMAND ${ClangFormatExe} -i -style=file ${Header}
    DEPENDS ${ClangFormatExe} ${Generator} ${Schema}
    COMMENT "${target}-api-generate")
endfunction()

# Add in tree mux targets.
file(GLOB Entries RELATIVE
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach(Entry ${Entries})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${Entry})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${Entry}/CMakeLists.txt)
      if(CA_MUX_TARGETS_TO_ENABLE STREQUAL "" OR
         ${Entry} IN_LIST CA_MUX_TARGETS_TO_ENABLE)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/${Entry})
      endif()
    endif()
  endif()
endforeach()

# Add out of tree mux targets.
if(NOT CA_EXTERNAL_MUX_TARGET_DIRS STREQUAL "")
  foreach(Entry "${CA_EXTERNAL_MUX_TARGET_DIRS}")
    message(STATUS "Entry: ${Entry}")
    if(NOT IS_DIRECTORY ${Entry})
      message(FATAL_ERROR
        "CA_EXTERNAL_MUX_TARGET_DIRS entry is not a directory: ${Entry}")
    endif()
    get_filename_component(Name ${Entry} NAME)
    if(CA_MUX_TARGETS_TO_ENABLE STREQUAL "" OR
       ${Name} IN_LIST CA_MUX_TARGETS_TO_ENABLE)
      add_subdirectory(${Entry} ${CMAKE_CURRENT_BINARY_DIR}/${Name})
    endif()
  endforeach()
endif()
