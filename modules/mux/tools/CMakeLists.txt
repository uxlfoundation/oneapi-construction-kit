# Copyright (C) Codeplay Software Limited. All Rights Reserved.

file(GLOB CHILDREN RELATIVE
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

foreach(CHILD ${CHILDREN})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${CHILD})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${CHILD}/CMakeLists.txt)
      add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/${CHILD})
    endif()
  endif()
endforeach()
