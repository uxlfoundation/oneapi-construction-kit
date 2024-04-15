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

# Only include this module once
if(${BUILTINS_TOOLS_CMAKE})
  return()
endif()
set(BUILTINS_TOOLS_CMAKE INCLUDED)

include(CAPlatform)  # Bring in CA_HOST_EXECUTABLE_SUFFIX.

# Find and set builtins tools cmake targets. These builtins tools are used to
# build the builtins into bitcode.
#
# The tools dir parameter is a path to a directory in which the function should
# look for the builtins tools, if it is an empty string, the function will
# attempt to use eventual in tree targets of the builtins tools.
function(find_builtins_tools tools_dir)
  if ("${tools_dir}" STREQUAL "" AND TARGET llvm-link AND TARGET clang)
    # If we're in-tree, we already have targets for the builtins tools. Create
    # alias executables for them.
    if(NOT TARGET builtins::compiler)
      add_executable(builtins::compiler ALIAS clang)
    endif()
    if(NOT TARGET builtins::linker)
      add_executable(builtins::linker ALIAS llvm-link)
    endif()

    # Exit early here. Note we don't check that the builtins tools version
    # matches the LLVM version; we assume that is the case as it's an in-tree
    # build.
    return()
  endif()

  set(BUILTINS_COMPILER ${tools_dir}/clang${CA_HOST_EXECUTABLE_SUFFIX})
  set(BUILTINS_LINKER ${tools_dir}/llvm-link${CA_HOST_EXECUTABLE_SUFFIX})
  set(BUILTINS_LLVM_CONFIG ${tools_dir}/llvm-config${CA_HOST_EXECUTABLE_SUFFIX})
  foreach(option ZLIB ZSTD)
    if(LLVM_ENABLE_${option})
      continue()
    endif()
    if(NOT DEFINED BUILTINS_LLVM_CMAKE)
      execute_process(
        COMMAND ${BUILTINS_LLVM_CONFIG} --cmakedir
        OUTPUT_VARIABLE BUILTINS_LLVM_CMAKE
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      set(BUILTINS_LLVM_CMAKE ${BUILTINS_LLVM_CMAKE}/LLVMConfig.cmake)
    endif()
    if(NOT EXISTS ${BUILTINS_LLVM_CMAKE})
      message(
        WARNING
        "builtins tools installation does not have necessary cmake modules; "
        "cannot determine whether the builtins compiler generates "
        "${option}-compressed bitcode")
      continue()
    endif()
    file(STRINGS ${BUILTINS_LLVM_CMAKE} option_set
      REGEX "^set\\(LLVM_ENABLE_${option} .*\\)$")
    if(NOT option_set)
      message(FATAL_ERROR "LLVMConfig.cmake found but lacks definition for "
        "LLVM_ENABLE_${option}")
    endif()
    # CMake prior to 3.29 does not set capture groups in file REGEX, so do it
    # again using string REGEX.
    string(REGEX MATCH "^set\\(LLVM_ENABLE_${option} (.*)\\)$"
      option_set "${option_set}")
    if(CMAKE_MATCH_1)
      message(FATAL_ERROR "builtins compiler will generate ${option}-compressed"
        " bitcode, but the target compiler does not have ${option} support")
    endif()
  endforeach()

  # Create imported targets for the builtins tools
  if(NOT TARGET builtins::compiler)
    add_executable(builtins::compiler IMPORTED GLOBAL)
    set_target_properties(builtins::compiler PROPERTIES
      IMPORTED_LOCATION ${BUILTINS_COMPILER})
  endif()
  if(NOT TARGET builtins::linker)
    add_executable(builtins::linker IMPORTED GLOBAL)
    set_target_properties(builtins::linker PROPERTIES
      IMPORTED_LOCATION ${BUILTINS_LINKER})
  endif()

  set(llvm_version "${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}.${LLVM_VERSION_PATCH}")

  # Check that the version of the compiler building the builtins matches the
  # version of the compiler linked into the runtime.
  execute_process(COMMAND ${BUILTINS_COMPILER} --version
    OUTPUT_VARIABLE version_string RESULT_VARIABLE result)
  if(result EQUAL 0)
    string(REGEX MATCH "clang version [0-9]+\\.[0-9]+\\.[0-9]"
      version_string ${version_string})
    string(REPLACE "clang version " "" version_string ${version_string})
    if(NOT llvm_version VERSION_EQUAL version_string)
      message(FATAL_ERROR
              "Builtins: LLVM tool versions do not match\n"
              "LLVM_PACKAGE_VERSION: '${llvm_version}'\n"
              "builtins::compiler version: '${version_string}'")
    else()
      message(STATUS "Builtins: LLVM tool versions match")
    endif()
  else()
    message(WARNING "Builtins: could not verify LLVM versions match")
  endif()
endfunction()
