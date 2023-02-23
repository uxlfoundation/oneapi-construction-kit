# This file is distributed under the University of Illinois Open Source License

#[=======================================================================[.rst:
A `CMake script`_ taking a file and embedding it in a C header with a given
variable name, to be invoked using ``-P`` as part of an internal
:cmake-variable:`CMAKE_COMMAND`.

.. cmake:variable:: BIN2H_INPUT_FILE

  The binary input filepath to be made into a header file

.. cmake:variable:: BIN2H_OUTUPUT_FILE

  The header filepath to generate

.. cmake:variable:: BIN2H_VARIABLE_NAME

  The name of the variable to access the data in the header file

.. _CMake script:
  https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#scripts
#]=======================================================================]

if(NOT DEFINED BIN2H_INPUT_FILE)
  message(FATAL_ERROR
    "Required cmake variable BIN2H_INPUT_FILE not set!")
endif()

if(NOT DEFINED BIN2H_OUTPUT_FILE)
  message(FATAL_ERROR
    "Required cmake variable BIN2H_OUTPUT_FILE not set!")
endif()

if(NOT DEFINED BIN2H_VARIABLE_NAME)
  message(FATAL_ERROR
    "Required cmake variable BIN2H_VARIABLE_NAME not set!")
endif()

if(NOT EXISTS ${BIN2H_INPUT_FILE})
  message(FATAL_ERROR "File '${BIN2H_INPUT_FILE}' does not exist!")
endif()

file(READ "${BIN2H_INPUT_FILE}" binary HEX)
string(REGEX MATCHALL ".." binary "${binary}")
string(REGEX REPLACE ";" "',\n  '\\\\x" binary "${binary}")

get_filename_component(IncludeGuard ${BIN2H_OUTPUT_FILE} NAME_WE)
string(TOUPPER "${IncludeGuard}" IncludeGuard)
string(REGEX REPLACE "[^A-Z|0-9]" "_" IncludeGuard "${IncludeGuard}")
set(IncludeGuard "${IncludeGuard}_INCLUDED")

file(WRITE "${BIN2H_OUTPUT_FILE}"
  "// Copyright (C) Codeplay Software Limited. All Rights Reserved.\n"
  "\n"
  "#ifndef ${IncludeGuard}\n"
  "#define ${IncludeGuard}\n"
  "\n"
  "#include <stddef.h>\n"
  "\n"
  "#ifdef __cplusplus\n"
  "extern \"C\" {\n"
  "#endif\n"
  "\n"
  "const char "
  "${BIN2H_VARIABLE_NAME}[] = {\n"
  "  '\\x${binary}'\n};\n"
  "\n"
  "const size_t ${BIN2H_VARIABLE_NAME}_size = sizeof(${BIN2H_VARIABLE_NAME});\n"
  "\n"
  "#ifdef __cplusplus\n"
  "}\n"
  "\n"
  "#endif\n"
  "\n"
  "#endif  // ${IncludeGuard}\n"
  "\n")
