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

# Add abacus source directories to get lists of source files.
add_subdirectory(abacus_cast)
add_subdirectory(abacus_common)
add_subdirectory(abacus_extra)
add_subdirectory(abacus_geometric)
add_subdirectory(abacus_integer)
add_subdirectory(abacus_math)
add_subdirectory(abacus_memory)
add_subdirectory(abacus_misc)
add_subdirectory(abacus_relational)

set(abacus_sources
  ${abacus_cast_sources} ${abacus_common_sources} ${abacus_extra_sources}
  ${abacus_geometric_sources} ${abacus_integer_sources} ${abacus_math_sources}
  ${abacus_memory_sources} ${abacus_misc_sources} ${abacus_relational_sources})

add_library(abacus_static STATIC ${abacus_sources})
add_dependencies(abacus_static abacus_generate)
target_compile_definitions(abacus_static PRIVATE
  "ABACUS_ENABLE_OPENCL_1_2_BUILTINS"
  "ABACUS_ENABLE_OPENCL_3_0_BUILTINS")

# If extra ComputeAorta commands exist, use them.
if(COMMAND add_ca_tidy)
  add_ca_tidy(abacus_static ${abacus_sources})
  if(TARGET tidy-abacus_static)
    add_dependencies(tidy-abacus_static abacus_generate)
  endif()
endif()

set_property(TARGET abacus_static PROPERTY CXX_STANDARD 11)

target_include_directories(abacus_static PUBLIC
  ${CMAKE_BINARY_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_options(abacus_static
  PUBLIC
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:QCC>>:
    $<$<COMPILE_LANGUAGE:CXX>:
      -fno-rtti         # Disable runtime type information
    >
    $<$<BOOL:${UNIX}>:
      -fPIC             # Enable PIC, but only for *NIX, i.e not MinGW Windows
    >
  >
  $<$<CXX_COMPILER_ID:MSVC>:
    /GR-          # disable runtime type information
    /EHs-         # disable C++ exception handling
    /EHc-         # disable catching C++ exceptions
  >
  PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
    -Wall         # enable all warnings
    -Wextra       # enable extra warnings
  >
  $<$<CXX_COMPILER_ID:MSVC>:
    /WX           # warnings as errors
    /wd4127       # disable warning conditional expression is constant
    /wd4305       # disable warning constant truncation
    /wd4310       # disable warning cast truncation
    /Za           # disable MSVC language extensions
    /fp:precise   # enable precise floating point mode
    /fp:except-   # with no exceptions
  >
  $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:
    /RTCc         # enable reporting value assigned to smaller data type
  >
  $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:
    /Gy           # enable function-level linking
    /Ox           # enable full optimization
  >)
# abacus_static is always built with doubles even if the target device doesn't
# support them
# TODO: CA-772 Target will eventually need to define __CA_BUILTINS_HALF_SUPPORT
# in some circumstances
target_compile_definitions(abacus_static PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:ABACUS_LIBRARY_STATIC>
  PUBLIC __CA_BUILTINS_DOUBLE_SUPPORT)

if(${CA_ENABLE_COVERAGE} AND ${CA_RUNTIME_COMPILER_ENABLED})
  # When online coverage is enabled we add the modules so that the XML file can
  # be generated automatically.
  set(ABACUS_STATIC_BINARY_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/abacus_static.dir)
  add_coverage_modules(
    ${CMAKE_CURRENT_SOURCE_DIR}/abacus_cast
    ${ABACUS_STATIC_BINARY_DIR}/abacus_cast
    ${CMAKE_CURRENT_SOURCE_DIR}/abacus_common
    ${ABACUS_STATIC_BINARY_DIR}/abacus_common
    ${CMAKE_CURRENT_SOURCE_DIR}/abacus_geometric
    ${ABACUS_STATIC_BINARY_DIR}/abacus_geometric
    ${CMAKE_CURRENT_SOURCE_DIR}/abacus_integer
    ${ABACUS_STATIC_BINARY_DIR}/abacus_integer
    ${CMAKE_CURRENT_SOURCE_DIR}/abacus_math
    ${ABACUS_STATIC_BINARY_DIR}/abacus_math
    ${CMAKE_CURRENT_SOURCE_DIR}/abacus_memory
    ${ABACUS_STATIC_BINARY_DIR}/abacus_memory
    ${CMAKE_CURRENT_SOURCE_DIR}/abacus_misc
    ${ABACUS_STATIC_BINARY_DIR}/abacus_misc
    ${CMAKE_CURRENT_SOURCE_DIR}/abacus_relational
    ${ABACUS_STATIC_BINARY_DIR}/abacus_relational)
endif()

if(${ABACUS_BUILD_WITH_RUNTIME_TOOLS})
  if("${RUNTIME_COMPILER}" STREQUAL "" OR "${RUNTIME_LINKER}" STREQUAL "" OR
  "${RUNTIME_CLHEADER}" STREQUAL "")
    message(FATAL_ERROR
      "ABACUS_BUILD_WITH_RUNTIME_TOOLS was set but runtime tools not found!")
  endif()

  string(REPLACE "-Werror" "" ABACUS_RUNTIME_OPTIONS "${RUNTIME_OPTIONS}")

  # Set up building abacus-${triple}${cap_suf}.bc
  #
  # Arguments:
  # * triple: E.g., spir64-unknown-unknown
  # * fp64: With doubles (TRUE/FALSE). Adds "_fp64" to ${cap_suf} if TRUE.
  # * fp16: With halfs (TRUE/FALSE}. Adds "_fp16" to ${cap_suf} if TRUE.
  macro(bitcode_gen triple fp64 fp16)
    set(ALL_BCS "")

    set(BUILTINS_EXTRA_OPTIONS "")

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

    foreach(SOURCE IN LISTS abacus_sources)
      string(LENGTH "${CMAKE_CURRENT_SOURCE_DIR}/" LENGTH)
      string(SUBSTRING "${SOURCE}" ${LENGTH} -1 SUB_SOURCE)
      string(REGEX REPLACE "^.*\\.\(c[lp]*\)$" "\\1" SOURCE_TYPE "${SOURCE}")

      set(XTYPE "${SOURCE_TYPE}") # Language for -x option.
      set(XOPTS "") # Extra language specific options.
      set(OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/bc/${triple}${cap_suf}/${SUB_SOURCE}.bc")

      # The 'Unix Makefiles' and 'MinGW Makefiles' generators do not
      # automatically create the output directories the bc files are output to,
      # this causes compilation failures as the output location does not exist.
      # So we have to manually create these output directories.
      if(${CMAKE_GENERATOR} MATCHES ".* Makefiles")
        get_filename_component(OUTPUT_DIR ${OUTPUT} DIRECTORY)
        file(MAKE_DIRECTORY ${OUTPUT_DIR})
      endif()

      if("${SOURCE_TYPE}" STREQUAL "cpp")
        set(XTYPE "c++")
        set(XOPTS "-std=c++11")
      elseif("${SOURCE_TYPE}" STREQUAL "cl")
        set(XOPTS "-cl-std=CL1.2")
      endif()

      # This is required to correctly expose all the builtins since some of their
      # defintions come from this file.
      set(ENABLE_OPENCL_BUILTINS
        "-DABACUS_ENABLE_OPENCL_1_2_BUILTINS -DABACUS_ENABLE_OPENCL_3_0_BUILTINS")
      add_custom_command(
        OUTPUT "${OUTPUT}"
        COMMAND ${RUNTIME_COMPILER} -cc1 -x ${XTYPE} ${XOPTS} -triple ${triple}
          ${ABACUS_RUNTIME_OPTIONS} ${BUILTINS_EXTRA_OPTIONS} ${ENABLE_OPENCL_BUILTINS}
          -include "${RUNTIME_CLHEADER}"
          -emit-llvm-bc -o "${OUTPUT}"
          "${SOURCE}"
        DEPENDS "${SOURCE}" "${RUNTIME_COMPILER}" "${RUNTIME_CLHEADER}"
          abacus_generate ${ABACUS_GENERATED_FILES})

      set(ALL_BCS "${ALL_BCS};${OUTPUT}")
    endforeach()

    # Maximum command line length of 8191 in Windows, the list of .bc files
    # will go over that, so output the list to a file that the linker can use.
    # We have to process the list one at a time so that ';' can be replaced
    # with ' ', using ' ' directly causes problems with Ninja.
    set(ALL_BCS_FILE
      "${CMAKE_CURRENT_BINARY_DIR}/bc/${triple}${cap_suf}/all.sources")
    file(REMOVE "${ALL_BCS_FILE}")
    foreach(BC IN LISTS ALL_BCS)
      file(APPEND ${ALL_BCS_FILE} " ${BC}")
    endforeach()

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/../../abacus-${triple}${cap_suf}.bc"
      COMMAND ${RUNTIME_LINKER} "@${ALL_BCS_FILE}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/../../abacus-${triple}${cap_suf}.bc"
      DEPENDS "${RUNTIME_LINKER}" ${ALL_BCS}
    )
  endmacro()

  # The bitcode_gen macro does lots of stuff, the end result of which is an
  # abacus-${triple}${cap_suf}.bc file. We can invoke this macro lots of times,
  # because it doesn't cause anything to build--it just tells cmake how to make
  # the abacus-*.bc files.
  bitcode_gen(spir-unknown-unknown FALSE FALSE)   # 32-bit
  bitcode_gen(spir-unknown-unknown FALSE TRUE)    # 32-bit, halfs
  bitcode_gen(spir-unknown-unknown TRUE FALSE)    # 32-bit, doubles
  bitcode_gen(spir-unknown-unknown TRUE TRUE)     # 32-bit, doubles, halfs
  bitcode_gen(spir64-unknown-unknown FALSE FALSE) # 64-bit
  bitcode_gen(spir64-unknown-unknown FALSE TRUE)  # 64-bit, halfs
  bitcode_gen(spir64-unknown-unknown TRUE FALSE)  # 64-bit, doubles
  bitcode_gen(spir64-unknown-unknown TRUE TRUE)   # 64-bit, doubles, halfs

  # Attach the abacus-*.bc files to targets so that builtins/CMakeLitsts.txt
  # can see the abacus-*.bc files when it's building INITIAL_LL.
  add_custom_target(abacus_bitcode
    DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/../../abacus-spir-unknown-unknown.bc"
  )
  add_custom_target(abacus_bitcode_fp16
    DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/../../abacus-spir-unknown-unknown_fp16.bc"
  )
  add_custom_target(abacus_bitcode_fp64
    DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/../../abacus-spir-unknown-unknown_fp64.bc"
  )
  add_custom_target(abacus_bitcode_fp64_fp16
    DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/../../abacus-spir-unknown-unknown_fp64_fp16.bc"
  )
  add_custom_target(abacus_bitcode64
    DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/../../abacus-spir64-unknown-unknown.bc"
  )
  add_custom_target(abacus_bitcode64_fp16
    DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/../../abacus-spir64-unknown-unknown_fp16.bc"
  )
  add_custom_target(abacus_bitcode64_fp64
    DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/../../abacus-spir64-unknown-unknown_fp64.bc"
  )
  add_custom_target(abacus_bitcode64_fp64_fp16
    DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/../../abacus-spir64-unknown-unknown_fp64_fp16.bc"
  )
endif()
