# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Only build tools if a runtime compiler is present
if(CA_RUNTIME_COMPILER_ENABLED)
  add_subdirectory(clc)
  add_subdirectory(oclc)
endif()

# If offline kernels aren't being built, an external clc **may** not be given
if(CA_EXTERNAL_CLC)
  # A native, offline-only build uses a drop-in external clc
  if(NOT CMAKE_CROSSCOMPILING AND NOT CA_RUNTIME_COMPILER_ENABLED)
    add_ca_executable_import(clc "${CA_EXTERNAL_CLC}")
  endif()

  # When cross-compiling, the `clc` binary is built for the target arch. A
  # native one may also be required.
  if(CMAKE_CROSSCOMPILING)
    add_ca_executable_import(clc_native "${CA_EXTERNAL_CLC}")
  endif()
endif()

option(OCL_ENABLE_TREACL "Enable building TreaCL if present." ON)
if(${OCL_ENABLE_TREACL} AND IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/TreaCL)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/TreaCL)
endif()
