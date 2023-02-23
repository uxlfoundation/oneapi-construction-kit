# Copyright (C) Codeplay Software Limited. All Rights Reserved.

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(BenchCL)
  cmake_minimum_required(VERSION 3.4.3)

  set(BENCHCL_OPENCL_LIBRARY "" CACHE PATH
    "Path to the OpenCL library to link against.")

  get_filename_component(BENCHCL_OPENCL_LIBRARY_ABSOLUTE
    ${BENCHCL_OPENCL_LIBRARY} ABSOLUTE)
  get_filename_component(BENCHCL_OPENCL_LIBRARY_NAME_WITH_LIB
    ${BENCHCL_OPENCL_LIBRARY_ABSOLUTE} NAME_WE)
  string(SUBSTRING ${BENCHCL_OPENCL_LIBRARY_NAME_WITH_LIB} 3 -1
    BENCHCL_OPENCL_LIBRARY_NAME)

  add_ca_library_import(${BENCHCL_OPENCL_LIBRARY_NAME} SHARED
    ${BENCHCL_OPENCL_LIBRARY_ABSOLUTE})

  set(OPENCL_LIBRARY ${BENCHCL_OPENCL_LIBRARY_NAME})

  if(BENCHCL_OPENCL_LIBRARY AND NOT ${BENCHCL_OPENCL_LIBRARY} STREQUAL "")
    # We've been provided a BENCHCL_OPENCL_LIBRARY to link against
    target_link_libraries(BenchCL ${BENCHCL_OPENCL_LIBRARY})
  else()
    message(FATAL_ERROR
      "Required cmake variable 'BENCHCL_OPENCL_LIBRARY' was not set!")
  endif()

else()
  if(BENCHCL_LIBRARY AND NOT ${BENCHCL_LIBRARY} STREQUAL "")
    set(OPENCL_LIBRARY ${BENCHCL_LIBRARY})
  else()
    message(FATAL_ERROR
      "Parent CMakeLists did not set 'BENCHCL_LIBRARY' to a valid value!")
  endif()
endif()

# Add any external benchmarks
foreach(NAME ${MUX_TARGET_LIBRARIES})
  list(APPEND CA_EXTERNAL_BENCHCL_SRC ${${NAME}_EXTERNAL_BENCHCL_SRC})
endforeach()

# Our benchmarking executable
add_ca_executable(BenchCL
  ${CMAKE_CURRENT_SOURCE_DIR}/include/BenchCL/error.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/BenchCL/environment.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/BenchCL/utils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/kernel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/program.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/queue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/buffer.cpp
  ${CA_EXTERNAL_BENCHCL_SRC})

target_link_libraries(BenchCL PRIVATE cargo)

target_compile_definitions(BenchCL PRIVATE ${CA_CL_COMPILE_DEFINITIONS})

# TODO(CA-1105): BenchCL is using deprecated google-benchmark calls, once they
# are replaced this flag should be removed.
target_compile_options(BenchCL PRIVATE
  $<$<OR:$<BOOL:${UNIX}>,$<BOOL:${ANDROID}>,$<BOOL:${MINGW}>>:
    -Wno-deprecated-declarations  # Disable: warnings about deprecated decls
  >)

target_include_directories(BenchCL PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

foreach(NAME ${MUX_TARGET_LIBRARIES})
  target_include_directories(BenchCL PRIVATE ${${NAME}_EXTERNAL_BENCHCL_INC})
endforeach()

# link against Google benchmark library
target_link_libraries(BenchCL PRIVATE ${OPENCL_LIBRARY} cargo ca-benchmark)

install(TARGETS BenchCL RUNTIME DESTINATION bin COMPONENT BenchCL)

foreach(NAME ${MUX_TARGET_LIBRARIES})
  if(${NAME}_EXTERNAL_BENCHCL_DEPS)
    add_dependencies(BenchCL ${${NAME}_EXTERNAL_BENCHCL_DEPS})
  endif()
endforeach()
