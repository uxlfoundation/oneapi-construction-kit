# Copyright (C) Codeplay Software Limited. All Rights Reserved.

#[=======================================================================[.rst:
A `CMake script`_ that wraps Khronos ``clang`` and compiles OpenCL-C kernels to
SPIR. (The SPIR kernels are used for testing SPIR support in ``libCL``. They
are also compiled to executables during ComputeAorta build-time and used to
test offline kernel support in ``libCL``.) For a given OpenCL-C kernel, this
script builds both 32-bit SPIR and 64-bit SPIR files (``.bc32``, ``.bc64``).

When compilation to SPIR is not required for any reason, then the script
creates stub files to enable dependency tracking and automatic rebuilding when
kernel sources change. This script is called as part of the ``regenerate-spir``
target. The expected use case is that this script will be wrapped in a custom
command.

.. warn::
  This script must be run from the ComputeAorta root directory (i.e., with
  ``WORKING_DIRECTORY ${ComputeAorta_SOURCE_DIR}``) so that dependencies can be
  found.

.. note::
  This script determines the names of the output SPIR files based on the input
  file name. Since the custom command that calls this script is created before
  this script is called, the output file names must be created separtely in the
  calling scope to enable dependency tracking. For an input file
  ``path/to/foo.cl``, the output files are ``path/to/foo.bc32`` and
  ``path/to/foo.bc64``.

The following variables must be defined when this script is invoked (using
``-DOPTION_NAME=value``):

.. cmake:variable:: INPUT_FILE

  The ``.cl`` file that will be compiled.

.. cmake:variable:: CA_EXTERNAL_KHRONOS_CLANG

  Absolute path to Khronos clang.

.. cmake:variable:: CA_EXTERNAL_OPENCL_SPIRH

  Absolute path to ``opencl_spir.h``.

.. _CMake script:
  https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#scripts
#]=======================================================================]

include("source/cl/test/UnitCL/cmake/ExtractReqsOpts.cmake")

# Enforce inputs. Since this is a script, it's possible to have these variables
# set to "" if something went wrong in with the caller, so also check for that.
if(NOT DEFINED INPUT_FILE OR INPUT_FILE STREQUAL "")
  message(FATAL_ERROR "INPUT_FILE not set")
endif()

if(NOT DEFINED CA_EXTERNAL_KHRONOS_CLANG
    OR CA_EXTERNAL_KHRONOS_CLANG STREQUAL "")
  message(FATAL_ERROR "CA_EXTERNAL_KHRONOS_CLANG not set")
endif()

if(NOT DEFINED CA_EXTERNAL_OPENCL_SPIRH OR CA_EXTERNAL_OPENCL_SPIRH STREQUAL "")
  message(FATAL_ERROR "CA_EXTERNAL_OPENCL_SPIRH not set")
endif()

# Extract directory, kernel name, and extension
# E.g., `foo.bar.cl` is split into `foo.bar` and `.cl`
get_filename_component(input_dir ${INPUT_FILE} DIRECTORY)
get_filename_component(kernel_name ${INPUT_FILE} NAME_WLE)
get_filename_component(input_ext ${INPUT_FILE} LAST_EXT)

if(NOT input_ext STREQUAL ".cl")
  message(FATAL_ERROR
    "Cannot compile ${INPUT_FILE}; only .cl files can be compiled to SPIR")
endif()

set(spir32_kernel_name "${kernel_name}.bc32")
set(spir64_kernel_name "${kernel_name}.bc64")

# Some of the kernels may be for specific target platforms and might live in
# different directories. We prepend the directory so we now have the full
# output path for the file.
set(spir32_output ${input_dir}/${spir32_kernel_name})
set(spir64_output ${input_dir}/${spir64_kernel_name})

# List of target requirements
set(REQUIREMENTS_LIST "")
# List of compile definitions
set(DEFS_LIST "")
# List of clang spir compilation options
set(SPIR_OPTIONS_LIST "")

extract_reqs_opts(
  INPUT_FILE ${INPUT_FILE}
  REQS_VAR REQUIREMENTS_LIST
  DEFS_VAR DEFS_LIST
  SPIR_OPTS_VAR SPIR_OPTIONS_LIST
)

# Skip when `nospir`
list(FIND REQUIREMENTS_LIST "nospir" idx)
if(NOT idx EQUAL -1)
  file(WRITE ${spir32_output} "// Skipped due to 'nospir' requirement")
  file(WRITE ${spir64_output} "// Skipped due to 'nospir' requirement")
  return()
endif()

# The legacy Khronos SPIR generator does not really support cl_khr_fp16.  The
# sample header has all the additional builtins commented out, and even if we
# reintroduce them then there is no -cl-ext option to enable the extension.
list(FIND REQUIREMENTS_LIST "half" idx)
if(NOT idx EQUAL -1)
  file(WRITE ${spir32_output} "// Skipped due to 'half' requirement")
  file(WRITE ${spir64_output} "// Skipped due to 'half' requirement")
  return()
endif()

# If the compilation of the kernel depends on test parameters then we have no
# way to compile the test in all parameter combinations, so don't offline
# compile, not generate SPIR nor SPIR-V.
list(FIND REQUIREMENTS_LIST "parameters" idx)
if(NOT idx EQUAL -1)
  file(WRITE ${spir32_output} "// Skipped due to 'parameters' requirement")
  file(WRITE ${spir64_output} "// Skipped due to 'parameters' requirement")
  return()
endif()

# If the kernel has extra options to pass to CLC we should also pass them in
# via -cl-spir-compile-options so they end up in the kernel metadata.
set(OPTIONS_LIST ${DEFS_LIST} ${SPIR_OPTIONS_LIST})
if(NOT "${OPTIONS_LIST}" STREQUAL "")
  set(SPIR_COMPILE_OPTIONS "-cl-spir-compile-options")
else()
  set(SPIR_COMPILE_OPTIONS "")
endif()

# execute_process() needs a ;-separated list for arguments, but all other uses
# need a space-separated list
string(REPLACE ";" " " OPTIONS_LIST_SPACES "${OPTIONS_LIST}")

# Build .bc32 file
execute_process(
  COMMAND ${CA_EXTERNAL_KHRONOS_CLANG}
    -cc1 -emit-llvm-bc -triple spir-unknown-unknown
    ${SPIR_COMPILE_OPTIONS} "${OPTIONS_LIST_SPACES}"
    -include ${CA_EXTERNAL_OPENCL_SPIRH}
    ${OPTIONS_LIST}
    -O0 -Werror
    -o ${spir32_output}
    ${INPUT_FILE}
  RESULT_VARIABLE clang_result
  ERROR_VARIABLE clang_error)

if(NOT clang_result EQUAL 0)
  # execute_process() doesn't print the failing command, so attempt to
  # reconstruct it here
  message(FATAL_ERROR
    "clang-spir failed with status '${clang_result}':
    ${CA_EXTERNAL_KHRONOS_CLANG}
      -cc1 -emit-llvm-bc -triple spir-unknown-unknown
      ${SPIR_COMPILE_OPTIONS} \"${OPTIONS_LIST_SPACES}\"
      -include ${CA_EXTERNAL_OPENCL_SPIRH}
      ${OPTIONS_LIST_SPACES}
      -O0 -Werror
      -o ${spir32_output}
      ${INPUT_FILE}
    ${clang_error}")
endif()

# Build .bc64 file
execute_process(
  COMMAND ${CA_EXTERNAL_KHRONOS_CLANG}
    -cc1 -emit-llvm-bc -triple spir64-unknown-unknown
    ${SPIR_COMPILE_OPTIONS} "${OPTIONS_LIST_SPACES}"
    -include ${CA_EXTERNAL_OPENCL_SPIRH}
    ${OPTIONS_LIST}
    -O0 -Werror
    -o ${spir64_output}
    ${INPUT_FILE}
  RESULT_VARIABLE clang_result
  ERROR_VARIABLE clang_error)

if(NOT clang_result EQUAL 0)
  # execute_process() doesn't print the failing command, so attempt to
  # reconstruct it here
  message(FATAL_ERROR
    "clang-spir failed with status ${clang_result}:
    ${CA_EXTERNAL_KHRONOS_CLANG}
      -cc1 -emit-llvm-bc -triple spir64-unknown-unknown
      ${SPIR_COMPILE_OPTIONS} \"${OPTIONS_LIST_SPACES}\"
      -include ${CA_EXTERNAL_OPENCL_SPIRH}
      ${OPTIONS_LIST_SPACES}
      -O0 -Werror
      -o ${spir64_output}
      ${INPUT_FILE}
    ${clang_error}")
endif()
