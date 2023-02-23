# Copyright (C) Codeplay Software Limited. All Rights Reserved.

#[=======================================================================[.rst:
A `CMake script`_ that wraps ``clang``, ``llvm-spirv``, and ``spirv-dis``, and
compiles OpenCL-C kernels to SPIR-V assembly. (The SPIR-V kernels are used for
testing SPIR-V support in ``libCL``. They are also compiled to executables
during ComputeAorta build-time and used to test offline kernel support in
``libCL``.) For a given OpenCL-C kernel, this script builds both 32-bit and
64-bit SPIR-V assembly files (``.spvasm32``, ``.spvasm64``).

When compilation to SPIR-V is not required for any reason, then the script
creates stub files to enable dependency tracking and automatic rebuilding when
kernel sources change. This script is called as part of the
``regenerate-spir-spirv`` target. The expected use case is that this script
will be wrapped in a custom command.

.. warn::
  This script must be run from the ComputeAorta root directory (i.e., with
  ``WORKING_DIRECTORY ${ComputeAorta_SOURCE_DIR}``) so that dependencies can be
  found.

.. note::
  This script determines the names of the output SPIR-V files based on the
  input file name. Since the custom command that calls this script is created
  before this script is called, the output file names must be created separtely
  in the calling scope to enable dependency tracking. For an input file
  ``path/to/foo.cl``, the output files are ``path/to/foo.spvasm32`` and
  ``path/to/foo.spvasm64``.

The following variables must be defined when this script is invoked (using
``-DOPTION_NAME=value``):

.. cmake:variable:: INPUT_FILE

  The ``.cl`` file that will be compiled.

.. cmake:variable:: CLANG_PATH

  Absolute path to the ``clang`` ComputeAorta has been configured with.

.. cmake:variable:: LLVM_SPIRV_PATH

  Absolute path to ``llvm-spirv``.

  .. warn::
    ``clang`` and ``llvm-spirv`` versions must match.

.. cmake:variable:: SPIRV_DIS_PATH

  Absolute path to ``spirv-dis``.

.. cmake:variable:: BINARY_DIR

  Absolute path to a location in the current build directory. This is used for
  temporary files.

.. _CMake script:
  https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#scripts
#]=======================================================================]

# This script must be run from the CA root so that dependencies can be found
include("source/cl/test/UnitCL/cmake/ExtractReqsOpts.cmake")

# Enforce inputs. Since this is a script, it's possible to have these variables
# set to "" if something went wrong in with the caller, so also check for that.
if(NOT DEFINED INPUT_FILE OR INPUT_FILE STREQUAL "")
  message(FATAL_ERROR "INPUT_FILE not set")
endif()

if(NOT DEFINED CLANG_PATH OR CLANG_PATH STREQUAL "")
  message(FATAL_ERROR "CLANG_PATH not set")
endif()

if(NOT DEFINED LLVM_SPIRV_PATH OR LLVM_SPIRV_PATH STREQUAL "")
  message(FATAL_ERROR "LLVM_SPIRV_PATH not set")
endif()

if(NOT DEFINED SPIRV_DIS_PATH OR SPIRV_DIS_PATH STREQUAL "")
  message(FATAL_ERROR "SPIRV_DIS_PATH not set")
endif()

if(NOT DEFINED BINARY_DIR OR BINARY_DIR STREQUAL "")
  message(FATAL_ERROR "BINARY_DIR not set")
endif()

# Split kernel name up: `d/a.b.c` => `d`, `a.b`, `.c`
get_filename_component(input_dir ${INPUT_FILE} DIRECTORY)
get_filename_component(kernel_name ${INPUT_FILE} NAME_WLE)
get_filename_component(input_ext ${INPUT_FILE} LAST_EXT)

if(NOT input_ext STREQUAL ".cl")
  message(FATAL_ERROR
    "Cannot compile ${INPUT_FILE}; only .cl files can be compiled to SPIR-V")
endif()

set(temp_bc32 ${BINARY_DIR}/${kernel_name}.bc32-temp)
set(temp_bc64 ${BINARY_DIR}/${kernel_name}.bc64-temp)
set(spirv32_output "${BINARY_DIR}/${kernel_name}.spv32")
set(spirv64_output "${BINARY_DIR}/${kernel_name}.spv64")
set(spvasm32_output "${input_dir}/${kernel_name}.spvasm32")
set(spvasm64_output "${input_dir}/${kernel_name}.spvasm64")

# List of target requirements
set(REQUIREMENTS_LIST "")
# List of compile definitions
set(DEFS_LIST "")
# List of clang spir-v compilation options
set(SPIRV_OPTIONS_LIST "")

extract_reqs_opts(
  INPUT_FILE ${INPUT_FILE}
  REQS_VAR REQUIREMENTS_LIST
  DEFS_VAR DEFS_LIST
  SPIRV_OPTS_VAR SPIRV_OPTIONS_LIST
)

# Skip when `nospirv`
list(FIND REQUIREMENTS_LIST "nospirv" idx)
if(NOT idx EQUAL -1)
  file(WRITE ${spvasm32_output} "// Skipped due to 'nospirv' requirement")
  file(WRITE ${spvasm64_output} "// Skipped due to 'nospirv' requirement")
  return()
endif()

# If the compilation of the kernel depends on test parameters then we have no
# way to compile the test in all parameter combinations, so don't offline
# compile, not generate SPIR nor SPIR-V.
list(FIND REQUIREMENTS_LIST "parameters" idx)
if(NOT idx EQUAL -1)
  file(WRITE ${spvasm32_output} "// Skipped due to 'parameters' requirement")
  file(WRITE ${spvasm64_output} "// Skipped due to 'parameters' requirement")
  return()
endif()

# We check in the textual spirv asm format of the spirv kernel. As such we have
# an additional step and dependency for these commands. First we use the modern
# version of clang in `CA_LLVM_INSTALL_DIR` to generate a new .bc file from the
# kernel that can be used with `llvm-spirv`. Then we use `llvm-spirv` to create
# a SPIR-V binary from that, and finally `spirv-dis` to get the disassembled
# file that is checked in to the repo.

# execute_process() doesn't print failing commands, so each call is followed by
# a check and a message printing the reconstructed command.

# Build .spvasm32

# Compile to .bc32
execute_process(
  COMMAND ${CLANG_PATH} -c -emit-llvm -target spir-unknown-unknown -cl-std=CL1.2
    -Xclang -finclude-default-header
    ${DEFS_LIST} ${SPIRV_OPTIONS_LIST}
    -O0 -Werror
    -o ${temp_bc32}
    ${INPUT_FILE}
  RESULT_VARIABLE clang_result
  ERROR_VARIABLE clang_error)

if(NOT clang_result EQUAL 0)
  string(REPLACE ";" " " PRINTABLE_OPTIONS_LIST
    "${DEFS_LIST} ${SPIRV_OPTIONS_LIST}")
  message(FATAL_ERROR
    "clang failed with status '${clang_result}':
    ${CLANG_PATH} -c -emit-llvm -target spir-unknown-unknown -cl-std=CL1.2
      -Xclang -finclude-default-header
      ${PRINTABLE_OPTIONS_LIST}
      -O0 -Werror
      -o ${temp_bc32}
      ${INPUT_FILE}
    ${clang_error}")
endif()

# Compile to .spv32
execute_process(
  COMMAND ${LLVM_SPIRV_PATH} ${temp_bc32} -o ${spirv32_output}
  RESULT_VARIABLE spirv_result
  ERROR_VARIABLE spirv_error)

if(NOT spirv_result EQUAL 0)
  message(FATAL_ERROR
    "llvm-spirv failed with status '${spirv_result}':
    ${LLVM_SPIRV_PATH} ${temp_bc32} -o ${spirv32_output}
    ${spirv_error}")
endif()

# Disassemble to .spvasm32
execute_process(
  COMMAND ${SPIRV_DIS_PATH} -o ${spvasm32_output} ${spirv32_output}
  RESULT_VARIABLE dis_result
  ERROR_VARIABLE dis_error)

if(NOT dis_result EQUAL 0)
  message(FATAL_ERROR
    "spirv-dis failed with status '${dis_result}':
    ${SPIRV_DIS_PATH} -o ${spvasm32_output} ${spirv32_output}
    ${dis_error}")
endif()

# Remove temporary files
file(REMOVE ${spirv32_output} ${temp_bc32})

# Build .spvasm64

# Compile to .bc64
execute_process(
  COMMAND ${CLANG_PATH} -c -emit-llvm -target spir64-unknown-unknown
    -cl-std=CL1.2 -Xclang -finclude-default-header
    ${DEFS_LIST} ${SPIRV_OPTIONS_LIST}
    -O0 -Werror
    -o ${temp_bc64}
    ${INPUT_FILE}
  RESULT_VARIABLE clang_result
  ERROR_VARIABLE clang_error)

if(NOT clang_result EQUAL 0)
  string(REPLACE ";" " " PRINTABLE_OPTIONS_LIST
    "${DEFS_LIST} ${SPIRV_OPTIONS_LIST}")
  message(FATAL_ERROR
    "clang failed with status '${clang_result}':
    ${CLANG_PATH} -c -emit-llvm -target spir64-unknown-unknown -cl-std=CL1.2
      -Xclang -finclude-default-header
      ${PRINTABLE_OPTIONS_LIST}
      -O0 -Werror
      -o ${temp_bc64}
      ${INPUT_FILE}
    ${clang_error}")
endif()

# Compile to .spv64
execute_process(
  COMMAND ${LLVM_SPIRV_PATH} ${temp_bc64} -o ${spirv64_output}
  RESULT_VARIABLE spirv_result
  ERROR_VARIABLE spirv_error)

if(NOT spirv_result EQUAL 0)
  message(FATAL_ERROR
    "llvm-spirv failed with status '${spirv_result}':
    ${LLVM_SPIRV_PATH} ${temp_bc64} -o ${spirv64_output}
    ${spirv_error}")
endif()

# Disassemble to .spvasm64
execute_process(
  COMMAND ${SPIRV_DIS_PATH} -o ${spvasm64_output} ${spirv64_output}
  RESULT_VARIABLE dis_result
  ERROR_VARIABLE dis_error)

if(NOT dis_result EQUAL 0)
  message(FATAL_ERROR
    "spirv-dis failed with status '${dis_result}':
    ${SPIRV_DIS_PATH} -o ${spvasm64_output} ${spirv64_output}
    ${dis_error}")
endif()

# Remove temporary files
file(REMOVE ${spirv64_output} ${temp_bc64})
