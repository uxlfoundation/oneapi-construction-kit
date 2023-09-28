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

if(CA_NATIVE_CPU)
  add_subdirectory(multi_llvm)
  add_subdirectory(utils)
  return()
endif()
add_subdirectory(builtins)
if(CA_RUNTIME_COMPILER_ENABLED)
  if(CA_ENABLE_TESTS)
    # Open up an active set of 'compiler' lit suites
    ca_umbrella_lit_testsuite_open(compiler)
  endif()

  add_subdirectory(multi_llvm)
  add_subdirectory(source/base)
  add_subdirectory(targets)
  add_subdirectory(utils)
  add_subdirectory(riscv)
  add_subdirectory(tools)
  add_subdirectory(spirv-ll)
  add_subdirectory(vecz)
  set(COMPILER_CONFIG_SOURCE ${PROJECT_BINARY_DIR}/modules/compiler/source/config.cpp)

  set(COMPILER_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/context.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/kernel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/limits.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/module.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/target.h)

  # Define the static compiler library.
  add_ca_library(compiler-static STATIC
    ${COMPILER_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/library/include/compiler/library.h
    ${CMAKE_CURRENT_SOURCE_DIR}/library/source/library.cpp
    ${COMPILER_CONFIG_SOURCE})
  target_include_directories(compiler-static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/include
    ${CMAKE_CURRENT_SOURCE_DIR}/library/include
    SYSTEM PUBLIC
    ${spirv-headers_SOURCE_DIR}/include)
  target_link_libraries(compiler-static
    PUBLIC builtins PRIVATE compiler-base mux-utils)

  # Any built compiler targets should be enabled by default
  # This can be limited with CA_MUX_COMPILERS_TO_ENABLE which
  # will dictate whether to build in that directory.
  set(MUX_COMPILER_LIBRARIES ${MUX_COMPILER_LIBRARIES}
      CACHE INTERNAL "List of ComputeMux compiler libraries.")

  # Validate the MUX_COMPILER_LIBRARIES variable actually contains a compiler
  # target.
  list(LENGTH MUX_COMPILER_LIBRARIES MUX_COMPILER_COUNT)
  if(0 EQUAL ${MUX_COMPILER_COUNT})
    message(FATAL_ERROR "Mux: No compilers enabled, append your target library "
      "to the MUX_COMPILER_LIBRARIES list, or disable the compiler library "
      "using the CA_RUNTIME_COMPILER_ENABLED CMake option.")
  else()
    message(STATUS "Mux compiler libraries: ${MUX_COMPILER_LIBRARIES}")
  endif()

  set(COMPILER_INFO_NAMES "")
  set(COMPILER_INFO_HEADERS "")
  foreach(TARGET ${MUX_COMPILER_LIBRARIES})
    get_target_property(target_compiler_info_names ${TARGET} COMPILER_INFO_NAMES)
    get_target_property(target_compiler_info_headers ${TARGET} COMPILER_INFO_HEADERS)
    list(APPEND COMPILER_INFO_NAMES ${target_compiler_info_names})
    list(APPEND COMPILER_INFO_HEADERS ${target_compiler_info_headers})
    target_link_libraries(compiler-static PRIVATE ${TARGET})
  endforeach()

  # Due to CMake splitting the lists when passing it to the custom command it is
  # not passed through correctly, replacing ';' with ',' and doing the opposite
  # on the other side works around this issue.
  string(REPLACE ";" "," COMPILER_INFO_NAMES_LIST "${COMPILER_INFO_NAMES}")
  string(REPLACE ";" "," COMPILER_INFO_HEADERS_LIST "${COMPILER_INFO_HEADERS}")

  add_custom_command(OUTPUT ${COMPILER_CONFIG_SOURCE}
    COMMAND ${CMAKE_COMMAND}
    -DCOMPILER_CONFIG_SOURCE=${COMPILER_CONFIG_SOURCE}
    -DCOMPILER_INFO_NAMES=${COMPILER_INFO_NAMES_LIST}
    -DCOMPILER_INFO_HEADERS=${COMPILER_INFO_HEADERS_LIST}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/compiler-config.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/compiler-config.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  add_custom_target(compiler-config DEPENDS ${COMPILER_CONFIG_SOURCE})
  add_dependencies(compiler-static compiler-config)

  if(TARGET tidy-compiler-static)
    add_dependencies(tidy-compiler-static compiler-config)
  endif()

  # Define the shared compiler library that can be loaded at runtime through
  # the loader library.
  add_ca_library(compiler SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/source/export.cpp
    $<$<PLATFORM_ID:Windows>:${BUILTINS_RC_FILE}>)
  target_link_libraries(compiler PRIVATE mux compiler-static
                                 PUBLIC builtins)
  target_resources(compiler NAMESPACES ${BUILTINS_NAMESPACES})

  # Choose library linker options.
  # Debugging llvm becomes very frustrating without the `dump` methods which
  # are otherwise stripped. Don't strip public symbols for any *full* debug
  # builds
  if(CA_PLATFORM_LINUX OR MINGW)
    # Export API entry point symbols for Linux, or if we're using a GNU compiler
    # on Windows.  I.e. a MinGW build on Windows uses the Linux symbol exports,
    # not the Windows ones.
    set(COMPILER_EXPORT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/source/export-linux.sym)
    if (NOT CMAKE_BUILD_TYPE STREQUAL Debug)
      # Retain only the symbols listed in the file filename, discarding all others.
      string(APPEND COMPILER_LINK_FLAGS
        " -Xlinker --retain-symbols-file=${COMPILER_EXPORT_FILE}")
      # Delete all local symbols.
      string(APPEND COMPILER_LINK_FLAGS_RELEASE " -Xlinker --discard-all")
      # Enable garbage collection of unused input sections.
      string(APPEND COMPILER_LINK_FLAGS_RELEASE " -Xlinker --gc-sections")
      # Sort the common symbols by size when placing them in the appropriate output
      # sections.
      string(APPEND COMPILER_LINK_FLAGS_RELEASE " -Xlinker --sort-common")
    endif()

    if(NOT CA_PLATFORM_ANDROID)
      # Strip any remaining debug info that may be there from customer static
      # libraries.
      string(APPEND COMPILER_LINK_FLAGS_RELEASE " -Xlinker --strip-debug")
    endif()
  elseif(CA_PLATFORM_WINDOWS AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Set exports definition file for MSVC Windows (not MinGW, that takes the
    # Linux equivalent path) and link flags to use it.
    set(COMPILER_EXPORT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/source/export-windows.def)
    string(APPEND COMPILER_LINK_FLAGS " /DEF:${COMPILER_EXPORT_FILE}")
  else()
    message(WARNING "Unhandled build environment.")
  endif()

  # Specify chosen library linker options, use the specified output name and
  # version of the Vulkan library.
  # Ensure we're not overwriting existing link options
  get_target_property(tgt_link_flags compiler LINK_FLAGS)
  get_target_property(tgt_link_flags_release compiler LINK_FLAGS_RELEASE)
  get_target_property(tgt_link_flags_minsizerel compiler LINK_FLAGS_MINSIZEREL)
  get_target_property(
    tgt_link_flags_releaseassert compiler LINK_FLAGS_RELEASEASSERT)
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
  set_target_properties(compiler PROPERTIES
    LINK_FLAGS "${tgt_link_flags} ${COMPILER_LINK_FLAGS}"
    LINK_FLAGS_RELEASE "${tgt_link_flags_release} ${COMPILER_LINK_FLAGS_RELEASE}"
    LINK_FLAGS_MINSIZEREL "${tgt_link_flags_minsizerel} ${COMPILER_LINK_FLAGS_RELEASE}"
    LINK_FLAGS_RELEASEASSERT "${tgt_link_flags_releaseassert} ${COMPILER_LINK_FLAGS_RELEASE}")

  install(TARGETS compiler
    LIBRARY DESTINATION lib COMPONENT compiler
    RUNTIME DESTINATION bin COMPONENT compiler)

  # We can't test the compiler in offline mode.
  if(CA_ENABLE_TESTS)
    add_subdirectory(test)
    # Close off the active set of 'compiler' lit suites
    ca_umbrella_lit_testsuite_close(compiler)
  endif()
endif()

add_subdirectory(loader)
