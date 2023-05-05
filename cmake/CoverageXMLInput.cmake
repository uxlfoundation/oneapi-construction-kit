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
A `CMake script`_ for generating an XML file to be used to calculate coverage
information. Script should be invoked using ``-P`` as part of an internal
:cmake-variable:`CMAKE_COMMAND`. The following CMake input variables are
required to be set by :doc:`/cmake/Coverage`:

* :cmake:variable:`COVERAGE_FLAGS`
* :cmake:variable:`COVERAGE_SOURCES`
* :cmake:variable:`COVERAGE_OBJECTS`
* :cmake:variable:`COVERAGE_TEST_SUITES`

.. cmake:variable:: COVERAGE_XML_INPUT

  Output variable containing generated XML.

.. _CMake script:
  https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#scripts
#]=======================================================================]

# Open root tag.
file(WRITE ${COVERAGE_XML_INPUT} "<coverage>\n")

# Write option flags.
message(STATUS "Flags: ${COVERAGE_FLAGS}")
foreach(flag ${COVERAGE_FLAGS})
  string(REPLACE " " ";" pair ${flag})
  list(GET pair 0 flag)
  list(GET pair 1 value)
  file(APPEND ${COVERAGE_XML_INPUT} "\
    <property name=\"${flag}\" value=\"${value}\"/>
")
endforeach()

# Get the number of source and objects folders.
list(LENGTH COVERAGE_SOURCES modules_len)
math(EXPR modules_len "${modules_len} - 1")
# Write modules.
foreach(index RANGE ${modules_len})
  list(GET COVERAGE_SOURCES ${index} src)
  list(GET COVERAGE_OBJECTS ${index} obj)

  # Convert src/obj paths from absolute to relative (from build dir)
  # - allows downstream coverage data to be archived/ported/compared.
  file(RELATIVE_PATH src ${${CMAKE_PROJECT_NAME}_BINARY_DIR} ${src})
  file(RELATIVE_PATH obj ${${CMAKE_PROJECT_NAME}_BINARY_DIR} ${obj})

  message(STATUS "Adding relative module paths to ${COVERAGE_XML_INPUT}
    Sources: ${src}
    Objects: ${obj}")

  file(APPEND ${COVERAGE_XML_INPUT} "\
    <module sources=\"${src}\"
            objects=\"${obj}\"/>
")
endforeach()

# Write test suites and their flags.
foreach(test_suite ${COVERAGE_TEST_SUITES})
  list(GET COVERAGE_${test_suite}_COMMAND 0 executable)
  if(REPLACE_${executable})
    # Replace the test suite executable with the path to the target file.
    message(STATUS "TargetFile: ${REPLACE_${executable}}")
    set(executable ${REPLACE_${executable}})
  endif()

  set(flags ${COVERAGE_${test_suite}_COMMAND})
  list(REMOVE_AT flags 0)

  set(environment ${COVERAGE_${test_suite}_ENV})

  # Replace TARGET_FILE generator expressions with the path to the target file.
  foreach(target ${TARGET_FILES})
    string(REPLACE "$<TARGET_FILE:${target}>" "${REPLACE_${target}}"
      flags "${flags}")
    string(REPLACE "$<TARGET_FILE:${target}>" "${REPLACE_${target}}"
      environment "${environment}")
  endforeach()

  message(STATUS "TestSuite: ${test_suite}
    executable: ${executable}
    flags: ${flags}
    env_variables: ${environment}")

  file(APPEND ${COVERAGE_XML_INPUT} "\
    <testsuite  name=\"${test_suite}\"
                path=\"${executable}\"
                flags=\"${flags}\"
                env_variables=\"${environment}\"/>
")
endforeach()

# Close root tag.
file(APPEND ${COVERAGE_XML_INPUT} "</coverage>")
