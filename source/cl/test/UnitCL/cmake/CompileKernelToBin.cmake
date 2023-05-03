# Copyright (C) Codeplay Software Limited. All Rights Reserved.

#[=======================================================================[.rst:
A `CMake script`_ that wraps the ``clc`` tool and compiles the offline kernels
that are used for testing. All the necessary option and requirement parsing are
performed to determine whether compilation is even required, and if it is, what
the ``clc`` invocation should be. A stub file is created when compilation is
not required, enabling dependency tracking and automatic rebuilding when kernel
sources change. The expected use case is that this script will be wrapped in a
custom command.

.. warn::
  This script must be run from the ComputeAorta root directory (i.e., with
  ``WORKING_DIRECTORY ${ComputeAorta_SOURCE_DIR}``) so that dependencies can be
  found.

The following variables must be defined when this script is invoked (using
``-DOPTION_NAME=value``):

.. cmake:variable:: INPUT_FILE

  The file that will be compiled. It will be a ``.cl``, a ``.bc{32|64}`` or a
  ``.spv{32|64}`` file.

.. cmake:variable:: OUTPUT_FILE

  The compiled executable file. It will be a ``.bin``.

.. cmake:variable:: CLC_EXECUTABLE

  Absolute path to the ``clc`` executable that will be used for compilation.
  ``CLC_EXECUTABLE`` may contain a ``qemu`` invocation of ``clc`` when ``clc``
  has been built for another architecture.

.. cmake:variable:: DEVICE_NAME

  The name of the OpenCL device that ``clc`` will compile for. This must match
  one of the devices that the provided ``clc`` supports.

.. cmake:variable:: TARGET_CAPABILITIES

  The list of capabilities supported by device ``DEVICE_NAME``. This is used to
  determine whether compilation is possible.

.. cmake:variable:: IMAGE_SUPPORT

  Boolean for whether device ``DEVICE_NAME`` supports images (since image
  support is not treated as a capability). This is used to determine whether
  compilation is possible.

The following variable may optionally be defined:

.. cmake:variable:: INPUT_CL_FILE

  File to parse for target device requirements and compilation options. It must
  be a ``.cl`` file.

  .. warning::
    Only ``INPUT_CL_FILE`` is ever parsed, even when ``INPUT_FILE`` is a
    ``.cl`` file. Consequently, when ``INPUT_FILE`` is a ``.cl`` file, the same
    ``.cl`` file is passed to this script twice.

  .. note::
    ``INPUT_CL_FILE`` is  determined when the custom command wrapping this
    script is created so that dependencies can be tracked correctly, and this
    script can be re-run when ``INPUT_CL_FILE`` changes (even when
    ``INPUT_FILE`` is an IR file and not a ``.cl`` file).

.. _CMake script:
  https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#scripts
#]=======================================================================]

include("source/cl/test/UnitCL/cmake/ExtractReqsOpts.cmake")

# Enforce inputs. Since this is a script, it's possible to have these variables
# set to "" if something went wrong in with the caller, so also check for that.
if(NOT DEFINED INPUT_FILE OR INPUT_FILE STREQUAL "")
  message(FATAL_ERROR "INPUT_FILE not set")
endif()

# Optional: INPUT_CL_FILE

if(NOT DEFINED OUTPUT_FILE OR OUTPUT_FILE STREQUAL "")
  message(FATAL_ERROR "OUTPUT_FILE not set")
endif()

if(NOT DEFINED CLC_EXECUTABLE OR CLC_EXECUTABLE STREQUAL "")
  message(FATAL_ERROR "CLC_EXECUTABLE not set")
endif()

if(NOT DEFINED DEVICE_NAME OR DEVICE_NAME STREQUAL "")
  message(FATAL_ERROR "DEVICE_NAME not set")
endif()

# Note: The list separator doesn't matter; the variable just gets searched.
if(NOT DEFINED TARGET_CAPABILITIES OR TARGET_CAPABILITIES STREQUAL "")
  message(FATAL_ERROR "TARGET_CAPABILITIES not set")
endif()

if(NOT DEFINED IMAGE_SUPPORT OR IMAGE_SUPPORT STREQUAL "")
  message(FATAL_ERROR "IMAGE_SUPPORT not set")
endif()

# Extract directory, kernel name, and extension
# E.g., `foo.bar.cl` is split into `foo.bar` and `.cl`
get_filename_component(input_dir ${INPUT_FILE} DIRECTORY)
get_filename_component(kernel_name ${INPUT_FILE} NAME_WLE)
get_filename_component(input_ext ${INPUT_FILE} LAST_EXT)

# Check the extension
set(valid_extensions ".cl" ".bc32" ".bc64" ".spv32" ".spv64")
list(FIND valid_extensions "${input_ext}" ext_idx)
if(ext_idx EQUAL -1)
  message(FATAL_ERROR "File with unknown extension: '${INPUT_FILE}'")
endif()

# Early exit if IR bit width doesn't match target capabilities
if(${input_ext} MATCHES "32" AND NOT TARGET_CAPABILITIES MATCHES "32bit"
    OR ${input_ext} MATCHES "64" AND NOT TARGET_CAPABILITIES MATCHES "64bit")
  file(WRITE ${OUTPUT_FILE} "// Skipped due to bit width mismatch")
  return()
endif()

# List of target requirements
set(REQUIREMENTS_LIST "")
# List of compile definitions
set(DEFS_LIST "")
# Required OpenCL C version
set(CLC_CL_STD "")
# List of clc compilation options
set(CLC_OPTIONS_LIST "")

# INPUT_FILE may be SPIR or SPIR-V, but there may still be relevant options in
# the corresponding .cl file (if it exists).
if(NOT INPUT_CL_FILE STREQUAL "")
  get_filename_component(input_cl_ext ${INPUT_CL_FILE} LAST_EXT)
  if(NOT input_cl_ext STREQUAL ".cl")
    message(FATAL_ERROR
      "INPUT_CL_FILE must be a .cl file. File given: '${INPUT_CL_FILE}'")
  endif()

  extract_reqs_opts(
    INPUT_FILE ${INPUT_CL_FILE}
    REQS_VAR REQUIREMENTS_LIST
    DEFS_VAR DEFS_LIST
    CL_STD_VAR CLC_CL_STD
    CLC_OPTS_VAR CLC_OPTIONS_LIST
  )
endif()

  # Assume a default of CL1.2.
if(NOT CLC_CL_STD)
  set(CLC_CL_STD "1.2")
endif()

# Set the default requirements for the kernels
set(REQUIREMENT_FP64 OFF)
set(REQUIREMENT_FP16 OFF)
set(REQUIREMENT_IMAGES OFF)

# At the moment, `clc` has no method to find out what dependencies a kernel has
# and to skip compilation appropriately. We need to separate `double`, `half`
# and some other tests based on these conditions. For now that means adding a
# comment in the right format with the requirements.
foreach(requirement ${REQUIREMENTS_LIST})
  if(requirement STREQUAL "noclc")
    # Nothing to compile
    file(WRITE ${OUTPUT_FILE} "// Skipped due to 'noclc' requirement")
    return()
  elseif(requirement STREQUAL "double")
    set(REQUIREMENT_FP64 ON)
  elseif(requirement STREQUAL "half")
    set(REQUIREMENT_FP16 ON)
  elseif(requirement STREQUAL "images")
    set(REQUIREMENT_IMAGES ON)
  # TODO CA-1830: These kernels are used in parameter tests but use these
  # parameters as macros. This is difficult to compile with `clc` so we need to
  # work out a way to do so.
  elseif(requirement STREQUAL "parameters")
    file(WRITE ${OUTPUT_FILE} "// Skipped due to 'parameters' requirement")
    return()
  else()
    # Do nothing
  endif()
endforeach()

# If the kernel requires FP64 or FP16 but the target doesn't have the
# capability then skip compiling this file
if((REQUIREMENT_FP64 AND NOT TARGET_CAPABILITIES MATCHES "fp64") OR
  (REQUIREMENT_FP16 AND NOT TARGET_CAPABILITIES MATCHES "fp16"))
  file(WRITE ${OUTPUT_FILE}
    "// Skipped due to missing floating point capabilities")
  return()
endif()

# If the kernel requires images we don't have a target property to check
# whether images are supported, so we just check if host supports images as a
# proxy (this  will need to change when an external target cares about images
# and offline compilation).
if(REQUIREMENT_IMAGES AND NOT IMAGE_SUPPORT)
  file(WRITE ${OUTPUT_FILE} "// Skipped due to missing image support")
  return()
endif()

# ${CLC_EXECUTABLE} may have other things in it (like a qemu invocation). Turn
# it into a CMake list, so that execute_process() isn't confused.
string(REPLACE " " ";" CLC_EXECUTABLE "${CLC_EXECUTABLE}")
execute_process(
  COMMAND ${CLC_EXECUTABLE}
    -d ${DEVICE_NAME}
    -cl-kernel-arg-info
    -cl-std=CL${CLC_CL_STD}
    ${CLC_OPTIONS_LIST}
    ${DEFS_LIST}
    -o ${OUTPUT_FILE} -- ${INPUT_FILE}
  RESULT_VARIABLE clc_result
  OUTPUT_VARIABLE clc_output
  ERROR_VARIABLE clc_error)

if(NOT clc_result EQUAL 0)
  # execute_process() doesn't print the failing command, so attempt to
  # reconstruct it here
  message(FATAL_ERROR
    "clc failed with status '${clc_result}':
    ${CLC_EXECUTABLE}
      -d '${DEVICE_NAME}'
      -cl-kernel-arg-info
      -cl-std=CL${CLC_CL_STD}
      ${CLC_OPTIONS_LIST}
      ${DEFS_LIST}
      -o '${OUTPUT_FILE}'
      -- '${INPUT_FILE}'
    ${clc_error}")
endif()
