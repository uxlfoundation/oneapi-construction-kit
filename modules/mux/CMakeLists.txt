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
.. cmake:variable:: CA_MUX_ENABLE_SHARED

  Specifies whether mux should be built as a shared or static library. Defaults
  to building mux as a static library.
#]=======================================================================]
ca_option(CA_MUX_ENABLE_SHARED BOOL
  "Enable building mux as a shared library" OFF)

set(MUX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# If the online coverage is enabled we add the modules so that the XML file
# can be generated automatically.
if(${CA_ENABLE_COVERAGE} AND ${CA_RUNTIME_COMPILER_ENABLED})
  add_coverage_modules(${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/mux.dir/source
    ${CMAKE_BINARY_DIR}/include/mux
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/mux.dir/source
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mux
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/mux.dir/source)
endif()

set(MUX_SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/mux/mux.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/mux/mux.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/command_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/config.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/executable.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/fence.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/image.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/kernel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/memory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/query_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/queue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/semaphore.cpp)

if(CA_MUX_ENABLE_SHARED)
  add_ca_library(mux SHARED ${MUX_SOURCE_FILES})
  install(TARGETS mux LIBRARY DESTINATION lib COMPONENT Mux)
else()
  add_ca_library(mux STATIC ${MUX_SOURCE_FILES})
endif()

target_include_directories(mux PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include/mux)
set(MUX_CONFIG_HEADER ${PROJECT_BINARY_DIR}/include/mux/config.h)

if(CA_ENABLE_TESTS)
  # Open up an active set of 'mux' lit suites
  ca_umbrella_lit_testsuite_open(mux)
endif()

# Add the source directory to include all enabled mux targets.
set(MUX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(external)
add_subdirectory(utils)
add_subdirectory(source)
add_subdirectory(targets)

# Any built mux targets should be enabled by default
# This can be limited with CA_MUX_COMPILERS_TO_ENABLE which
# will dictate whether to build in that directory.
set(MUX_TARGET_LIBRARIES ${MUX_TARGET_LIBRARIES}
  CACHE INTERNAL "List of mux target libraries.")

# Validate the MUX_TARGET_LIBRARIES variable actually contains a target.
list(LENGTH MUX_TARGET_LIBRARIES MUX_TARGET_COUNT)
if(0 EQUAL ${MUX_TARGET_COUNT})
  message(FATAL_ERROR "Mux: No targets enabled, append your target library "
    "to the MUX_TARGET_LIBRARIES list.")
else()
  message(STATUS "Mux target libraries: ${MUX_TARGET_LIBRARIES}")
endif()

add_subdirectory(tools)
# test's are added in `modules/CMakeLists.txt`

# Due to CMake splitting the MUX_TARGET_LIBRARIES list when passing it to the
# custom command it is not passed through correctly, replacing ';' with ',' and
# doing the opposite on the other side works around this issue.
string(REPLACE ";" "," MUX_TARGET_LIST "${MUX_TARGET_LIBRARIES}")

add_custom_command(OUTPUT ${MUX_CONFIG_HEADER}
  COMMAND ${CMAKE_COMMAND}
  -DMUX_CONFIG_HEADER=${MUX_CONFIG_HEADER}
  -DMUX_TARGET_LIBRARIES=${MUX_TARGET_LIST}
  -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/mux-config.cmake
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/mux-config.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_custom_target(mux-config DEPENDS ${MUX_CONFIG_HEADER})
add_dependencies(mux mux-config)

if(TARGET tidy-mux)
  add_dependencies(tidy-mux mux-config mux-api-select-generate)
endif()

# Mux targets depend on each other's headers through include/mux/config.h.
# Break the cycling dependency between mux and the mux targets by creating
# a CMake target with all the relevant include directories and definitions.
add_library(mux-headers INTERFACE)
get_target_property(mux_target_include_dirs mux INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(mux-headers INTERFACE "${mux_target_include_dirs}")
foreach(MuxTarget ${MUX_TARGET_LIBRARIES})
  get_target_property(mux_target_include_dirs ${MuxTarget} INTERFACE_INCLUDE_DIRECTORIES)
  target_include_directories(mux-headers INTERFACE "${mux_target_include_dirs}")
  get_target_property(mux_target_definitions ${MuxTarget} INTERFACE_COMPILE_DEFINITIONS)
  if(mux_target_definitions)
    target_compile_definitions(mux-headers INTERFACE ${mux_target_definitions})
  endif()
endforeach()

# Link all target libraries into mux.
foreach(MuxTarget ${MUX_TARGET_LIBRARIES})
  target_link_libraries(mux PUBLIC ${MuxTarget})
  add_dependencies(${MuxTarget} mux-config)
endforeach()

target_link_libraries(mux PUBLIC cargo mux-utils tracer)

# Append to the list of module libraries, the cache MUST be updated.
list(APPEND MODULES_LIBRARIES mux)
set(MODULES_LIBRARIES ${MODULES_LIBRARIES}
  CACHE INTERNAL "List of module libraries.")

# For online coverage we want to add UnitMux as a test suite
if(${CA_ENABLE_COVERAGE} AND ${CA_RUNTIME_COMPILER_ENABLED})
  # TODO: Reenable when it is working on Jenkins
  # add_coverage_test_suite(${PROJECT_BINARY_DIR}/bin/UnitMux " " " " UnitMux)
endif()

if(CA_ENABLE_TESTS)
  # Close off the active set of 'mux' lit suites
  ca_umbrella_lit_testsuite_close(mux)
endif()
