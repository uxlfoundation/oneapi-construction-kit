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
Module providing support for `lcov`_,  a graphical front-end for GCC's coverage
testing tool `gcov`_.

To access the commands and variables in this module:

.. code:: cmake

  include(Coverage)

.. _lcov:
  http://ltp.sourceforge.net/coverage/lcov.php
.. _gcov:
  https://gcc.gnu.org/onlinedocs/gcc/Gcov.html
#]=======================================================================]

#[=======================================================================[.rst:
.. cmake:variable:: COVERAGE_XML_INPUT

  The XML input file path to pass to the coverage script.
#]=======================================================================]
set(COVERAGE_XML_INPUT ${PROJECT_BINARY_DIR}/coverage_input.xml)

#[=======================================================================[.rst:
.. cmake:variable:: COVERAGE_COMPILATION_FLAGS

  Compilation flags for coverage checking.
#]=======================================================================]
set(COVERAGE_COMPILATION_FLAGS
  " --coverage -fno-inline -fno-inline-small-functions -fno-default-inline -O0")

#[=======================================================================[.rst:
.. cmake:variable:: COVERAGE_LINKING_FLAGS

  Linking flags for coverage checking.
#]=======================================================================]
set(COVERAGE_LINKING_FLAGS "--coverage")

#[=======================================================================[.rst:
.. cmake:variable:: COVERAGE_SOURCES

  List of source file directories to include in coverage, populated by
  function :cmake:command:`add_coverage_modules`.
#]=======================================================================]
set(COVERAGE_SOURCES CACHE INTERNAL "" FORCE)

#[=======================================================================[.rst:
.. cmake:variable:: COVERAGE_OBJECTS

  List containing an object file directory associated to each source directory
  in :cmake:variable:`COVERAGE_SOURCES`, populated by function
  :cmake:command:`add_coverage_modules`.
#]=======================================================================]
set(COVERAGE_OBJECTS CACHE INTERNAL "" FORCE)

#[=======================================================================[.rst:
.. cmake:variable:: COVERAGE_TEST_SUITES

  List of test suite CMake targets to generate coverage information for,
  populated by :cmake:command:`add_coverage_test_suite`.
#]=======================================================================]
set(COVERAGE_TEST_SUITES CACHE INTERNAL "" FORCE)

#[=======================================================================[.rst:
.. cmake:variable:: COVERAGE_SCRIPT_PATH

  Path to the entry point of the code coverage checker Python script.
#]=======================================================================]
set(COVERAGE_SCRIPT_PATH ${PROJECT_SOURCE_DIR}/scripts/coverage/coverage.py)

#[=======================================================================[.rst:
.. cmake:variable:: COVERAGE_FLAGS

  Flags to pass to the code coverage script, following format
  '``[FLAG-NAME] [SPACE] [FLAG-VALUE]``'.
#]=======================================================================]
set(COVERAGE_FLAGS
  "run-lcov False"
  "help False"
  "lcov-html-output False"
  "no-branches True"
  "no-functions True"
  "no-intermediate-files True"
  "no-module-reporting True"
  "no-test-suite-reporting True"
  "output-directory /tmp/"
  "percentage 90"
  "quiet False"
  "recursive True"
  "verbose False")

if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  option(CA_ENABLE_COVERAGE "Enable generation of code coverage data." OFF)

  if(${CA_ENABLE_COVERAGE})
    # Setup global gcov compiler flags.
    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${COVERAGE_COMPILATION_FLAGS})
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${COVERAGE_COMPILATION_FLAGS})

    # Setup global gcov linker flags.
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS}
      ${COVERAGE_LINKING_FLAGS})
    set(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS}
      ${COVERAGE_LINKING_FLAGS})
    set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS}
      ${COVERAGE_LINKING_FLAGS})
  endif()
endif()

#[=======================================================================[.rst:
.. cmake:command:: add_coverage_modules

  This function adds '``source directory, object directory``' pairs to the list
  of modules to target. It is invoked by the ``CMakeLists.txt`` files of
  individual ComputeAorta modules which have this information available.

  Arguments:
    * ``ARGV`` - Even index arguments contain source directories, and are
      appended to :cmake:variable:`COVERAGE_SOURCES`. Odd indexed arguments
      contain directory names for the object files, and are appended to
      :cmake:variable:`COVERAGE_OBJECTS`.
#]=======================================================================]
function(add_coverage_modules sources objects ...)
  list(LENGTH ARGV len)
  math(EXPR len "${len} - 1")

  foreach(index RANGE ${len})
    math(EXPR modulo "${index} % 2")
    if(${modulo} EQUAL 0)
      set(COVERAGE_SOURCES ${COVERAGE_SOURCES} ${ARGV${index}}
        CACHE INTERNAL "" FORCE)
    else()
      set(COVERAGE_OBJECTS ${COVERAGE_OBJECTS} ${ARGV${index}}
        CACHE INTERNAL "" FORCE)
    endif()
  endforeach()
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_coverage_test_suite

  Function adding a test suite and flags to the list of test suites to be
  run by the script.

  Arguments:
    * ``test_suite`` - Target name of test suite, appended to
      :cmake:variable:`COVERAGE_TEST_SUITES`.

  Keyword Arguments:
    * ``COMMAND`` - Command to execute test suite.
    * ``ENVIRONMENT`` -  Environment variables of the form "VAR=<value>".
#]=======================================================================]
function(add_coverage_test_suite test_suite)
  cmake_parse_arguments(args "" "" "COMMAND;ENVIRONMENT" ${ARGN})
  set(COVERAGE_TEST_SUITES ${COVERAGE_TEST_SUITES} ${test_suite}
    CACHE INTERNAL "Coverage: List of enabled test suites." FORCE)
  set(COVERAGE_${test_suite}_COMMAND ${args_COMMAND}
    CACHE INTERNAL "Coverage: ${test_suite} command." FORCE)
  set(COVERAGE_${test_suite}_ENV ${args_ENVIRONMENT}
    CACHE INTERNAL "Coverage: ${test_suite} environment." FORCE)
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_coverage_xml_input

  Creates a CMake target ``coverage_input`` for generating the XML input file
  to gathering coverage information on test suite targets in
  :cmake:variable:`COVERAGE_TEST_SUITES`.
#]=======================================================================]
function(add_coverage_xml_input)
  # Construct a list of definitions for commands which begin with a target name
  # to be replaced with the target files absolute path.
  set(TargetFileReplacements)
  foreach(test_suite ${COVERAGE_TEST_SUITES})
    set(command ${COVERAGE_${test_suite}_COMMAND})
    list(GET command 0 target)
    if(TARGET ${target})
      list(APPEND TargetFileReplacements
        "-DREPLACE_${target}=$<TARGET_FILE:${target}>")
    endif()
  endforeach()

  # Construct a list of TARGET_FILE generator expression replacements to be
  # replaced with the path to the target file.
  set(TargetFiles)
  foreach(test_suite ${COVERAGE_TEST_SUITES})
    string(REGEX MATCHALL "\\$<TARGET_FILE:.+>" targetFiles
      ${COVERAGE_${test_suite}_COMMAND} ${COVERAGE_${test_suite}_ENV})
    foreach(targetFile ${targetFiles})
      string(REPLACE "$<TARGET_FILE:" "" target ${targetFile})
      string(REPLACE ">" "" target ${target})
      list(APPEND TargetFiles ${target})
      list(APPEND TargetFileReplacements "-DREPLACE_${target}=${targetFile}")
    endforeach()
  endforeach()

  if(TargetFileReplacements)
    list(REMOVE_DUPLICATES TargetFileReplacements)
  endif()
  if(TargetFiles)
    list(REMOVE_DUPLICATES TargetFiles)
  endif()

  # Custom command to generate the XML input for the coverage scripts, this
  # resolves the $<TARGET_FILE:${target}> generator expressions in the
  # replacements list.
  add_custom_command(OUTPUT ${COVERAGE_XML_INPUT}
    COMMAND ${CMAKE_COMMAND} ${PROJECT_BINARY_DIR}
      -DTARGET_FILES=${TargetFiles} ${TargetFileReplacements}
      -DCOVERAGE_XML_INPUT=${COVERAGE_XML_INPUT}
      -P ${PROJECT_SOURCE_DIR}/cmake/CoverageXMLInput.cmake
    DEPENDS ${PROJECT_SOURCE_DIR}/cmake/CoverageXMLInput.cmake
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generate Coverage XML")

  # Named target to generate the XML input file.
  add_custom_target(coverage_input ALL
    DEPENDS ${PROJECT_BINARY_DIR}/coverage_input.xml
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Generate Coverage XML")

  # Ensure the coverage file is generated with the ComputeAorta target.
  add_dependencies(ComputeAorta coverage_input)
endfunction()

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/lcov)

#[=======================================================================[.rst:
.. cmake:command:: add_coverage_custom_target

  When the ``CA_ENABLE_COVERAGE`` variable is set, this function uses CMake
  :cmake-command:`add_custom_target` to add the following commands:

  ``move_files``
    Copies the generated files ``coreConfig.h`` and ``coreSelect.h`` from the
    build directory to source directory so they can be included in coverage
    metrics.

  ``remove_files``
    Deletes files ``coreConfig.h`` and ``coreSelect.h`` from the source directory.

  ``coverage``
    Copies ``coreConfig.h`` and ``coreSelect.h`` from the build directory to
    source directory. Runs the coverage script
    :cmake:variable:`COVERAGE_SCRIPT_PATH`, then deletes the copied
    ``coreConfig.h`` and ``coreSelect.h`` files from the source directory.

  To start code coverage analysis a user should run the ``coverage`` target:

  .. code:: console

    $ ninja coverage
#]=======================================================================]
function(add_coverage_custom_target)
  if(CA_ENABLE_COVERAGE)
    add_custom_target(move_files
      COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_BINARY_DIR}/include/core/coreConfig.h
      ${CMAKE_CURRENT_SOURCE_DIR}/modules/core/include/core/coreConfig.h
      COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_BINARY_DIR}/include/core/coreSelect.h
      ${CMAKE_CURRENT_SOURCE_DIR}/modules/core/include/core/coreSelect.h
      COMMENT "Coverage Moving Files.")
    add_custom_target(remove_files
      COMMAND ${CMAKE_COMMAND} -E remove
      ${CMAKE_CURRENT_SOURCE_DIR}/modules/core/include/core/coreSelect.h
      ${CMAKE_CURRENT_SOURCE_DIR}/modules/core/include/core/coreConfig.h
      COMMENT "Coverage Removing Files.")

    add_custom_target(coverage
      COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_BINARY_DIR}/include/core/coreConfig.h
      ${CMAKE_CURRENT_SOURCE_DIR}/modules/core/include/core/coreConfig.h
      COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_BINARY_DIR}/include/core/coreSelect.h
      ${CMAKE_CURRENT_SOURCE_DIR}/modules/core/include/core/coreSelect.h
      COMMAND ${Python3_EXECUTABLE} ${COVERAGE_SCRIPT_PATH} "--xml-input"
      ${COVERAGE_XML_INPUT} "-o" "${PROJECT_BINARY_DIR}/lcov" "-p" "90"
      COMMAND ${CMAKE_COMMAND} -E remove
      ${CMAKE_CURRENT_SOURCE_DIR}/modules/core/include/core/coreSelect.h
      ${CMAKE_CURRENT_SOURCE_DIR}/modules/core/include/core/coreConfig.h
      DEPENDS ${Python3_EXECUTABLE} ${COVERAGE_SCRIPT_PATH}
      COMMENT "Coverage checking started.")
  endif()
endfunction()
