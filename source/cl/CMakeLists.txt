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

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(AddCACL)

# If no library version is given, use the project version
if("${CA_CL_LIBRARY_VERSION}" STREQUAL "")
  set(CA_CL_LIBRARY_VERSION_INTERNAL
    "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
else()
  set(CA_CL_LIBRARY_VERSION_INTERNAL "${CA_CL_LIBRARY_VERSION}")
endif()

set(CA_HOST_ARCH)
get_ca_host_arch(CA_HOST_ARCH)

# Platform informations can be overriden
if("${CA_CL_PLATFORM_VERSION}" STREQUAL "")
  # We support OpenCL (1.2|3.0), we don't print this here since we append more
  # information for non-release builds.
  string(CONCAT CA_CL_PLATFORM_VERSION_STRING
    "OpenCL ${CA_CL_PLATFORM_VERSION_MAJOR}.${CA_CL_PLATFORM_VERSION_MINOR} ComputeAorta ${PROJECT_VERSION} "
    "${CMAKE_SYSTEM_NAME} ${CA_HOST_ARCH}")
  set(CA_CL_PLATFORM_VERSION ${CA_CL_PLATFORM_VERSION_STRING})
  # Version strings for non-release builds will have their build type and
  # shortened git commit hash in the version string.
  string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UPPER)
  if(NOT ${BUILD_TYPE_UPPER} MATCHES "RELEASE")
    string(APPEND CA_CL_PLATFORM_VERSION
      " (${CMAKE_BUILD_TYPE}, ${CA_GIT_COMMIT})")
  endif()
endif()
message(STATUS "OpenCL platform version: ${CA_CL_PLATFORM_VERSION}")

# Platform informations can be overriden
if("${CA_CL_PLATFORM_NAME}" STREQUAL "")
  set(CA_CL_PLATFORM_NAME "ComputeAorta")
endif()
message(STATUS "OpenCL platform name: ${CA_CL_PLATFORM_NAME}")

# Platform informations can be overriden
if("${CA_CL_PLATFORM_VENDOR}" STREQUAL "")
  set(CA_CL_PLATFORM_VENDOR "Codeplay Software Ltd.")
endif()
message(STATUS "OpenCL platform vendor: ${CA_CL_PLATFORM_VENDOR}")

# Device information can be overriden
if("${CA_CL_DEVICE_VENDOR}" STREQUAL "")
  set(CA_CL_DEVICE_VENDOR "${CA_CL_PLATFORM_VENDOR}")
endif()
message(STATUS "OpenCL device vendor: ${CA_CL_DEVICE_VENDOR}")

# Driver information can be overriden
if("${CA_CL_DRIVER_VERSION}" STREQUAL "")
  set(CA_CL_DRIVER_VERSION "${PROJECT_VERSION}")
endif()
message(STATUS "OpenCL driver version: ${CA_CL_DRIVER_VERSION}")

# Device information can be overriden
if("${CA_CL_DEVICE_VERSION}" STREQUAL "")
  # The device version is OpenCL (1.2|3.0) based, we don't print this here
  # since the LLVM version is appended below.
  set(CA_CL_DEVICE_VERSION "OpenCL ${CA_CL_PLATFORM_VERSION_MAJOR}.${CA_CL_PLATFORM_VERSION_MINOR} ")
  string(APPEND CA_CL_DEVICE_VERSION "ComputeAorta ${PROJECT_VERSION}")
endif()
message(STATUS "OpenCL device version: ${CA_CL_DEVICE_VERSION}")

# Device information can be overriden
set(CA_CL_DEVICE_OPENCL_C_VERSION "OpenCL C 1.2 Clang ${LLVM_PACKAGE_VERSION}")
message(STATUS "OpenCL C version: ${CA_CL_DEVICE_OPENCL_C_VERSION}")

set(CL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/OpenCL-Headers
  CACHE PATH "OpenCL header include directory" FORCE)

# If the online coverage is enabled we add the modules so that the XML file
# can be generated automatically.
if(${CA_ENABLE_COVERAGE} AND ${CA_RUNTIME_COMPILER_ENABLED})
  add_coverage_modules(${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/CL.dir/source
    ${CMAKE_CURRENT_SOURCE_DIR}/external/OpenCL-Headers/CL
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/CL.dir/source)
endif()

add_subdirectory(external)
add_subdirectory(source/binary)
add_subdirectory(source/extension)

# Generate config header containing all compile-time configuration macros.
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/cl/config.h)

set(CL_SOURCE_FILES
  ${CMAKE_CURRENT_BINARY_DIR}/include/cl/config.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/base.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/command_queue.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/context.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/event.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/image.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/kernel.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/printf.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/limits.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/macros.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/mem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/mux.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/platform.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/program.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/sampler.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/semaphore.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/validate.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/base.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/command_queue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/event.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/exports.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/image.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/kernel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/printf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/mem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/program.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/semaphore.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/source/sampler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/validate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/opencl-3.0.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opencl-3.0.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/exports-3.0.cpp)

add_ca_library(CL SHARED
  ${CL_SOURCE_FILES} $<$<PLATFORM_ID:Windows>:${BUILTINS_RC_FILE}>)
add_ca_library(CL-static STATIC EXCLUDE_FROM_ALL ${CL_SOURCE_FILES})

if(NOT CA_COMPILER_ENABLE_DYNAMIC_LOADER)
  target_resources(CL NAMESPACES ${BUILTINS_NAMESPACES})
endif()

# Clear link flags to avoid duplicates in rebuild configs.
set(CL_LINK_FLAGS)
set(CL_LINK_FLAGS_RELEASE)

if(CA_PLATFORM_LINUX OR CA_PLATFORM_ANDROID OR
    (CA_PLATFORM_WINDOWS AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR
    CA_PLATFORM_QNX)
  # Set the OpenCL API entry point version map.
  string(APPEND CL_LINK_FLAGS " -Xlinker \
    --version-script=${CMAKE_CURRENT_SOURCE_DIR}/source/export-linux.map")
  # Don't specify the retain symbols file when debug backtrace is enabled
  # otherwise DEBUG_BACKTRACE will not output internal symbols correctly, or
  # when using sanitizers otherwise the quality of error messages will be
  # reduced and suppression filters may not work.
  # Debugging llvm becomes very frustrating without the `dump` methods which are
  # otherwise stripped. Don't strip public symbols for any *full* debug builds
  if(NOT CMAKE_BUILD_TYPE STREQUAL Debug)
    if(NOT CA_ENABLE_DEBUG_BACKTRACE AND NOT CA_USE_SANITIZER AND NOT
        CA_PLATFORM_QNX)
      # Export API entry point symbols for Linux, or if we're using a GNU
      # compiler on Windows, i.e. a MinGW build on Windows uses the Linux symbol
      # exports, not the Windows ones.
      set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
      string(APPEND CL_LINK_FLAGS " -Xlinker --retain-symbols-file \
        ${CMAKE_CURRENT_SOURCE_DIR}/source/export-linux.sym")
    endif()
  endif()

  # Delete all local symbols.
  string(APPEND CL_LINK_FLAGS_RELEASE " -Xlinker --discard-all")
  # Enable garbage collection of unused input sections.
  string(APPEND CL_LINK_FLAGS_RELEASE " -Xlinker --gc-sections")
  # Sort the common symbols by size when placing them in the appropriate output
  # sections.
  string(APPEND CL_LINK_FLAGS_RELEASE " -Xlinker --sort-common")

  if(NOT CA_PLATFORM_ANDROID)
    # Strip any remaining debug info that may be there from customer static
    # libraries.
    string(APPEND CL_LINK_FLAGS_RELEASE " -Xlinker --strip-debug")
  endif()

  if(CA_USE_LINKER STREQUAL gold)
    # Identical Code Folding, number of iterations of ICF (default 2).
    string(APPEND CL_LINK_FLAGS_RELEASE
      " -Xlinker --icf=all -Xlinker --icf-iterations -Xlinker 5")
  endif()
elseif(CA_PLATFORM_WINDOWS AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Detect whether or not LLVMExecutionEngine is linked into the target.
  # This knowledge is needed to export JIT debug symbols, which will
  # cause an unresolved error if the library is not linked.
  get_target_link_libraries(CL_LINK_LIBRARIES CL RECURSIVE)
  foreach(library ${CL_LINK_LIBRARIES})
    if("${library}" MATCHES "LLVMExecutionEngine")
      set(LLVM_DEBUG_REGISTER_CODE_SYM "__jit_debug_register_code")
      set(LLVM_DEBUG_DESCRIPTOR_SYM "__jit_debug_descriptor")
      break()
    endif()
  endforeach()

  # In order to avoid unnessesary duplication of .def files depending on the
  # OPENCL_LIBRARY_NAME name we use CMake to configure these files before use.
  set(CA_CL_LIBRARY_DEF_FILE "export-windows.def")
  set(ExportLibraryName ${CA_CL_LIBRARY_NAME})
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/source/${CA_CL_LIBRARY_DEF_FILE}.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/source/${CA_CL_LIBRARY_DEF_FILE})
  # Lastly tell the linker to use the configured .def file
  string(APPEND CL_LINK_FLAGS
    " /DEF:${CMAKE_CURRENT_BINARY_DIR}/source/${CA_CL_LIBRARY_DEF_FILE}")
  string(APPEND CL_LINK_FLAGS_RELEASE " /OPT:REF /OPT:ICF=5")
else()
  message(WARNING "Unhandled build environment.")
endif()

# Use the specified output name and set the version of the OpenCL library and
# set the platform specific link flags.
message(STATUS "OpenCL library output name: ${CA_CL_LIBRARY_NAME}")
message(STATUS "OpenCL library version: ${CA_CL_LIBRARY_VERSION_INTERNAL}")
# Extract the library major version.
string(REGEX REPLACE "([0-9]+)\\.[0-9]+" "\\1"
  CA_CL_LIBRARY_VERSION_MAJOR ${CA_CL_LIBRARY_VERSION_INTERNAL})

foreach(CL_lib CL CL-static)
  # Add the OpenCL version to the compile definitions so we can
  # conditionally enable/disable features between versions.
  target_compile_definitions(${CL_lib} PUBLIC ${CA_CL_COMPILE_DEFINITIONS})

  # clCreateCommandQueue and clCreateSampler were deprecated in OpenCL-3.0.
  target_compile_options(${CL_lib} PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:/wd4996>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
      -Wno-deprecated-declarations>
  )

  target_include_directories(${CL_lib} SYSTEM PUBLIC ${CL_INCLUDE_DIR})
  target_include_directories(${CL_lib} PUBLIC ${CA_CL_EXTENSION_INCLUDE_DIRS}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include)

  target_link_libraries(${CL_lib}
    PUBLIC Threads::Threads
    # Optional extra library dependencies set for customer devices
    ${CA_CL_PUBLIC_LINK_LIBRARIES}
    # Link against version (for clang) on Windows.
    $<$<BOOL:${WIN32}>:version>
    # Link against libm.so on UNIX/Android/MinGW
    $<$<OR:$<BOOL:${UNIX}>,$<BOOL:${ANDROID}>,$<BOOL:${MINGW}>>:m>
    PRIVATE extension compiler-loader CL-binary mux)

  # Ensure we're not overwriting existing link options
  get_target_property(tgt_link_flags ${CL_lib} LINK_FLAGS)
  get_target_property(tgt_link_flags_release ${CL_lib} LINK_FLAGS_RELEASE)
  get_target_property(tgt_link_flags_minsizerel ${CL_lib} LINK_FLAGS_MINSIZEREL)
  get_target_property(
    tgt_link_flags_releaseassert ${CL_lib} LINK_FLAGS_RELEASEASSERT)
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

  set_target_properties(${CL_lib} PROPERTIES OUTPUT_NAME ${CA_CL_LIBRARY_NAME}
    SOVERSION ${CA_CL_LIBRARY_VERSION_MAJOR}
    VERSION ${CA_CL_LIBRARY_VERSION_INTERNAL}
    LINK_FLAGS "${tgt_link_flags} ${CL_LINK_FLAGS}"
    LINK_FLAGS_RELEASE "${tgt_link_flags_release} ${CL_LINK_FLAGS_RELEASE}"
    LINK_FLAGS_MINSIZEREL
      "${tgt_link_flags_minsizerel} ${CL_LINK_FLAGS_RELEASE}"
    LINK_FLAGS_RELEASEASSERT
      "${tgt_link_flags_releaseassert} ${CL_LINK_FLAGS_RELEASE}")
endforeach()

# MSVC already creates a file called ${CA_CL_LIBRARY_NAME}.lib for the shared
# target. This isn't the same as a static version of the library, but it does
# break if we don't rename the proper static version to something else.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set_target_properties(CL-static PROPERTIES
    OUTPUT_NAME "${CA_CL_LIBRARY_NAME}_static")
endif()

set_target_properties(CL PROPERTIES
  ENABLE_ICD_LOADER ${CA_CL_ENABLE_ICD_LOADER})
add_ca_cl_icd_file(CL NAME ${CA_CL_LIBRARY_NAME})
if(CA_CL_ENABLE_ICD_LOADER)
  add_dependencies(OpenCL CL)
endif()

# Add an indirect dependency on the compiler shared library. Adding compiler's
# LIBRARY_OUTPUT_DIRECTORY as a target_link_directory for CL means that CL's
# rpath is set up correctly on Unix platforms to discover the compiler library.
if(CA_RUNTIME_COMPILER_ENABLED AND CA_COMPILER_ENABLE_DYNAMIC_LOADER)
  add_dependencies(CL compiler)
  get_target_property(
    COMPILER_LIBRARY_OUTPUT_DIRECTORY
    compiler
    LIBRARY_OUTPUT_DIRECTORY)
  set_target_properties(CL PROPERTIES
    LINK_DIRECTORIES ${COMPILER_LIBRARY_OUTPUT_DIRECTORY}
    INTERFACE_LINK_DIRECTORIES ${COMPILER_LIBRARY_OUTPUT_DIRECTORY}
    BUILD_RPATH ${COMPILER_LIBRARY_OUTPUT_DIRECTORY})
endif()

if(TARGET tidy-CL)
  add_dependencies(tidy-CL abacus_generate mux-config)
endif()

install(TARGETS CL
  ARCHIVE DESTINATION lib COMPONENT OCL
  LIBRARY DESTINATION lib COMPONENT OCL
  RUNTIME DESTINATION bin COMPONENT OCL)

# Install all the OpenCL headers required by the registry.  This means that we
# install headers for extensions that we do not support (e.g. graphics
# interop).  We do this to (a) follow the registry instructions, and (b) to
# allow the legal program that checks for an extension at run time (and thus
# needs to know names at compile time).
install(DIRECTORY ${CL_INCLUDE_DIR}/CL DESTINATION include COMPONENT OCL)

add_subdirectory(tools)
if(CA_ENABLE_EXAMPLES)
  add_subdirectory(examples)
endif()

# Build the test suites.
if(CA_ENABLE_TESTS)
  add_subdirectory(test)

  # Install city_runner and the csv files controlling test execution.
  install(PROGRAMS
    ${PROJECT_SOURCE_DIR}/scripts/testing/run_cities.py
    DESTINATION bin COMPONENT OCLTest)
  install(DIRECTORY
    ${PROJECT_SOURCE_DIR}/scripts/testing/city_runner
    DESTINATION bin COMPONENT OCLTest
    FILES_MATCHING PATTERN *.py PATTERN __pycache__ EXCLUDE)
  install(DIRECTORY
    ${PROJECT_SOURCE_DIR}/scripts/jenkins/cts_summary
    DESTINATION share COMPONENT OCLTest
    FILES_MATCHING PATTERN *.csv)
endif()
