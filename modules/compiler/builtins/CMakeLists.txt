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

set(BUILTINS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH
  "Cached path of the builtins source directory.")

# Find suitable tools to build the builtins
include(cmake/FindBuiltinsTools.cmake)
# Debug builds are significantly slower due to using in tree builds of clang,
# opt, and llvm-link; to alleviate slow build times it is possible to external
# release build of these executables by setting
# -DCA_BUILTINS_TOOLS_DIR:PATH=<path> to the path containing the clang, opt,
# and llvm-link binaries.
if(NOT CA_EXTERNAL_BUILTINS AND CA_RUNTIME_COMPILER_ENABLED)
  find_builtins_tools("${CA_BUILTINS_TOOLS_DIR}")
  # ABACUS_BUILD_WITH_RUNTIME_TOOLS is mis-named. When ON, abacus is built with
  # builtins tools (if available) or tools in CA_LLVM_INSTALL_DIR otherwise.
  # When OFF, an external abacus is required.
  set(ABACUS_BUILD_WITH_RUNTIME_TOOLS ON CACHE BOOL
    "Enable building with ComputeAorta runtime tools")
endif()

# Set properties required by abacus
set(RUNTIME_CLHEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/builtins.h")
set(BUILTINS_OPTIONS
  -Werror -Wall -mstackrealign -O3 -fcolor-diagnostics
  -I ${CMAKE_BINARY_DIR}/include
  -I ${BUILTINS_SOURCE_DIR}/abacus/include
  -I ${BUILTINS_SOURCE_DIR}/include)

# compatibility with abacus
set(RUNTIME_COMPILER builtins::compiler)
set(RUNTIME_LINKER builtins::linker)
set(RUNTIME_OPTIONS ${BUILTINS_OPTIONS})

option(ABACUS_BUILD_DOCS
  "Request that Abacus documentation is built also" OFF)
add_ca_subdirectory(abacus)

# Provide variables to set image library headers to adapt to the image library
# to the device runtime. Also do not build a static image library lib as it is
# integrated into the device builtins.
option(CODEPLAY_IMG_BUILD_KERNEL_LIBRARY
  "Enable 'image_library_kernel' static library." OFF)
option(CODEPLAY_IMG_EXPORT_KERNEL_SOURCES
  "Enable export of CMake variables to export kernel source files." ON)
set(CODEPLAY_IMG_INTEGRATION_INCLUDE_DIRS
  ${CMAKE_BINARY_DIR}/include
  ${BUILTINS_SOURCE_DIR}/include/builtins
  ${BUILTINS_SOURCE_DIR}/abacus/include)
# TODO(CA-600): libimg should not depend on the OpenCL headers
set(CODEPLAY_IMG_OPENCL_INCLUDE_DIR
  ${PROJECT_SOURCE_DIR}/source/cl/external/OpenCL-Headers)
option(CODEPLAY_IMG_ENABLE_INSTALL "Enable libimg install target." OFF)
add_ca_subdirectory(libimg)

# If image are supported, check the paths and set additional builtins options
if(CA_ENABLE_HOST_IMAGE_SUPPORT)
  if("${CODEPLAY_IMG_KERNEL_HEADER_PATH}" STREQUAL "")
    message(FATAL_ERROR
      "The CODEPLAY_IMG_KERNEL_HEADER_PATH variable is empty, please set it "
      "to fix this error.")
  endif()
  if("${CODEPLAY_IMG_INCLUDE_DIR}" STREQUAL "")
    message(FATAL_ERROR
      "The CODEPLAY_IMG_INCLUDE_DIR variable is empty, please set it to fix "
      "this error.")
  endif()
  if("${CODEPLAY_IMG_KERNEL_SOURCE_PATH}" STREQUAL "")
    message(FATAL_ERROR
      "The CODEPLAY_IMG_KERNEL_SOURCE_PATH variable is empty, please set it "
      "to fix this error.")
  endif()

  set(BUILTINS_OPTIONS ${BUILTINS_OPTIONS}
    -I${CODEPLAY_IMG_INCLUDE_DIR}
    -I${BUILTINS_SOURCE_DIR}/include/builtins
    -D__CODEPLAY_OCL_IMAGE_SUPPORT)
endif()

set(BUILTINS_PCH_OPTIONS ${BUILTINS_OPTIONS} -fgnu89-inline)

# Add internal builtins target
add_custom_target(internal_builtins)

# Macro adding a command to build a bitcode file from an OpenCL C input.
#
# The argument names are:
# * TRIPLE: Target triple for which to build the bc file.
# * INPUT: File to build into bitcode.
# * OUTPUT: Name of the output bitcode file.
# * EXTRA_OPTS: Any options in addition to BUILTINS_OPTIONS (colon-separated
#               list to work around macro limitations)
# * DEPENDENCIES: Any other things to be passed as a dependency to the command.
#                 Note that the file passed as input is automatically added to
#                 the dependencies.
#
# Variables expected to exist:
# * BUILTINS_COMPILER: compiler binary with which to build the builtins.
# * BUILTINS_OPTIONS: options with which to compile the builtins.
# * ALL_BCS: list to which the output bitcode file will be added.
macro(build_cl_bc_cmd)
  set(oneValueArgs TRIPLE INPUT OUTPUT STD)
  set(multiValueArgs EXTRA_OPTS DEPENDENCIES)
  cmake_parse_arguments(BUILD_CL "" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN})

  add_custom_command(OUTPUT ${BUILD_CL_OUTPUT}
    COMMAND builtins::compiler
      -cc1 -x cl -cl-std=${BUILD_CL_STD} -triple ${BUILD_CL_TRIPLE} ${BUILTINS_OPTIONS}
      ${BUILD_CL_EXTRA_OPTS}
      ${BUILD_CL_INPUT}
      -emit-llvm-bc
      -o ${BUILD_CL_OUTPUT}
    DEPENDS
      builtins::compiler
      ${BUILD_CL_INPUT}
      ${BUILD_CL_DEPENDENCIES})
  list(APPEND ALL_BCS ${BUILD_CL_OUTPUT})
endmacro()

# Macro adding a command to build a bitcode file from an C++ input.
#
# The argument names are:
# * TRIPLE: Target triple for which to build the bc file.
# * INPUT: File to build into bitcode.
# * OUTPUT: Name of the output bitcode file.
# * EXTRA_OPTS: Any options in addition to BUILTINS_OPTIONS (colon-separated
#               list to work around macro limitations)
# * DEPENDENCIES: Any other things to be passed as a dependency to the command.
#                 Note that the file passed as input is automatically added to
#                 the dependencies.
#
# Variables expected to exist:
# * builtins::compiler: compiler binary with which to build the builtins.
# * BUILTINS_OPTIONS: options with which to compile the builtins.
# * ALL_BCS: list to which the output bitcode file will be added.
macro(build_cxx_bc_cmd)
  set(oneValueArgs TRIPLE INPUT OUTPUT)
  set(multiValueArgs EXTRA_OPTS DEPENDENCIES)
  cmake_parse_arguments(BUILD_CXX "" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN})

  add_custom_command(OUTPUT ${BUILD_CXX_OUTPUT}
    COMMAND builtins::compiler
      -cc1 -std=c++11 -triple ${BUILD_CXX_TRIPLE} ${BUILTINS_OPTIONS}
      ${BUILD_CXX_EXTRA_OPTS}
      ${BUILD_CXX_INPUT}
      -emit-llvm-bc
      -o ${BUILD_CXX_OUTPUT}
    DEPENDS
      builtins::compiler
      ${BUILD_CXX_INPUT}
      ${BUILD_CXX_DEPENDENCIES})
  list(APPEND ALL_BCS ${BUILD_CXX_OUTPUT})
endmacro()

macro(build_builtins triple bits fp64 fp16)
  # ALL_BCS list that will be populated by the build_<x>_bc_cmd macros
  set(ALL_BCS "")

  # BUILTINS_EXTRA_OPTIONS contains options required to build the right version
  # of the builtins
  set(BUILTINS_EXTRA_OPTIONS "")

  # cap_suf is the capability suffix
  if(${fp64})
    list(APPEND BUILTINS_EXTRA_OPTIONS "-D__CA_BUILTINS_DOUBLE_SUPPORT")
    set(cap_suf "_fp64")
  else()
    set(cap_suf "")
  endif()

  if(${fp16})
    list(APPEND BUILTINS_EXTRA_OPTIONS "-D__CA_BUILTINS_HALF_SUPPORT")
    set(cap_suf "${cap_suf}_fp16")
  endif()

  macro(remove_dummy_file path)
    if(EXISTS ${path})
      file(READ ${path} fileContent)
      if("${fileContent}" STREQUAL "0")
        file(REMOVE ${path})
      endif()
    endif()
  endmacro()

  # First, to support reconfiguring a build with different combinations of
  # capabilities check to see if any dummy files are present, if so remove them
  # to allow non-dummy files can be built.
  remove_dummy_file(
    ${CMAKE_CURRENT_BINARY_DIR}/builtins_cl${bits}${cap_suf}.bc)
  remove_dummy_file(
    ${CMAKE_CURRENT_BINARY_DIR}/builtins_30_cl${bits}${cap_suf}.bc)
  remove_dummy_file(
    ${CMAKE_CURRENT_BINARY_DIR}/builtins_cxx${bits}${cap_suf}.bc)
  if(CA_ENABLE_HOST_IMAGE_SUPPORT)
    remove_dummy_file(
      ${CMAKE_CURRENT_BINARY_DIR}/builtins_img${bits}${cap_suf}.bc)
  endif()
  remove_dummy_file(
    ${CMAKE_CURRENT_BINARY_DIR}/builtins${bits}${cap_suf}.pch)
  remove_dummy_file(
    ${CMAKE_CURRENT_BINARY_DIR}/builtins${bits}${cap_suf}.bc)

  # The abacus_bitcode* targets are created in abacus/source/CMakeLists.txt.
  # They appear to be required when building with MSVC.
  if(${bits} EQUAL 32)
    set(ABACUS_BITCODE_NAME "abacus_bitcode${cap_suf}")
  else()
    set(ABACUS_BITCODE_NAME "abacus_bitcode64${cap_suf}")
  endif()

  # Build OpenCL C builtins
  build_cl_bc_cmd(TRIPLE ${triple}
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/source/builtins.cl
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/builtins_cl${bits}${cap_suf}.bc
    STD CL1.2 EXTRA_OPTS ${BUILTINS_EXTRA_OPTIONS}
    DEPENDENCIES
    abacus_generate
    ${ABACUS_GENERATED_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/builtins.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/clbuiltins.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/image_library_integration.h)

  # Build OpenCL C 3.0 builtins
  build_cl_bc_cmd(TRIPLE ${triple}
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/source/builtins-3.0.cl
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/builtins_30_cl${bits}${cap_suf}.bc
    STD CL3.0 EXTRA_OPTS ${BUILTINS_EXTRA_OPTIONS}
    DEPENDENCIES
    abacus_generate ${ABACUS_GENERATED_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/builtins.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/clbuiltins-3.0.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/builtins-3.0.h)

  # Build C++ builtins
  build_cxx_bc_cmd(TRIPLE ${triple}
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/source/builtins.cpp
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/builtins_cxx${bits}${cap_suf}.bc
    EXTRA_OPTS ${BUILTINS_EXTRA_OPTIONS}
    DEPENDENCIES
    abacus_generate
    ${ABACUS_GENERATED_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/cxxbuiltins.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/builtins.h)

  if(CA_ENABLE_HOST_IMAGE_SUPPORT)
    # Build image builtins
    build_cxx_bc_cmd(TRIPLE ${triple}
      INPUT ${CODEPLAY_IMG_KERNEL_SOURCE_PATH}
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/builtins_img${bits}${cap_suf}.bc
      EXTRA_OPTS ${BUILTINS_EXTRA_OPTIONS}
      DEPENDENCIES
      abacus_generate
      ${ABACUS_GENERATED_FILES}
      ${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/image_library_integration.h
      ${CODEPLAY_IMG_KERNEL_HEADER_PATH})
  endif()

  set(INITIAL_LL "${CMAKE_CURRENT_BINARY_DIR}/builtins-${triple}${cap_suf}.ll")

  # Link everything into one ll file.
  # This needs to depend on abacus_static because ABACUS_BITCODE_NAME is a
  # custom target and custom commands don't re-run when a custom target they
  # depend on is re-run. However they do for all other kinds of dependencies,
  # so we can depend on abacus_static which uses almost the same sources as
  # abacus_bitcode, and that will trigger a re-run when the sources change.
  add_custom_command(
    OUTPUT "${INITIAL_LL}"
    COMMAND builtins::linker -S -o "${INITIAL_LL}"
      ${ALL_BCS} ${CMAKE_CURRENT_BINARY_DIR}/abacus-${triple}${cap_suf}.bc
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/abacus-${triple}${cap_suf}.bc
      builtins::linker ${ALL_BCS} abacus_static ${ABACUS_BITCODE_NAME})

  # The stripper python script does:
  # * removes OpenCL metadata that becomes huge when the linking above happens
  # * changes hidden functions to internal (so we can remove them entirely)
  set(LL_STRIPPER "${CMAKE_CURRENT_SOURCE_DIR}/scripts/ll_stripper.py")
  set(STRIPPED_LL
    "${CMAKE_CURRENT_BINARY_DIR}/builtins-stripped-${triple}${cap_suf}.ll")

  if(${fp16})
    set(STRIPPER_HALF_SUPPORT "--half-support=ON")
  else()
    set(STRIPPER_HALF_SUPPORT "--half-support=OFF")
  endif()

  add_custom_command(
    OUTPUT "${STRIPPED_LL}"
    COMMAND "${PYTHON_EXECUTABLE}"
      "${LL_STRIPPER}"
      "${INITIAL_LL}"
      "${STRIPPED_LL}"
      "${STRIPPER_HALF_SUPPORT}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS ${INITIAL_LL} ${LL_STRIPPER})

  set(LTO_LL "${CMAKE_CURRENT_BINARY_DIR}/builtins-lto-${triple}${cap_suf}.ll")

  add_custom_command(
    OUTPUT "${LTO_LL}"
    COMMAND ${RUNTIME_COMPILER} -cc1 -Oz -flto -triple ${triple} -emit-llvm
      -o ${LTO_LL} "${STRIPPED_LL}"
    DEPENDS
      "${RUNTIME_COMPILER}" ${STRIPPED_LL})

  # The scrubber CMake script does:
  # * changes the target triple to "unknown-unknown-unknown"
  set(LL_SCRUBBER "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ll_scrubber.cmakescript")
  set(SCRUBBED_LL
    "${CMAKE_CURRENT_BINARY_DIR}/builtins-scrubbed-${triple}${cap_suf}.ll")

  add_custom_command(
    OUTPUT "${SCRUBBED_LL}"
    COMMAND ${CMAKE_COMMAND}
      -DINPUT_FILE:FILEPATH="${LTO_LL}"
      -DOUTPUT_FILE:FILEPATH="${SCRUBBED_LL}"
      -P "${LL_SCRUBBER}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS ${LTO_LL} ${LL_SCRUBBER})

  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/builtins${bits}${cap_suf}.bc"
    COMMAND builtins::linker -o
      "${CMAKE_CURRENT_BINARY_DIR}/builtins${bits}${cap_suf}.bc"
      "${SCRUBBED_LL}"
    DEPENDS builtins::linker ${SCRUBBED_LL} ${ALL_BCS} abacus_static
      ${ABACUS_BITCODE_NAME})

  # Build PCH
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/builtins${bits}${cap_suf}.pch
    COMMAND builtins::compiler
      -cc1 -x cl -cl-std=CL1.2 -triple ${triple} ${BUILTINS_EXTRA_OPTIONS}
      ${BUILTINS_PCH_OPTIONS}
      ${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/builtins.h
      -emit-pch -fno-trigraphs -fno-rtti
      -o ${CMAKE_CURRENT_BINARY_DIR}/builtins${bits}${cap_suf}.pch
    DEPENDS
      builtins::compiler
      ${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/builtins.h)

  # Add custom targets for the bitcode and pch files
  add_custom_target(builtins_bc${bits}${cap_suf}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/builtins${bits}${cap_suf}.bc)

  add_custom_target(builtins_pch${bits}${cap_suf}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/builtins${bits}${cap_suf}.pch)

  # Make the internal builtins target depend on the bc and pch
  add_dependencies(internal_builtins
    builtins_bc${bits}${cap_suf} builtins_pch${bits}${cap_suf})
endmacro()

# Create dummy builtins files instead of the real ones. The linker expects to
# link against these files.
macro(build_dummy_builtins triple bits fp64 fp16)
  if(${fp64})
    set(cap_suf "_fp64")
  else()
    set(cap_suf "")
  endif()

  if(${fp16})
    set(cap_suf "${cap_suf}_fp16")
  endif()

  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/builtins_cl${bits}${cap_suf}.bc "0")
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/builtins_cxx${bits}${cap_suf}.bc "0")
  if(CA_ENABLE_HOST_IMAGE_SUPPORT)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/builtins_img${bits}${cap_suf}.bc "0")
  endif()
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/builtins${bits}${cap_suf}.pch "0")
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/builtins${bits}${cap_suf}.bc "0")
endmacro()

# Create a list of all possible combinations of capabilities
set(dummy_capability_combos
  "32bit" "32bit:fp16" "32bit:fp64" "32bit:fp64:fp16"
  "64bit" "64bit:fp16" "64bit:fp64" "64bit:fp64:fp16")

# Create a list of the file name prefixes corresponding to the capabilities.
# Needs to be cached, because it's used by add_builtins_rc_file, which is
# called later from outside of the current file.
set(CA_ALL_BC_PCH_PREFIXES
  "builtins32"
  "builtins32_fp16"
  "builtins32_fp64"
  "builtins32_fp64_fp16"
  "builtins64"
  "builtins64_fp16"
  "builtins64_fp64"
  "builtins64_fp64_fp16"
  CACHE INTERNAL "Builtins file name prefixes")

# Header prefixes like BC/PCH file prefixes
set(builtins_h_prefixes
  "builtins"
  "builtins_30"
  ${CA_FORCE_HEADERS_PREFIXES})

# Header files can be in many places, so need to store a path corresponding to
# CA_ALL_BUILTINS_H_PREFIXES. This variable does not need to be cached, because
# GenerateRCFile uses *null_terminated files in the build directory.
set(CA_ALL_BUILTINS_H_PATHS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/builtins.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/builtins-3.0.h"
  ${CA_FORCE_HEADERS_PATHS})

# Must be set in the internal cache since as it is used outside of this
# CMakeLists.txt file by add_builtins_rc_file().
set(CA_ALL_BUILTINS_H_PREFIXES "${builtins_h_prefixes}"
  CACHE INTERNAL "Builtins header file name prefixes")

# Determine which combinations of capabilities are required using the
# ${${mux_target}_CAPABILITIES} list. This variable is set in
# mux/targets/<target>/CMakeLists.txt. The list is unordered.
# Without a runtime compiler, no combinations are required.
if(CA_RUNTIME_COMPILER_ENABLED)
  foreach(mux_target ${MUX_TARGET_LIBRARIES})
    set(this_target_capabilities ${${mux_target}_CAPABILITIES})

    if(this_target_capabilities MATCHES "all")
      set(reqd_capability_combos "${dummy_capability_combos}")
      continue()
    endif()

    # Look up bit width
    if(this_target_capabilities MATCHES "32bit")
      set(this_capability "32bit")
    else()
      if(this_target_capabilities MATCHES "64bit")
        set(this_capability "64bit")
      else()
        message(FATAL_ERROR
          "${mux_target} has neither \"32bit\" nor \"64bit\" capability")
      endif()
    endif()

    # Look up double support
    if(this_target_capabilities MATCHES "fp64")
      set(this_capability "${this_capability}:fp64")
    endif()

    # Look up half support
    if(this_target_capabilities MATCHES "fp16")
      set(this_capability "${this_capability}:fp16")
    endif()

    # Store the required capability combinations
    list(APPEND reqd_capability_combos ${this_capability})
  endforeach()

  list(REMOVE_DUPLICATES reqd_capability_combos)
else()
  set(reqd_capability_combos "")
endif()

# Remove required combinations from the dummy list
foreach(reqd_combo ${reqd_capability_combos})
  list(REMOVE_ITEM dummy_capability_combos ${reqd_combo})
endforeach()

# Build builtins for each required combination
foreach(reqd_combo ${reqd_capability_combos})
  if(reqd_combo MATCHES "fp64")
    set(this_fp64 TRUE)
  else()
    set(this_fp64 FALSE)
  endif()

  if(reqd_combo MATCHES "fp16")
    set(this_fp16 TRUE)
  else()
    set(this_fp16 FALSE)
  endif()

  if(reqd_combo MATCHES "32bit")
    build_builtins("spir-unknown-unknown" 32 ${this_fp64} ${this_fp16})
  else()
    build_builtins("spir64-unknown-unknown" 64 ${this_fp64} ${this_fp16})
  endif()
endforeach()

# Set up dummy files for each combination that isn't required
foreach(dummy_combo ${dummy_capability_combos})
  if(dummy_combo MATCHES "fp64")
    set(this_fp64 TRUE)
  else()
    set(this_fp64 FALSE)
  endif()

  if(dummy_combo MATCHES "fp16")
    set(this_fp16 TRUE)
  else()
    set(this_fp16 FALSE)
  endif()

  if(dummy_combo MATCHES "32bit")
    build_dummy_builtins("spir-unknown-unknown" 32 ${this_fp64} ${this_fp16})
  else()
    build_dummy_builtins("spir64-unknown-unknown" 64 ${this_fp64} ${this_fp16})
  endif()
endforeach()

if(CA_EXTERNAL_BUILTINS)
  # File dependencies can only be added to a target via the add_custom_target
  # command.
  add_custom_target(builtins_bitcode
    DEPENDS "${CA_BUILTINS_DIR}/builtins32.bc"
    DEPENDS "${CA_BUILTINS_DIR}/builtins32_fp16.bc"
    DEPENDS "${CA_BUILTINS_DIR}/builtins32_fp64.bc"
    DEPENDS "${CA_BUILTINS_DIR}/builtins32_fp64_fp16.bc"
    DEPENDS "${CA_BUILTINS_DIR}/builtins64.bc"
    DEPENDS "${CA_BUILTINS_DIR}/builtins64_fp16.bc"
    DEPENDS "${CA_BUILTINS_DIR}/builtins64_fp64.bc"
    DEPENDS "${CA_BUILTINS_DIR}/builtins64_fp64_fp16.bc"
    DEPENDS "${CA_BUILTINS_DIR}/builtins32.pch"
    DEPENDS "${CA_BUILTINS_DIR}/builtins32_fp16.pch"
    DEPENDS "${CA_BUILTINS_DIR}/builtins32_fp64.pch"
    DEPENDS "${CA_BUILTINS_DIR}/builtins32_fp64_fp16.pch"
    DEPENDS "${CA_BUILTINS_DIR}/builtins64.pch"
    DEPENDS "${CA_BUILTINS_DIR}/builtins64_fp16.pch"
    DEPENDS "${CA_BUILTINS_DIR}/builtins64_fp64.pch"
    DEPENDS "${CA_BUILTINS_DIR}/builtins64_fp64_fp16.pch")
else()
  add_custom_target(builtins_bitcode DEPENDS internal_builtins)
endif()

set(BUILTINS_EXTRA_SOURCES)

# Boilerplate that goes into bakery.cpp
set(baked_file_declarations "")

# Construct list of builtin binary resources to be compiled into executables
# and shared objects.
set(builtins_resources)
foreach(prefix ${CA_ALL_BC_PCH_PREFIXES})
  list(APPEND builtins_resources
    ${CMAKE_CURRENT_BINARY_DIR}/${prefix}.pch
    ${CMAKE_CURRENT_BINARY_DIR}/${prefix}.bc)
endforeach()
list(LENGTH CA_ALL_BUILTINS_H_PATHS num_headers)
math(EXPR num_headers "${num_headers}-1")
foreach(iter RANGE ${num_headers})
  list(GET CA_ALL_BUILTINS_H_PATHS ${iter} header_path)
  list(APPEND builtins_resources ${header_path})
endforeach()
add_resources(NAMESPACE builtins
  HEADER_FILE ${CMAKE_CURRENT_BINARY_DIR}/include/builtins/resources.h
  RESOURCES ${builtins_resources} DEPENDS builtins_bitcode)

# Generate the body of get_api_force_file_device()
set(get_force_header_src "")
set(get_force_header_str "\
  if (0 == std::strcmp(device_name, \"@dev_name@\")) {
    return rc::builtins::@prefix@\;
  }
")
if(CA_FORCE_HEADERS_PREFIXES)
  list(LENGTH CA_FORCE_HEADERS_PREFIXES num_headers)
  math(EXPR num_headers "${num_headers} - 1")
  foreach(iter RANGE ${num_headers})
    list(GET CA_FORCE_HEADERS_DEVICE_NAMES ${iter} dev_name)
    list(GET CA_FORCE_HEADERS_PREFIXES ${iter} prefix)
    string(CONFIGURE "${get_force_header_str}" get_force_header_string_cfgd
      @ONLY)
    string(APPEND get_force_header_src ${get_force_header_string_cfgd})
  endforeach()
  unset(num_headers)
else()
  # If there are no forced headers, device_name isn't used
  set(get_force_header_src "  (void)device_name;")
endif()

# get_force_header_src is used to configure bakery.cpp.in
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/source/bakery.cpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/source/bakery.cpp
  @ONLY)

add_ca_library(builtins STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/bakery.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/builtins/printf.h
  ${CMAKE_CURRENT_BINARY_DIR}/source/bakery.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/printf.cpp
  ${BUILTINS_EXTRA_SOURCES})

target_include_directories(builtins PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${CODEPLAY_IMG_INTEGRATION_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/libimg/include)
target_include_directories(builtins SYSTEM PUBLIC
  ${CODEPLAY_IMG_OPENCL_INCLUDE_DIR})

target_compile_options(builtins PRIVATE
  $<$<CXX_COMPILER_ID:Clang>:
    # Clang enables erroring on unused command line arguments when used with
    # -Werror. CMake passes a ton of command line arguments to the assembly
    # compiler that aren't used, and cause a mild explosion. Disable this error
    # on clang as it is outside our control what options CMake and the
    # generator provide to clang.
    -Wno-unused-command-line-argument
  >)

target_compile_definitions(builtins PRIVATE CL_VERSION_3_0)

target_link_libraries(builtins PUBLIC cargo abacus_static image_library_host)

# Append to the list of module libraries, the cache MUST be updated.
list(APPEND MODULES_LIBRARIES builtins)
set(MODULES_LIBRARIES ${MODULES_LIBRARIES}
  CACHE INTERNAL "List of module libraries.")

set(BUILTINS_NAMESPACES builtins CACHE INTERNAL
  "List of builtins resource compiler namespaces" FORCE)

#[=======================================================================[.rst:
.. cmake:command:: add_target_builtins

  Add compile commands for target specific builtins source files then add the
  linked bitcode file to be embedded in executables and shared libraries.

  Arguments:
    * ``target`` - the name of the target the builtins are associated with.

  Keyword Arguments:
    * ``32BIT`` - flag to specify a 32-bit target architecture, mutually
      exclusive with ``64BIT`.
    * ``64BIT`` - flag to specify a 64-bit target architecture, mutually
      exclusive with ``32BIT`.
    * ``FP16`` - flag to enable 16-bit (half) floating-point support.
    * ``FP64`` - flag to enable 64-bit (double) floating-point support.
    * ``CLSTD`` - the OpenCL standard to target, e.g. ``CL3.0`, only required
      when compiling ``.cl`` source files.
    * ``TRIPLE`` - the target triple to be used when compiling the builtins
      source files, if not set ``spir-unknown-unknown` will be used if
      ``32BIT`` is specified and ``spir64-unknown-unknown`` will be used if
      ``64BIT`` is specified.
    * ``OPTIONS`` - a list of additional compile options.
    * ``SOURCES`` - the list of source files which implement the builtins.
#]=======================================================================]
function(add_target_builtins target)
  cmake_parse_arguments(args
    "32BIT;64BIT;FP16;FP64"     # options
    "HEADER_FILE;CLSTD;TRIPLE"  # one value keywords
    "OPTIONS;SOURCES"           # multi value keywords
    ${ARGN})

  if(NOT args_32BIT AND NOT args_64BIT)
    message(FATAL_ERROR "32BIT or 64BIT must be provided")
  endif()
  if(args_32BIT AND args_64BIT)
    message(FATAL_ERROR "32BIT and 64BIT are mutually exclusive")
  endif()

  # Use the provided triple if set, otherwise compile in SPIR mode.
  if(args_TRIPLE)
    set(triple ${args_TRIPLE})
  elseif(args_32BIT)
    set(triple spir-unknown-unknown)
  elseif(args_64BIT)
    set(triple spir64-unknown-unknown)
  endif()

  # Loop over all the input source files.
  set(compiled_bcs)
  foreach(input ${args_SOURCES})
    # Make sure the input source file exists.
    if(NOT EXISTS ${input})
      if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${input})
        set(input ${CMAKE_CURRENT_SOURCE_DIR}/${input})
      else()
        message(FATAL_ERROR "source file does not exist: ${input}")
      endif()
    endif()

    # Get the source file name, without the directory.
    get_filename_component(compiled_bc ${input} NAME)
    set(compiled_bc ${CMAKE_CURRENT_BINARY_DIR}/${compiled_bc}-${triple}.bc)

    # Detect if this is a .cl or .cpp file.
    get_filename_component(input_ext ${input} LAST_EXT)
    if(input_ext STREQUAL .cl)
      if(NOT args_CLSTD)
        message(FATAL_ERROR "CLSTD is required for compiling: ${input}")
      endif()
      set(lang_opts -x cl -cl-std=${args_CLSTD})
    elseif(input_ext STREQUAL .cpp)
      set(lang_opts -x c++ -std=c++11)
    else()
      message(FATAL_ERROR "unsupported file type (not .cl or .cpp): ${input}")
    endif()

    # Add the command to compile the source file into a bitcode file.
    file(RELATIVE_PATH compiled_bc_relpath
      "${CMAKE_BINARY_DIR}" "${compiled_bc}")

    add_custom_command(OUTPUT ${compiled_bc}
      COMMAND builtins::compiler -cc1 ${lang_opts} -triple ${triple}
        -include ${BUILTINS_SOURCE_DIR}/include/builtins/builtins.h
        -I${CMAKE_BINARY_DIR}/include
        -I${BUILTINS_SOURCE_DIR}/include
        -I${BUILTINS_SOURCE_DIR}/abacus/include
        -I${BUILTINS_SOURCE_DIR}/libimg/include
        # Enable/disable floating-point capabilities
        $<$<BOOL:${args_FP16}>:-D__CA_BUILTINS_HALF_SUPPORT>
        $<$<BOOL:${args_FP64}>:-D__CA_BUILTINS_DOUBLE_SUPPORT>
        # Pretend to compile in SPIR mode when using a different triple.
        $<$<AND:$<BOOL:${args_TRIPLE}>,$<BOOL:${args_32BIT}>>:-D__SPIR32__>
        $<$<AND:$<BOOL:${args_TRIPLE}>,$<BOOL:${args_64BIT}>>:-D__SPIR64__>
        ${args_OPTIONS} -emit-llvm-bc -o ${compiled_bc} ${input}
      MAIN_DEPENDENCY ${input}
      DEPENDS internal_builtins
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building BC object ${compiled_bc_relpath}")

    # Add the output bitcode file to the list of compiled bitcode files.
    list(APPEND compiled_bcs ${compiled_bc})
  endforeach()

  # Link the compiled bitcode files into a single bitcode file.
  set(linked_bc ${CMAKE_CURRENT_BINARY_DIR}/${target}-${triple}.bc)
  file(RELATIVE_PATH linked_bc_relpath "${CMAKE_BINARY_DIR}" "${linked_bc}")
  add_custom_command(OUTPUT ${linked_bc}
    COMMAND builtins::linker -o ${linked_bc} ${compiled_bcs}
    DEPENDS ${compiled_bcs}
    COMMENT "Linking BC object ${linked_bc_relpath}")

  # Create a resource compiler namespace.
  add_resources(NAMESPACE ${target}
    HEADER_FILE ${args_HEADER_FILE} RESOURCES ${linked_bc})

  # Append the resource compiler namespace to the list of builtins resource
  # compiler namespaces.
  list(APPEND BUILTINS_NAMESPACES ${target})
  set(BUILTINS_NAMESPACES "${BUILTINS_NAMESPACES}" CACHE INTERNAL
    "List of builtins resource compiler namespaces" FORCE)
endfunction()
