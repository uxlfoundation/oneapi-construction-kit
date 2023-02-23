# Copyright (C) Codeplay Software Limited. All Rights Reserved.

#[=======================================================================[.rst:
Utilities to convert binary files to header files at build time using only
CMake in script mode. To use these utilities:

.. code:: cmake

  include(Bin2H)

  hal_add_bin2h_command(my_bin
    ${path_to_input}/file.bin
    ${path_to_output}/file.h)

  hal_add_bin2h_target(my_bin
    ${path_to_input}/file.bin
    ${path_to_output}/file.h)
#]=======================================================================]

#[=======================================================================[.rst:
.. cmake:command:: hal_add_bin2h_command

  The ``hal_add_bin2h_command`` macro creates a custom command which generates a
  header file from a binary file that can be included into source code.
  Additionally the macro specifies that the header file is generated at build
  time, this allows it to be used as a source file for libraries and
  exectuables.

  Arguments:
    * ``variable``: The name of the variable to access the data in the header
      file
    * ``input``: The binary input filepath to be made into a header file
    * ``output``: The header filepath to generate
#]=======================================================================]
function(hal_add_bin2h_command variable input output)
  get_property(HAL_SOURCE_DIR GLOBAL PROPERTY HAL_SOURCE_DIR)
  file(RELATIVE_PATH relOut ${CMAKE_BINARY_DIR} ${output})
  add_custom_command(OUTPUT ${output}
    COMMAND ${CMAKE_COMMAND}
      -DBIN2H_INPUT_FILE:FILEPATH="${input}"
      -DBIN2H_OUTPUT_FILE:FILEPATH="${output}"
      -DBIN2H_VARIABLE_NAME:STRING="${variable}"
      -P ${HAL_SOURCE_DIR}/cmake/HALBin2HScript.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${input} ${HAL_SOURCE_DIR}/cmake/HALBin2HScript.cmake
    COMMENT "Generating H file ${relOut}")
  set_source_files_properties(${output} PROPERTIES GENERATED ON)
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: hal_add_bin2h_target

  The ``hal_add_bin2h_target`` macro creates a custom target which generates a
  header file from a binary file that can be included into source code and use
  the target in CMake dependency tracking.

  Arguments:
    * ``target``: The name of the target and the name of the variable to access
      the data in the header file
    * ``input``: The binary input filepath to be made into a header file
    * ``output``: The header filepath to generate
#]=======================================================================]
function(hal_add_bin2h_target target input output)
  hal_add_bin2h_command(${target} ${input} ${output})
  file(RELATIVE_PATH relOut ${CMAKE_BINARY_DIR} ${output})
  add_custom_target(${target}
    DEPENDS ${output}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating H file ${relOut}")
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: hal_add_baked_data

  The ``hal_add_baked_data`` macro creates a custom target which sets up the
  dependencies and include directories for a target on data which is also
  created via hal_add_bin2h_target.

  Arguments:
    * ``target``: The name of the target and the name of the variable to access
      the data in the header file
    * ``name``: A name representing what we are generating
    * ``header``: The header filepath to generate
    * ``src``: The binary input filepath to be made into a header file
#]=======================================================================]
function(hal_add_baked_data TARGET NAME HEADER SRC)
  set(DATA_SRC ${SRC})
  if (NOT IS_ABSOLUTE ${DATA_SRC})
    set(DATA_SRC ${CMAKE_CURRENT_SOURCE_DIR}/${DATA_SRC})
  endif()
  hal_add_bin2h_target(${NAME} ${DATA_SRC} ${CMAKE_CURRENT_BINARY_DIR}/${HEADER})

  add_dependencies(${TARGET} ${NAME})

  target_include_directories(${TARGET} PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR})
endfunction()
