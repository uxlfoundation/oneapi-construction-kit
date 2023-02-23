# Copyright (C) Codeplay Software Limited. All Rights Reserved.

set(CLIK_MUX_INCLUDE_DIR "" CACHE PATH "Path to Mux headers")
set(CLIK_MUX_MAIN_HEADER "${CLIK_MUX_INCLUDE_DIR}/core/core.h")
if(NOT EXISTS ${CLIK_MUX_MAIN_HEADER})
  message(FATAL_ERROR "Could not find '${CLIK_MUX_MAIN_HEADER}'. Has CLIK_MUX_INCLUDE_DIR been set with a valid path?")
endif()

set(CLIK_MUX_LIBRARY_PATH "" CACHE FILEPATH "Path to a Mux library")
if(NOT EXISTS ${CLIK_MUX_LIBRARY_PATH})
  message(FATAL_ERROR "Could not find '${CLIK_MUX_LIBRARY_PATH}'. Has CLIK_MUX_LIBRARY_PATH been set with a valid path?")
endif()

set(CLIK_MUX_EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR})

function(add_mux_example TARGET)
  add_example(${TARGET}
    ${ARGN}
    ${CLIK_MUX_EXAMPLES_DIR}/common/mux_util.cpp
  )
  target_include_directories(${TARGET} PRIVATE
    ${CLIK_MUX_INCLUDE_DIR}
    ${CLIK_MUX_EXAMPLES_DIR}/common
  )
  target_link_libraries(${TARGET} PRIVATE option_parser ${CLIK_MUX_LIBRARY_PATH})
  target_compile_definitions(${TARGET} PRIVATE -DMUX_HAL_DEVICE="${CLIK_HAL_NAME}")
endfunction()

install(FILES ${CLIK_MUX_LIBRARY_PATH} DESTINATION lib)

add_subdirectory(hello)
add_subdirectory(copy)
add_subdirectory(vector_add)
add_subdirectory(barrier_print)
add_subdirectory(barrier_sum)
add_subdirectory(matrix_multiply)
add_subdirectory(matrix_multiply_tiled)
