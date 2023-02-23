# Copyright (C) Codeplay Software Limited. All Rights Reserved.

include(GLSL)

# GLSL shader
set(comp ${CMAKE_CURRENT_SOURCE_DIR}/source/vector_add.comp)
if(NOT EXISTS ${comp})
  message(FATAL_ERROR "${comp} doesn't exist!")
endif()

# Header to be generated containing SPIR-V binary
set(header ${CMAKE_CURRENT_BINARY_DIR}/vector_add.h)

# Compile GLSL to create SPIR-V, then bin2h to convert it to a header
add_glsl_bin2h_command(vector_add_shader ${comp} ${header})
add_custom_target(vector_add_shader DEPENDS ${header})

add_ca_executable(vkVectorAddition
  ${CMAKE_CURRENT_SOURCE_DIR}/source/main.c ${header})
target_link_libraries(vkVectorAddition PRIVATE VK)

# Look in binary directory for generated header
target_include_directories(vkVectorAddition PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR})

if(TARGET tidy-vkVectorAddition)
  add_dependencies(tidy-vkVectorAddition vector_add_shader)
endif()

add_ca_check(vkVectorAddition
  COMMAND vkVectorAddition
  DEPENDS vkVectorAddition)

install(TARGETS vkVectorAddition RUNTIME DESTINATION bin COMPONENT VKExamples)
