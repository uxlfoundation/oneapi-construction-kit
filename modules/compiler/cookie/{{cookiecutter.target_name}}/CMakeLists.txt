# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# If the {{cookiecutter.target_name}} target is disabled, then the compiler should be too.
if(NOT TARGET {{cookiecutter.target_name}})
  return()
endif()

if(CA_RUNTIME_COMPILER_ENABLED)
{% if "clmul"  in cookiecutter.feature.split(";") -%}
  add_subdirectory(extension)
{% endif %}

  set({{cookiecutter.target_name_capitals}}_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/source/{{cookiecutter.target_name}}_pass_machinery.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/{{cookiecutter.target_name}}_pass_registry.def
  ${CMAKE_CURRENT_SOURCE_DIR}/source/target.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/info.cpp
{% if "refsi_wrapper_pass"  in cookiecutter.feature.split(";") -%}
${CMAKE_CURRENT_SOURCE_DIR}/source/passes/refsi_wrapper_pass.cpp
${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/refsi_wrapper_pass.h
{% endif %} 
  ${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/info.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/module.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/target.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/{{cookiecutter.target_name}}_pass_machinery.h 
  )

  add_ca_library(compiler-{{cookiecutter.target_name}} STATIC ${{"{"}}{{cookiecutter.target_name_capitals}}_SOURCES})

  ca_option(CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE BOOL "Enable ComputeAorta {{cookiecutter.target_name_capitals}} demo mode" OFF)
  if(CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE)
  target_compile_definitions(compiler-{{cookiecutter.target_name}} PRIVATE
    CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE=$<BOOL:${{"{"}}CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE}>)
endif()

  target_include_directories(compiler-{{cookiecutter.target_name}} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  target_link_libraries(compiler-{{cookiecutter.target_name}} PUBLIC
    compiler-base
    utils
    compiler-utils
    compiler-linker-utils
    LLVMCoverage LLVMDebugInfoCodeView LLVMExecutionEngine
    LLVMMCJIT LLVMVectorize LLVMipo multi_llvm)
  
  add_mux_compiler_target(compiler-{{cookiecutter.target_name}}
    COMPILER_INFO {{cookiecutter.target_name}}::{{cookiecutter.target_name.capitalize()}}Info::get
    HEADER_FILE {{cookiecutter.target_name}}/info.h)

  if(CA_ENABLE_TESTS)
    add_subdirectory(test)
  endif()
endif()
