# Copyright (C) Codeplay Software Limited. All Rights Reserved.

if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT WIN32 AND NOT CA_BUILD_32_BITS)
  set(CMAKE_CL_64 true)
endif()

if(CA_CL_ENABLE_ICD_LOADER)
  set(CLConform_HEADERS ${CL_INCLUDE_DIR})
  set(CLConform_LIBRARIES OpenCL)
else()
  set(CLConform_LIBRARIES CL)
endif()
set(BENCHCL_LIBRARY CL)
set(CLConf_OUT_DIR bin)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/UnitCL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/MultiDevice)
if(CA_RUNTIME_COMPILER_ENABLED)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/GeneratedTests)
endif()
if(TARGET ca-benchmark)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/BenchCL)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/FuzzCL)
