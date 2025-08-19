# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#[=======================================================================[.rst:
Target ``mux-api-generate`` is created in this CMake file using
:cmake-command:`add_custom_target` and added as a dependency to our ``mux``
target.

``mux-api-generate`` generates a header in the build directory called
``select.h`` which is used to switch between each of the Mux targets
listed in :cmake:variable:`MUX_TARGET_LIBRARIES` for every Mux entry
point. This model allows abstraction between the APIs we target and the Mux
implementation, as Mux is responsible for plumbing control flow through to the
correct backend.

Generation is performed by a Python script ``generate.py`` based on the
``mux.xml`` XML schema describing the Mux specification.
``update_version.py`` is also run by the ``mux-api-generate`` target to
update the Mux API version in relevant documentation files ``spec.rst`` and
``changes.md`` to match the ``mux.xml`` version.
#]=======================================================================]

option(MUX_API_GENERATE_SELECT
  "Mux: api-generator output dispatch functions." ON)

set(MUX_API_PREFIX mux)
set(MUX_API_GENERATOR
  ${CMAKE_CURRENT_SOURCE_DIR}/../../external/api-generator/generate.py)
set(MUX_API_SCHEMA ${CMAKE_CURRENT_SOURCE_DIR}/mux.xml)
set(MUX_API_HEADER
  ${MUX_SOURCE_DIR}/include/${MUX_API_PREFIX}/${MUX_API_PREFIX}.h)
set(MUX_API_SELECT_HEADER
  ${PROJECT_BINARY_DIR}/include/${MUX_API_PREFIX}/select.h)
set(MUX_API_UPDATE_VERSION
  ${CMAKE_CURRENT_SOURCE_DIR}/update_version.py)
set(MUX_RUNTIME_SPEC ${PROJECT_SOURCE_DIR}/doc/specifications/mux-runtime-spec.rst)
set(MUX_COMPILER_SPEC ${PROJECT_SOURCE_DIR}/doc/specifications/mux-compiler-spec.rst)
set(MUX_CHANGES ${PROJECT_SOURCE_DIR}/doc/modules/mux/changes.rst)

list(LENGTH MUX_TARGET_LIBRARIES MUX_API_TARGETS_LENGTH)
if(${MUX_API_TARGETS_LENGTH} EQUAL 0 )
  message(FATAL_ERROR "Mux: api-generator MUX_TARGET_LIBRARIES is empty!")
elseif(${MUX_API_TARGETS_LENGTH} EQUAL 1)
  set(MUX_API_STUB_NAME single)
elseif(${MUX_API_TARGETS_LENGTH} GREATER 1)
  set(MUX_API_STUB_NAME select)
endif()

if(PythonInterp_FOUND)
  set(MUX_API_DEPENDENCIES ${MUX_API_GENERATOR} ${MUX_API_SCHEMA})

  set(MUX_API_HEADER_COMMAND
    ${PYTHON_EXECUTABLE} ${MUX_API_GENERATOR}
    -f ${MUX_API_PREFIX}
    -p ${MUX_API_PREFIX}
    ${MUX_API_SCHEMA} > ${MUX_API_HEADER})

  set(MUX_API_UPDATE_VERSION_COMMAND
    ${PYTHON_EXECUTABLE} ${MUX_API_UPDATE_VERSION} ${MUX_API_SCHEMA}
    ${MUX_RUNTIME_SPEC} ${MUX_COMPILER_SPEC} ${MUX_CHANGES})

  string(REPLACE ";" "," MUX_TARGET_LIST "${MUX_TARGET_LIBRARIES}")

  set(MUX_API_SELECT_HEADER_COMMAND
    ${PYTHON_EXECUTABLE} ${MUX_API_GENERATOR}
    -f ${MUX_API_PREFIX}
    -p ${MUX_API_PREFIX}
    -s ${MUX_API_STUB_NAME}
    -v targets:${MUX_TARGET_LIST}
    -i mux/config.h
    -g ${MUX_API_SCHEMA} > ${MUX_API_SELECT_HEADER})

  if(TARGET ClangTools::clang-format)
    add_custom_target(mux-api-generate
      COMMAND ${MUX_API_HEADER_COMMAND}
      COMMAND ClangTools::clang-format -i -style=file ${MUX_API_HEADER}
      COMMAND ${MUX_API_UPDATE_VERSION_COMMAND}
      DEPENDS ClangTools::clang-format ${MUX_API_DEPENDENCIES}
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      COMMENT "Generating mux and registered target headers")

    # Ensure all mux target headers are also generated.
    foreach(TARGET ${MUX_TARGET_LIBRARIES})
      add_dependencies(mux-api-generate ${TARGET}-api-generate)
    endforeach()

    # Ensure compatibility layer target header is also generated.
    if(TARGET compat-api-generate)
      add_dependencies(mux-api-generate compat-api-generate)
    endif()

    # Ensure Core target headers are also generated.
    if(TARGET core-api-generate)
      add_dependencies(mux-api-generate core-api-generate)
    endif()

    add_custom_command(
      OUTPUT ${MUX_API_SELECT_HEADER}
      COMMAND ${MUX_API_SELECT_HEADER_COMMAND}
      COMMAND ClangTools::clang-format -i -style=file ${MUX_API_SELECT_HEADER}
      DEPENDS ${MUX_API_DEPENDENCIES}
      COMMENT "Generating mux target selection header")
  else()
    add_custom_command(
      OUTPUT ${MUX_API_SELECT_HEADER}
      COMMAND ${MUX_API_SELECT_HEADER_COMMAND}
      DEPENDS ${MUX_API_DEPENDENCIES}
      COMMENT "Generating mux target selection header")
  endif()

  add_custom_target(mux-api-select-generate DEPENDS
    ${MUX_API_SELECT_HEADER})

  if(TARGET mux-api-generate)
    add_dependencies(mux-api-generate mux-api-select-generate)
  endif()
  add_dependencies(mux mux-api-select-generate)
else()
  message(FATAL_ERROR "Mux: api-generator Python interpreter not found!")
endif()
