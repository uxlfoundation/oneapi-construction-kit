# Copyright (C) Codeplay Software Limited. All Rights Reserved.

#[=======================================================================[.rst:
Utilities to compile GLSL compute shaders to SPIR-V binaries and convert them
to header files at build time. This module depends on the :doc:`/cmake/Bin2H`
and :doc:`/cmake/CAPlatform` modules to function. To use these utilities:

.. code:: cmake

  include(GLSL)

  add_glsl_command(
    ${path_to_input}/file.comp
    ${path_to_output}/file.spv)

  add_glsl_target(my_spv
    ${path_to_input}/file.comp
    ${path_to_output}/file.spv)

  add_glsl_bin2h_command(my_spv
    ${path_to_input}/file.comp
    ${path_to_output}/file.h)

  add_glsl_bin2h_target(my_spv
    ${path_to_input}/file.comp
    ${path_to_output}/file.h)
#]=======================================================================]
include(Bin2H)
include(CAPlatform)

# Make sure we have the glslang validator
if(NOT TARGET glslangValidator)
  find_program(GLSLANG_VALIDATOR_PATH
    glslangValidator${CA_HOST_EXECUTABLE_SUFFIX}
    HINTS "$ENV{VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/Bin")
  if(GLSLANG_VALIDATOR_PATH STREQUAL "GLSLANG_VALIDATOR_PATH-NOTFOUND")
    message(FATAL_ERROR "glslangValidator not found, ensure the Vulkan SDK is "
                        "installed. Please visit https://vulkan.lunarg.com/ "
                        "to download!")
  else()
    add_ca_executable_import(glslangValidator ${GLSLANG_VALIDATOR_PATH})
  endif()
endif()

#[=======================================================================[.rst:
.. cmake:command:: add_glsl_command

  The ``add_glsl_command`` macro creates a custom command which compiles a GLSL
  compute shader into a SPIR-V binary using glslangValidator provided by the
  Vulkan SDK.

  Arguments:
    * ``input``: The GLSL compute shader source filepath
    * ``output``: The SPIR-V binary output filepath
#]=======================================================================]
macro(add_glsl_command input output)
  file(RELATIVE_PATH relOut ${CMAKE_BINARY_DIR} ${output})
  add_custom_command(OUTPUT ${output}
    COMMAND glslangValidator -s -V -o ${spv} ${input}
    DEPENDS glslangValidator ${input}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building GLSL object ${relOut}")
endmacro()

#[=======================================================================[.rst:
.. cmake:command:: add_spvasm_command

  The ``add_spvasm_command`` macro creates a custom command which compiles a
  hand-written SPVASM compute shader into a SPIR-V binary using spv-as provided
  by the Vulkan SDK.

  Arguments:
    * ``input``: The GLSL compute shader source filepath
    * ``output``: The SPIR-V binary output filepath
#]=======================================================================]
macro(add_spvasm_command input output)
  file(RELATIVE_PATH relOut ${CMAKE_BINARY_DIR} ${output})
  add_custom_command(OUTPUT ${output}
    COMMAND spirv::spirv-as --target-env vulkan1.0 -o ${spv} ${spvasm}
    DEPENDS spirv::spirv-as ${spvasm}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building SPVASM object ${relOut}")
endmacro()

#[=======================================================================[.rst:
.. cmake:command:: add_glsl_target

  The ``add_glsl_target`` macro creates creates a custom target which compiles
  a GLSL compute shader into a SPIR-V binary and used in CMake dependency
  tracking.

  Arguments:
    * ``target``: The name of the target
    * ``input``: The GLSL compute shader source filepath
    * ``output``: The SPIR-V binary output filepath
#]=======================================================================]
macro(add_glsl_target target input output)
  add_glsl_command(${input} ${output})
  add_custom_target(${target}
    DEPENDS ${output}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building GLSL object ${output}")
endmacro()

#[=======================================================================[.rst:
.. cmake:command:: add_glsl_bin2h_command

  The ``add_glsl_bin2h_command`` function creates a custom command which
  compiles a GLSL compute shader into a SPIR-V binary then generates a header
  that can be included into source code.

  .. seealso::
    See :doc:`add_bin2h_command</cmake/Bin2H>` documentation for more
    information.

  Arguments:
    * ``variable``: The name of the variable to access the data in the header
      file
    * ``input``: The GLSL compute shader source filepath
    * ``output``: The header filepath to generate
#]=======================================================================]
function(add_glsl_bin2h_command variable input output)
  get_filename_component(name ${input} NAME_WE)
  set(spv ${CMAKE_CURRENT_BINARY_DIR}/${name}.spv)
  add_glsl_command(${input} ${spv})
  add_bin2h_command(${variable} ${spv} ${output})
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_spvasm_bin2h_command

  The ``add_spvasm_bin2h_command`` function creates a custom command which
  compiles a hand-written spvasm compute shader into a SPIR-V binary then
  generates a header that can be included into source code.

  .. seealso::
    See :doc:`add_bin2h_command</cmake/Bin2H>` documentation for more
    information.

  Arguments:
    * ``variable``: The name of the variable to access the data in the header
      file
    * ``input``: The SPVASM compute shader source filepath
    * ``output``: The header filepath to generate
#]=======================================================================]
function(add_spvasm_bin2h_command variable input output)
  get_filename_component(name ${input} NAME_WE)
  set(spv ${CMAKE_CURRENT_BINARY_DIR}/${name}.spv)
  add_spvasm_command(${input} ${spv})
  add_bin2h_command(${variable} ${spv} ${output})
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_glsl_bin2h_target

  The ``add_glsl_bin2h_target`` macro creates a custom target which compiles a
  GLSL compute shader into a SPIR-V binary then generates a header that can be
  included into source code and use the target in CMake dependency tracking.

  Arguments:
    * ``target``: The name of the target and the name of the variable to access
      the data in the header file
    * ``input``: The GLSL compute shader source filepath
    * ``output``: The header filepath to generate
#]=======================================================================]
macro(add_glsl_bin2h_target target input output)
  add_glsl_bin2h_command(${target} ${input} ${output})
  add_custom_target(${target}
    DEPENDS ${output} ${input}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating H file ${output}")
endmacro()
