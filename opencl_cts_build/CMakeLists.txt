# Copyright (C) Codeplay Software Limited. All Rights Reserved.

cmake_minimum_required(VERSION 3.4.3 FATAL_ERROR)
project(CA-OpenCL-CTS)

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_CTS_LLVM_VERSION

  Select the desired LLVM version to use for the ``clang`` and `llvm-spirv``
  dependencies. This option controls which commit of the SPIRV-LLVM-Translator
  to checkout when building ``llvm-spirv`` to match the version of ``clang``
  detected by :cmake:command:`find_package`.
#]=======================================================================]
set(CA_CL_CTS_LLVM_VERSION 14.0.0 CACHE STRING
  "LLVM version to target, for clang and llvm-spirv")
message(STATUS "CA-OpenCL-CTS LLVM Version: ${CA_CL_CTS_LLVM_VERSION}")

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_CTS_BUILD_32_BITS

  Enable building 32-bit executables for the OpenCL-ICD-Loader and OpenCL-CTS
  on a 64-bit Linux operating system.
#]=======================================================================]
if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT CMAKE_SYSTEM_NAME STREQUAL Windows)
  option(CA_CL_CTS_BUILD_32_BITS "Build 32-bit executables" OFF)
  message(STATUS
    "CA-OpenCL-CTS Build 32-bit executables: ${CA_CL_CTS_BUILD_32_BITS}")
endif()

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_CTS_CLC_EXECUTABLE

  Specify the path to the ``clc`` executable used for testing the _binary_
  compilation mode.
#]=======================================================================]
set(CA_CL_CTS_CLC_EXECUTABLE "" CACHE STRING
  "Path to clc executable for binary compilation")
message(STATUS "CA-OpenCL-CTS clc executable: ${CA_CL_CTS_CLC_EXECUTABLE}")

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_CTS_ICD_LOADER_INSTALL_DIR

  Path to an existing OpenCL-ICD-Loader install directory.
#]=======================================================================]
set(CA_CL_CTS_ICD_LOADER_INSTALL_DIR "" CACHE PATH
  "Path to external OpenCL-ICD-Loader install directory")
message(STATUS "CA-OpenCL-CTS OpenCL-ICD-Loader install directory: "
  "${CA_CL_CTS_ICD_LOADER_INSTALL_DIR}")

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_CTS_COMPILATION_MODE

  Semi-colon separated list of offline compilation modes to enable, by default
  both ``binary`` and ``spir-v`` compilation modes are enabled.
#]=======================================================================]
set(CA_CL_CTS_COMPILATION_MODE "binary;spir-v" CACHE STRING
  "Semi-colon separate list of offline compilation modes to enable")
message(STATUS "CA-OpenCL-CTS Enabled Compilation Modes: "
  "${CA_CL_CTS_COMPILATION_MODE}")

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_CTS_SPLIT_VECTORS

  Boolean variable to control whether the CTS spreadsheets should have tests
  split per vector width.
#]=======================================================================]
set(CA_CL_CTS_SPLIT_VECTORS OFF CACHE BOOL
  "Boolean to enable splitting CTS tests per vector width")
message(STATUS "CA-OpenCL-CTS Split vectors: "
  "${CA_CL_CTS_SPLIT_VECTORS}")

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_CTS_COMPILATION_CACHE_PATH

  Path, relative to :cmake:variable:`CMAKE_PREFIX_PATH```/bin``, for the
  compilation cache directory when compilation mode is ``binary`` or
  ``spir-v``. Used by :cmake:command:`add_ca_cl_cts_csv_file` to generate CSV
  files for each compilation mode.
#]=======================================================================]
set(CA_CL_CTS_COMPILATION_CACHE_PATH "." CACHE PATH
  "Path (relative to install dir) for the compilation cache")
message(STATUS
  "CA-OpenCL-CTS Compilation Cache Path: ${CA_CL_CTS_COMPILATION_CACHE_PATH}")

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_CTS_COMPILATION_CACHE_MODE

  Compilation cache mode, choose from ``compile-if-absent``, ``force-read``,
  ``overwrite``, and ``dump-cl-files``, used to specify how programs should be
  compiled and/or loaded from the compilation cache when the compilation mode
  is ``binary`` or ``spir-v``. Used by :cmake:command:`add_ca_cl_cts_csv_file`
  to generate CSV files for each compilation mode.
#]=======================================================================]
set(CA_CL_CTS_COMPILATION_CACHE_MODE "" CACHE STRING
  "Mode of operation for the compilation cache")
message(STATUS
  "CA-OpenCL-CTS Compilation Cache Mode: ${CA_CL_CTS_COMPILATION_CACHE_MODE}")

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_CTS_COMPILATION_WRAPPER

  The compilation wrapper is a path to an executable which, when set, will be
  invoked so that additional logic can be performed. If tests are being
  executed on a device which is unable to run the offline compilation tools
  locally. For example, this may be used to perform offline compilation over
  ``ssh``.
#]=======================================================================]
set(CA_CL_CTS_COMPILATION_WRAPPER "" CACHE PATH
  "Compilation command wrapper, for use when local compilation is not feasible")
message(STATUS
  "CA-OpenCL-CTS Compilation Wrapper: ${CA_CL_CTS_COMPILATION_WRAPPER}")

# Use ExternalProject to glue together the dependncies required to build the
# OpenCL-CTS, a few of which assume they are the root CMake project and don't
# get along with add_subdirectory().
include(ExternalProject)

#[=======================================================================[.rst:
.. cmake:command:: add_external_project_clean_target

  :cmake:command:`ExternalProject_Add` has some shortcomings around source code
  updates and the clean target. This command provides an explicit
  ``clean-${prefix}`` target which invokes the ``clean`` target in the build
  system of the external project and removes the stamp files, this means the
  external project can be rebuilt as new rather than being skipped due to stamp
  files existing.

  Arguments:
    * ``prefix`` the external project prefix to add the clean target for.
#]=======================================================================]
function(add_external_project_clean_target prefix)
  set(PrefixDir ${CMAKE_CURRENT_BINARY_DIR}/${prefix})
  set(BinaryDir ${PrefixDir}/build)
  set(StampDir ${PrefixDir}/src/${prefix}-stamp)
  add_custom_target(clean-${prefix}
    COMMAND ${CMAKE_COMMAND} --build ${BinaryDir} --target clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${StampDir})
  set(CleanFiles
    ${StampDir}/${prefix}-build
    ${StampDir}/${prefix}-done
    ${StampDir}/${prefix}-install
    ${StampDir}/${prefix}-patch
    ${StampDir}/${prefix}-configure
    ${StampDir}/${prefix}-download
    ${StampDir}/${prefix}-mkdir
    ${StampDir}/${prefix}-update)
  set_directory_properties(PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES "${CleanFiles}")
endfunction()

find_package(PythonInterp REQUIRED)

#[=======================================================================[.rst:
.. cmake:command:: add_ca_cl_cts_csv_file

  There are multiple CSV file test input configurations required to run with
  full range math testing, and in the three compilation modes (online, binary,
  and spir-v). The lists of tests do not change so we configure 12 different
  CSV files from the two located in the scripts directory. To generate all CSV
  files use the ``csv-files`` target.

  Keyword Arguments:
  * ``IMAGES`` - Flag to specify that an images-specific CSV should be used.

  Arguments:
    * ``CL_STD`` is the OpenCL standard to test, "3.0".
    * ``MATH_MODE`` determines math testing strictness, "full" or "wimpy".
    * ``COMPILATION_MODE`` tests different compilation paths; "online",
      "binary", or "spir-v".
#]=======================================================================]
function(add_ca_cl_cts_csv_file)
  cmake_parse_arguments(args "IMAGES" "CL_STD;MATH_MODE;COMPILATION_MODE" "" ${ARGN})
  if(NOT IS_DIRECTORY ${PROJECT_BINARY_DIR}/scripts)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/scripts)
  endif()
  if (NOT ${args_IMAGES})
    set(input
      ${PROJECT_SOURCE_DIR}/scripts/cts-${args_CL_STD}-subtest-full-online.csv)
    set(output
      scripts/cts-${args_CL_STD}-subtest-${args_MATH_MODE}-${args_COMPILATION_MODE}.csv)
  else()
    set(ImagesName -images)
    set(CompilationImages --images)
    set(input
      ${PROJECT_SOURCE_DIR}/scripts/cts-${args_CL_STD}-subtest-images-full-online.csv)
    set(output
      scripts/cts-${args_CL_STD}-subtest-images-${args_MATH_MODE}-${args_COMPILATION_MODE}.csv)
  endif()
  # TODO: Support compiler cache mode and path with CMake options.
  set(generateCsvFileScript
    ${PROJECT_SOURCE_DIR}/scripts/generate-csv-file.py)
  if(CA_CL_CTS_COMPILATION_CACHE_MODE)
    set(CompilationCacheMode
      --compilation-cache-mode ${CA_CL_CTS_COMPILATION_CACHE_MODE})
  endif()
  if(CA_CL_CTS_COMPILATION_CACHE_PATH)
    set(CompilationCachePath
      --compilation-cache-path ${CA_CL_CTS_COMPILATION_CACHE_PATH})
  endif()
  if(${CA_CL_CTS_SPLIT_VECTORS})
    set(CompilationSplitVectors
      --split-vectors)
  endif()
  add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/${output}
    COMMAND ${PYTHON_EXECUTABLE} ${generateCsvFileScript}
      --cl-std ${args_CL_STD} --math-mode ${args_MATH_MODE}
      --compilation-mode ${args_COMPILATION_MODE}
      ${CompilationImages}
      ${CompilationSplitVectors}
      ${CompilationCacheMode} ${CompilationCachePath}
      ${PROJECT_BINARY_DIR}/scripts
    DEPENDS ${generateCsvFileScript} ${input}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Generate CSV file scripts/${output}")
  set(target csv-${args_CL_STD}${ImagesName}-${args_MATH_MODE}-${args_COMPILATION_MODE})
  add_custom_target(${target}
    DEPENDS ${PROJECT_BINARY_DIR}/${output}
    COMMENT "Generate CSV file scripts/${output}")
  add_dependencies(csv-files ${target})
  install(FILES ${PROJECT_BINARY_DIR}/${output}
    DESTINATION bin COMPONENT OpenCL-CTS)
endfunction()
# Target to generate all test input CSV files.
add_custom_target(csv-files ALL)

if("binary" IN_LIST CA_CL_CTS_COMPILATION_MODE)
  if(NOT CA_CL_CTS_CLC_EXECUTABLE)
    message(WARNING
      "CA_CL_CTS_COMPILATION_MODE enables the binary compilation mode but "
      "CA_CL_CTS_CLC_EXECUTABLE is not set, either reconfigure the build "
      "and set CA_CL_CTS_CLC_EXECUTABLE or set the CLC_EXECUTABLE environment "
      "variable when running tests.")
  endif()
endif()

# When spir-v mode is disabled, clang and llvm-spirv are not required.
if("spir-v" IN_LIST CA_CL_CTS_COMPILATION_MODE)
  # Extract the version components from the LLVM version option.
  string(REPLACE "." ";" CACLCTSLLVMVersionList ${CA_CL_CTS_LLVM_VERSION})
  list(LENGTH CACLCTSLLVMVersionList CACLCTSLLVMVersionListLength)
  if(NOT CACLCTSLLVMVersionListLength EQUAL 3)
    message(FATAL_ERROR "invalid version (major.minor.patch) for "
      "CA_CL_CTS_LLVM_VERSION: ${CA_CL_CTS_LLVM_VERSION}")
  endif()
  list(GET CACLCTSLLVMVersionList 0 CA_CL_CTS_LLVM_VERSION_MAJOR)
  list(GET CACLCTSLLVMVersionList 1 CA_CL_CTS_LLVM_VERSION_MINOR)
  list(GET CACLCTSLLVMVersionList 2 CA_CL_CTS_LLVM_VERSION_PATCH)

  find_package(LLVM ${CA_CL_CTS_LLVM_VERSION} COMPONENTS Clang)
  if(LLVM_FOUND)
    set(CLANG_EXECUTABLE "${LLVM_TOOLS_BINARY_DIR}/clang")
  else()
    find_program(CLANG_EXECUTABLE clang-${CA_CL_CTS_LLVM_VERSION_MAJOR})
    if(CLANG_EXECUTABLE STREQUAL "CLANG_EXECUTABLE-NOTFOUND")
      message(FATAL_ERROR "clang executable not found. "
        "Install clang version ${CA_CL_CTS_LLVM_VERSION} on your system, or "
        "configure the build with CMAKE_PREFIX_PATH=/path/to/llvm/install.")
    endif()
  endif()

  if(CA_CL_CTS_LLVM_VERSION VERSION_EQUAL 10 OR
     CA_CL_CTS_LLVM_VERSION VERSION_EQUAL 11)
    # The commit is matched to the Khronos Group OpenCL 3.0 llvm-project fork.
    set(SpirvLLVMTranslatorTag 710d731b470062f95d4da77eb80c013c1fb0ab17)
  else()
    set(SpirvLLVMTranslatorTag llvm_release_${CA_CL_CTS_LLVM_VERSION_MAJOR}0)
  endif()
  message(STATUS "CA-OpenCL-CTS SPIRV-LLVM-Translator Git Tag: "
    "${SpirvLLVMTranslatorTag}")


  # Clone, configure, build, and install the SPIRV-LLVM-Translator project.
  set(SpirvLLVMInstallDir
    ${CMAKE_CURRENT_BINARY_DIR}/SPIRV-LLVM-Translator/install)
  ExternalProject_Add(SPIRV-LLVM-Translator PREFIX SPIRV-LLVM-Translator
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-LLVM-Translator.git
    GIT_TAG ${SpirvLLVMTranslatorTag}
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/SPIRV-LLVM-Translator/build
    INSTALL_DIR ${SpirvLLVMInstallDir}
    CMAKE_ARGS
    $<$<BOOL:${CMAKE_CROSSCOMPILING}>:-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}>
    $<$<NOT:$<BOOL:${CMAKE_CROSSCOMPILING}>>:-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}>
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DLLVM_BUILD_TOOLS=ON)

  add_external_project_clean_target(SPIRV-LLVM-Translator)

  set(LLVM_SPIRV_EXECUTABLE
    ${SpirvLLVMInstallDir}/bin/llvm-spirv${CMAKE_EXECUTABLE_SUFFIX})

  # Install the llvm-spirv executable.
  install(PROGRAMS ${LLVM_SPIRV_EXECUTABLE}
    DESTINATION bin COMPONENT OpenCL-CTS)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
  # Add and install the cl_offline_compiler executable, the default command name
  # specified by the OpenCL-CTS when using binary or spir-v offline compilation
  # modes, it forwards all arguments to compile_offline.py. This is required on
  # Windows as the Python script itself will not work unless it has the .py
  # extension, conversely .exe can be omitted and will operate as expected.
  add_executable(cl_offline_compiler tools/cl_offline_compiler.cpp)
  install(TARGETS cl_offline_compiler DESTINATION bin COMPONENT OpenCL-CTS)
  set(OfflineCompilerScript ${CMAKE_CURRENT_BINARY_DIR}/compile_offline.py)
else()
  set(OfflineCompilerScript ${CMAKE_CURRENT_BINARY_DIR}/cl_offline_compiler)
endif()

if(CA_CL_CTS_COMPILATION_WRAPPER)
  # Install the compilation wrapper when specified.
  install(PROGRAMS ${CA_CL_CTS_COMPILATION_WRAPPER}
    DESTINATION bin COMPONENT OpenCL-CTS)
  # Extract the filename of the compilation wrapper to be used as a relative
  # path from <install>/bin. This value is configured in the file
  # compile_offline.py.in in the next step.
  get_filename_component(CompilationWrapperName
    ${CA_CL_CTS_COMPILATION_WRAPPER} NAME)
  if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    set(CompilationWrapper ${CompilationWrapperName})
  else()
    set(CompilationWrapper ./${CompilationWrapperName})
  endif()
endif()

# Configure and install the compilation script.
configure_file(scripts/compile_offline.py.in ${OfflineCompilerScript})
install(PROGRAMS ${OfflineCompilerScript} DESTINATION bin COMPONENT OpenCL-CTS)

# Create and install test input CSV files.
add_ca_cl_cts_csv_file(CL_STD 3.0 MATH_MODE wimpy COMPILATION_MODE online)
add_ca_cl_cts_csv_file(CL_STD 3.0 MATH_MODE full  COMPILATION_MODE online)
add_ca_cl_cts_csv_file(CL_STD 3.0 MATH_MODE full  COMPILATION_MODE online IMAGES)
if("binary" IN_LIST CA_CL_CTS_COMPILATION_MODE)
  add_ca_cl_cts_csv_file(CL_STD 3.0 MATH_MODE wimpy COMPILATION_MODE binary)
  add_ca_cl_cts_csv_file(CL_STD 3.0 MATH_MODE full COMPILATION_MODE binary)
  add_ca_cl_cts_csv_file(CL_STD 3.0 MATH_MODE full COMPILATION_MODE binary IMAGES)
endif()
if("spir-v" IN_LIST CA_CL_CTS_COMPILATION_MODE)
  add_ca_cl_cts_csv_file(CL_STD 3.0 MATH_MODE wimpy COMPILATION_MODE spir-v)
  add_ca_cl_cts_csv_file(CL_STD 3.0 MATH_MODE full COMPILATION_MODE spir-v)
  add_ca_cl_cts_csv_file(CL_STD 3.0 MATH_MODE full COMPILATION_MODE spir-v IMAGES)
endif()

# Set the OpenCL-Headers directory.
set(OpenCLHeadersSourceDir
  ${CMAKE_CURRENT_SOURCE_DIR}/external/OpenCL-Headers)

if(CA_CL_CTS_ICD_LOADER_INSTALL_DIR)
  set(OpenCLICDLoaderInstallDir ${CA_CL_CTS_ICD_LOADER_INSTALL_DIR})
else()
  # The desired release tag to download, found at
  # https://github.com/KhronosGroup/OpenCL-ICD-Loader/releases
  set(OpenCLICDLoaderTag v2022.01.04)
  # Path to the patch which allows environment variables to be used on Windows.
  set(OpenCLICDLoaderPatch
    ${PROJECT_SOURCE_DIR}/patches/OpenCL-ICD-Loader.patch)
  set(OpenCLICDLoaderSourceDir
    ${CMAKE_CURRENT_BINARY_DIR}/OpenCL-ICD-Loader/src/OpenCL-ICD-Loader)
  # Configure, build, and install the OpenCL-ICD-Loader project.
  set(OpenCLICDLoaderInstallDir
    ${CMAKE_CURRENT_BINARY_DIR}/OpenCL-ICD-Loader/install)
  # The patch command needs a relative path
  file(RELATIVE_PATH OpenCLICDLoaderRelSourceDir ${PROJECT_SOURCE_DIR} ${OpenCLICDLoaderSourceDir})
  file(TO_NATIVE_PATH "${OpenCLICDLoaderRelSourceDir}" OpenCLICDLoaderRelSourceDir)
  ExternalProject_Add(OpenCL-ICD-Loader PREFIX OpenCL-ICD-Loader
    URL https://github.com/KhronosGroup/OpenCL-ICD-Loader/archive/refs/tags/${OpenCLICDLoaderTag}.tar.gz
    PATCH_COMMAND git apply ${OpenCLICDLoaderPatch} --directory ${OpenCLICDLoaderRelSourceDir}
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/OpenCL-ICD-Loader/build
    INSTALL_DIR ${OpenCLICDLoaderInstallDir}
    CMAKE_ARGS
    $<$<BOOL:${CMAKE_CROSSCOMPILING}>:-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}>
    $<$<NOT:$<BOOL:${CMAKE_CROSSCOMPILING}>>:-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}>
    $<$<NOT:$<BOOL:${CMAKE_CROSSCOMPILING}>>:-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}>
    $<$<BOOL:${CA_CL_CTS_BUILD_32_BITS}>:-DCMAKE_C_FLAGS=-m32>
    $<$<BOOL:${CA_CL_CTS_BUILD_32_BITS}>:-DCMAKE_CXX_FLAGS=-m32>
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DOPENCL_ICD_LOADER_HEADERS_DIR=${OpenCLHeadersSourceDir}
    -DOPENCL_ICD_LOADER_SECURE_GETENV=OFF
    -DENABLE_OPENCL30_PROVISIONAL=1
    $<$<PLATFORM_ID:Windows>:-DOPENCL_ICD_LOADER_REQUIRE_WDK=OFF>)
  add_external_project_clean_target(OpenCL-ICD-Loader)
  # Install the OpenCL-ICD-Loader.
  install(DIRECTORY ${OpenCLICDLoaderInstallDir}/
    DESTINATION . COMPONENT OpenCL-CTS)
endif()

set(libclcxxSourceDir ${CMAKE_CURRENT_SOURCE_DIR}/external/libclcxx)
# Install the libclcxx headers.
install(DIRECTORY ${libclcxxSourceDir}/include
  DESTINATION lib/libclcxx COMPONENT OpenCL-CTS)

# The desired release tag to download, found at
# https://github.com/KhronosGroup/OpenCL-CTS
set(OpenCLCTSTag v2024-01-09-04)
# Path to the CTS patch, which allows us to patch out any issues quickly while
# waiting for upstream fixes.
set(OpenCLCTSPatch ${PROJECT_SOURCE_DIR}/patches/OpenCL-CTS.patch)
# Configure, build, but don't install (there's no install target) the
# OpenCL-CTS project.
set(OpenCLCTSSourceDir
  ${CMAKE_CURRENT_BINARY_DIR}/OpenCL-CTS/src/OpenCL-CTS)
set(OpenCLCTSBinaryDir
  ${CMAKE_CURRENT_BINARY_DIR}/OpenCL-CTS/build)
# The patch command needs a relative path
file(RELATIVE_PATH OpenCLCTSRelSourceDir ${PROJECT_SOURCE_DIR} ${OpenCLCTSSourceDir})
file(TO_NATIVE_PATH "${OpenCLCTSRelSourceDir}" OpenCLCTSRelSourceDir)
ExternalProject_Add(OpenCL-CTS PREFIX OpenCL-CTS
  URL https://github.com/KhronosGroup/OpenCL-CTS/archive/refs/tags/${OpenCLCTSTag}.tar.gz
  PATCH_COMMAND git apply ${OpenCLCTSPatch} --directory ${OpenCLCTSRelSourceDir}
  BINARY_DIR ${OpenCLCTSBinaryDir}
  CMAKE_ARGS
  $<$<BOOL:${CMAKE_CROSSCOMPILING}>:-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}>
  $<$<NOT:$<BOOL:${CMAKE_CROSSCOMPILING}>>:-DCMAKE_CXX_COMPILER=${CMAKE_C_COMPILER}>
  $<$<NOT:$<BOOL:${CMAKE_CROSSCOMPILING}>>:-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}>
  $<$<BOOL:${CA_CL_CTS_BUILD_32_BITS}>:-DCMAKE_C_FLAGS=-m32>
  $<$<BOOL:${CA_CL_CTS_BUILD_32_BITS}>:-DCMAKE_CXX_FLAGS=-m32>
  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=.
  $<$<PLATFORM_ID:Linux>:-DCMAKE_BUILD_RPATH="$ORIGIN/../../lib">
  -DKHRONOS_OFFLINE_COMPILER="clang"
  -DCL_LIBCLCXX_DIR="../../lib/libclcxx/include"
  -DCL_INCLUDE_DIR=${OpenCLHeadersSourceDir}
  -DCL_LIB_DIR=${OpenCLICDLoaderInstallDir}/lib
  -DOPENCL_LIBRARIES=OpenCL
  INSTALL_COMMAND "")
if(TARGET SPIRV-LLVM-Translator)
  add_dependencies(OpenCL-CTS SPIRV-LLVM-Translator)
endif()
if(TARGET OpenCL-ICD-Loader)
  add_dependencies(OpenCL-CTS OpenCL-ICD-Loader)
endif()

# Unzip SPIR binaries from zip archives to avoid filesystem dataraces when
# running SPIR tests with City Runner.
set(spirTestNames
  api atomics basic binary_type commonfns compile_and_link conversions
  enum_values geometrics half images_kernel_read_write images_samplerlessRead
  integer_ops kernel_attributes kernel_image_methods math_brute_force printf
  profiling relationals sampler_enumeration select vec_align vec_step)
foreach(spirTestName ${spirTestNames})
  set(spirZipFile spir/${spirTestName}.zip)
  add_custom_command(TARGET OpenCL-CTS POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E tar x ${spirZipFile}
    WORKING_DIRECTORY ${OpenCLCTSBinaryDir}/test_conformance
    COMMENT "Extract ZIP file ${spirZipFile}")
endforeach()

# Generate the SPIR-V binaries from the SPIR-V assembly using the
# python script provided by the CTS.
add_custom_command(TARGET OpenCL-CTS POST_BUILD
  COMMAND ${PYTHON_EXECUTABLE}
  ${OpenCLCTSSourceDir}/test_conformance/spirv_new/assemble_spirv.py
  --source-dir ${OpenCLCTSSourceDir}/test_conformance/spirv_new/spirv_asm
  --output-dir ${OpenCLCTSBinaryDir}/test_conformance/spirv_bin
  COMMENT "Assembling SPIR-V")

add_external_project_clean_target(OpenCL-CTS)
# Install the test_conformance build directory.
install(DIRECTORY ${OpenCLCTSBinaryDir}/test_conformance/
  DESTINATION bin USE_SOURCE_PERMISSIONS
  COMPONENT OpenCL-CTS REGEX [Cc][Mm]ake.* EXCLUDE)
