# Copyright (C) Codeplay Software Limited. All Rights Reserved.

find_package(Lit)
if(NOT Lit_FOUND)
  message(WARNING "spirv-ll lit tests disabled: lit not found "
    "Please install https://pypi.python.org/pypi/lit")
  return()
endif()

find_package(LLVMTool COMPONENTS opt)
if(NOT TARGET LLVM::opt)
  message(WARNING "spirv-ll lit tests disabled: opt not found")
  return()
endif()

find_package(LLVMTool COMPONENTS FileCheck)
if(NOT TARGET LLVM::FileCheck)
  message(WARNING "spirv-ll lit tests disabled: FileCheck not found")
  return()
endif()

find_package(SpirvTools COMPONENTS spirv-as)
if(NOT TARGET spirv::spirv-as)
  message(WARNING "spirv-ll some lit tests unsupported: spirv-as not found")
  return()
endif()

# Create the test/test inputs directories and set the relevent variables
set(INPUTS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# The run.py script is a helper script to easily execute the spirv-ll lit
# tests manually without being required to pass additional arguments to lit.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/run.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/run.py @ONLY)

# Generate the main lit.cfg file, since we need to find the paths to various
# executables the lit tests depend on and this can only be done using generator
# expressions, we must configure the file at build time.
add_ca_configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.cfg
  DEFINED TEST_PATH=${TEST_PATH}
  PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
  SPIRV_LL=$<TARGET_FILE:spirv-ll-tool>
  FILECHECK=$<TARGET_FILE:LLVM::FileCheck>
  OPT=$<TARGET_FILE:LLVM::opt>
  TRIPLE=${TRIPLE}
  LLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR}
  SPIRV_LIT_EXPERIMENTAL=$<BOOL:${CA_ENABLE_SPIRV_LL_EXPERIMENTAL}>
  # TODO(CA-3968): Revert when fixed.
  CMAKE_CROSSCOMPILING=$<BOOL:${CMAKE_CROSSCOMPILING}>
  DEPENDS spirv-ll-tool LLVM::FileCheck LLVM::opt)

add_subdirectory(glsl)
add_subdirectory(spvasm)

add_ca_lit_testsuite(spirv-ll
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ca-common-lit spirv-ll-lit-glsl spirv-ll-lit-spvasm ${CMAKE_CURRENT_BINARY_DIR}/lit.cfg)
