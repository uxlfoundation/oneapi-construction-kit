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

# Find and set builtins tools cmake variable, these builtins tools are used to
# build the builtins into bitcode.
#
# This function sets the three CMake variables:
# * BUILTINS_COMPILER
# * BUILTINS_LINKER
# * BUILTINS_OPTIMIZER
#
# The tools dir parameter is a path to a directory in which the function should
# look for the builtins tools, if it is an empty string, the function will
# attempt to use eventual in tree targets of the builtins tools.
function(find_builtins_tools tools_dir)
  if("${tools_dir}" STREQUAL ""
      AND TARGET clang AND TARGET llvm-link AND TARGET opt)
    message(STATUS "Builtins tools found in tree targets.")
    set(BUILTINS_COMPILER clang)
    set(BUILTINS_LINKER llvm-link)
  else()
    set(BUILTINS_COMPILER ${tools_dir}/clang${CA_HOST_EXECUTABLE_SUFFIX})
    set(BUILTINS_LINKER ${tools_dir}/llvm-link${CA_HOST_EXECUTABLE_SUFFIX})
    if(NOT LLVM_ENABLE_ZLIB)
      set(BUILTINS_COMPILER_ZLIB_ENABLED ON)
      set(BUILTINS_LLVM_CONFIG
        ${tools_dir}/llvm-config${CA_HOST_EXECUTABLE_SUFFIX})
      execute_process(
        COMMAND ${BUILTINS_LLVM_CONFIG} --cmakedir
        OUTPUT_VARIABLE BUILTINS_LLVM_CMAKE
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )

      set(BUILTINS_LLVM_CMAKE ${BUILTINS_LLVM_CMAKE}/LLVMConfig.cmake)
      if(EXISTS ${BUILTINS_LLVM_CMAKE})
        file(STRINGS ${BUILTINS_LLVM_CMAKE} builtins_llvm_config
          REGEX "LLVM_ENABLE_ZLIB (1|ON|YES|TRUE|Y|0|OFF|NO|FALSE|N)")
        foreach(line ${builtins_llvm_config})
          string(STRIP "${line}" line)
          string(REGEX MATCHALL
            "^set\\(LLVM_ENABLE_ZLIB (ON|OFF|0|1)\\)$" _ "${line}")
          set(BUILTINS_COMPILER_ZLIB_ENABLED ${CMAKE_MATCH_1})
        endforeach()
        if(BUILTINS_COMPILER_ZLIB_ENABLED)
          message(FATAL_ERROR "builtins compiler will generate zlib-compressed "
            "bitcode, but the target compiler does not have zlib support")
            return()
        endif()
      else()
        message(
          WARNING
          "builtins tools installation does not have necessary cmake modules; "
          "cannot determine whether the builtins compiler generates "
          "zlib-compressed bitcode"
        )
      endif()
    endif()

    string(REGEX MATCH ".*clang${CA_HOST_EXECUTABLE_SUFFIX}$"
      COMPILER_RESULT ${BUILTINS_COMPILER})
    if(NOT "${BUILTINS_COMPILER}" STREQUAL "${COMPILER_RESULT}")
      message(FATAL_ERROR
        "Clang is the only supported compiler '${BUILTINS_COMPILER}' is not "
        "supported. Clean the build directory and set BUILTINS_COMPILER to "
        "'<llvm-build>/bin/clang${CA_HOST_EXECUTABLE_SUFFIX}' to fix this "
        "error.")
    endif()
    if(NOT EXISTS ${COMPILER_RESULT})
      message(FATAL_ERROR
        "${COMPILER_RESULT} does not exist. Clean the build directory and set "
        "CA_BUILTINS_TOOLS_DIR to '<llvm-build>' or BUILTINS_COMPILER to "
        "'<llvm-build>/bin/clang${EXTENSION}' to fix this error.")
    endif()

    string(REGEX MATCH ".*llvm-link${CA_HOST_EXECUTABLE_SUFFIX}$"
      LINKER_RESULT ${BUILTINS_LINKER})
    if(NOT "${BUILTINS_LINKER}" STREQUAL "${LINKER_RESULT}")
      message(FATAL_ERROR
        "llvm-link is the only supported linker '${BUILTINS_LINKER}' "
        "is not supported. Clean the build directory and set "
        "BUILTINS_LINKER to '<llvm-build>/bin/llvm-link${CA_HOST_EXECUTABLE_SUFFIX}' "
        "to fix this error.")
    endif()
    if(NOT EXISTS ${LINKER_RESULT})
      message(FATAL_ERROR
        "${tools_dir}/${LINKER_RESULT} does not exist. Clean the build "
        "directory and set CA_BUILTINS_TOOLS_DIR to '<llvm-build>' or "
        "BUILTINS_LINKER to '<llvm-build>/bin/llvm-link${EXTENSION}' to fix "
        "this error.")
    endif()
  endif()

  # Check that the version of the compiler building the builtins matches the
  # version of the compiler linked into the runtime.
  execute_process(COMMAND ${BUILTINS_COMPILER} --version
    OUTPUT_VARIABLE version_string RESULT_VARIABLE result)
  if(result EQUAL 0 AND NOT CA_NATIVE_CPU)
    string(REPLACE "svn" "" llvm_version ${LLVM_PACKAGE_VERSION})
    string(REGEX MATCH "clang version [0-9]+\\.[0-9]+\\.[0-9]"
      version_string ${version_string})
    string(REPLACE "clang version " "" version_string ${version_string})
    if(NOT llvm_version VERSION_EQUAL version_string)
      message(FATAL_ERROR
              "Builtins: LLVM tool versions do not match\n"
              "LLVM_PACKAGE_VERSION: '${llvm_version}'\n"
              "BUILTINS_COMPILER version: '${version_string}'")
    else()
      message(STATUS "Builtins: LLVM tool versions match")
    endif()
  else()
    message(WARNING "Builtins: could not verify LLVM versions match")
  endif()

  # Propagate the builtins tools variable to the parent scope
  set(BUILTINS_COMPILER ${BUILTINS_COMPILER} PARENT_SCOPE)
  set(BUILTINS_LINKER ${BUILTINS_LINKER} PARENT_SCOPE)

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
endfunction()
