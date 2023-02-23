# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# If the online coverage is enabled we add the modules so that the XML file
# can be generated automatically.
if(${CA_ENABLE_COVERAGE} AND ${CA_RUNTIME_COMPILER_ENABLED})
  add_coverage_modules(${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/tracer.dir/source
    ${CMAKE_CURRENT_SOURCE_DIR}/include/tracer
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/tracer.dir/source)
endif()

add_ca_library(tracer STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/tracer/tracer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/tracer.cpp)

target_include_directories(tracer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Toggle various tracing options
ca_option(CA_TRACE_CL BOOL "Enable tracing OpenCL entry points" OFF)
ca_option(CA_TRACE_CORE BOOL "Enable tracing Core details" OFF)
ca_option(CA_TRACE_IMPLEMENTATION BOOL "Enable tracing of the Implementation details" OFF)

target_compile_definitions(tracer PUBLIC
  $<$<BOOL:${CA_TRACE_CL}>:CA_TRACE_CL=1>
  $<$<BOOL:${CA_TRACE_CORE}>:CA_TRACE_CORE=1>
  $<$<BOOL:${CA_TRACE_IMPLEMENTATION}>:CA_TRACE_IMPLEMENTATION=1>)

target_link_libraries(tracer PRIVATE utils)

# Append to the list of module libraries, the cache MUST be updated.
list(APPEND MODULES_LIBRARIES tracer)
set(MODULES_LIBRARIES ${MODULES_LIBRARIES}
  CACHE INTERNAL "List of module libraries.")
