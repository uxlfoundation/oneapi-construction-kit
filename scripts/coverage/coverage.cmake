# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Find python interpretor 2.7+
find_package(PythonInterp 2.7 REQUIRED)

# Raise an error if the python interpreter has not been found.
if(NOT PythonInterp_FOUND)
  message(FATAL_ERROR "Python interpreter not found!")
endif()

# The XML input file path to pass to the coverage scrpit.
set(COVERAGE_XML_INPUT ${PROJECT_BINARY_DIR}/coverage_input.xml)

# Compilation flags for coverage checking.
set(COVERAGE_COMPILATION_FLAGS
  "--coverage -fno-inline -fno-inline-small-functions -fno-default-inline -O0")

# Linking flags for coverage checking.
set(COVERAGE_LINKING_FLAGS "--coverage")

# Reset modules and test suites variables.
set(COVERAGE_SOURCES CACHE INTERNAL "" FORCE)
set(COVERAGE_OBJECTS CACHE INTERNAL "" FORCE)
set(COVERAGE_TEST_SUITES CACHE INTERNAL "" FORCE)
set(COVERAGE_TEST_SUITES_FLAGS CACHE INTERNAL "" FORCE)

# Path to the entry point of the code coverage checker.
# FIXME: Set the path to the coverage script.
set(COVERAGE_SCRIPT_PATH)

# Flags to pass to the code coverage script.
# Format: [FLAG-NAME] [SPACE] [FLAG-VALUE].
set(COVERAGE_FLAGS
  "csv-output ${PROJECT_BINARY_DIR}/coverage-output.csv"
  "help False"
  "junit-xml-output ${PROJECT_BINARY_DIR}/coverage-output.xml"
  "lcov-html-output True"
  "no-branches False"
  "no-functions False"
  "no-intermediate-files False"
  "no-module-reporting False"
  "no-test-suite-reporting False"
  "output-directory /tmp/"
  "percentage 90"
  "quiet False"
  "recursive True"
  "verbose False")

if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  option(ENABLE_COVERAGE "Enable generation of code coverage data." OFF)

  if(${ENABLE_COVERAGE})
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

# Get modules from their source and object folders.
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

# Get test suites from the path of their executable and their flags.
function(add_coverage_test_suite test_suite flags)
    set(COVERAGE_TEST_SUITES ${COVERAGE_TEST_SUITES} ${test_suite}
        CACHE INTERNAL "" FORCE)
    set(COVERAGE_TEST_SUITES_FLAGS ${COVERAGE_TEST_SUITES_FLAGS} ";"
      ${flags} CACHE INTERNAL "" FORCE)
endfunction()

# Generate the xml input file.
function(edit_coverage_xml_input)
  # Open root tag.
  file(WRITE ${COVERAGE_XML_INPUT} "<coverage>\n")

  # Write option flags.
  foreach(flag_value ${COVERAGE_FLAGS})
    string(REPLACE " " ";" pair ${flag_value})
    list(GET pair 0 flag)
    list(GET pair 1 value)

    file(APPEND ${COVERAGE_XML_INPUT}
      "\t<property name=\"${flag}\" value=\"${value}\"/>\n")
  endforeach()

  # Get the number of source and objects folders.
  list(LENGTH COVERAGE_SOURCES modules_len)
  math(EXPR modules_len "${modules_len} - 1")

  # Get the number of test suite executables.
  list(LENGTH COVERAGE_TEST_SUITES test_suites_len)
  math(EXPR test_suites_len "${test_suites_len} - 1")

  # Write modules.
  foreach(index RANGE ${modules_len})
    list(GET COVERAGE_SOURCES ${index} src)
    list(GET COVERAGE_OBJECTS ${index} obj)

    file(APPEND ${COVERAGE_XML_INPUT}
      "\t<module sources=\"${src}\" objects=\"${obj}\"/>\n")
  endforeach()

  # Write test suites and their flags.
  foreach(index RANGE ${test_suites_len})
    list(GET COVERAGE_TEST_SUITES ${index} path)
    list(GET COVERAGE_TEST_SUITES_FLAGS ${index} flags)

    file(APPEND ${COVERAGE_XML_INPUT}
      "\t<testsuite path=\"${path}\" flags=\"${flags}\"/>\n")
  endforeach()

  # Close root tag.
  file(APPEND ${COVERAGE_XML_INPUT} "</coverage>")
endfunction()

# Create coverage custom command 'make coverage'.
function(add_coverage_custom_target)
  if(${ENABLE_COVERAGE})
    add_custom_target(coverage
      COMMAND ${PYTHON_EXECUTABLE} ${COVERAGE_SCRIPT_PATH} "--xml-input"
      ${COVERAGE_XML_INPUT}
      DEPENDS ${PYTHON_EXECUTABLE} ${COVERAGE_SCRIPT_PATH}
      COMMENT "Code coverage checking started.")
  endif()
endfunction()
