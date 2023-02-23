# Copyright (C) Codeplay Software Limited. All Rights Reserved.

{% if cookiecutter.llvm_name == "RISCV" -%}
  set(RISCV_TOOLCHAIN_DIR "${CA_LLVM_INSTALL_DIR}" CACHE PATH
    "Path to the clang RISC-V toolchain")
{% endif -%}

# Iterate through all 'HAL_{{cookiecutter.target_name_capitals}}_*' sub-directories.
file(GLOB HAL_{{cookiecutter.target_name_capitals}}_DIR_NAMES FOLLOW_SYMLINKS LIST_DIRECTORIES true "hal_*")

if (DEFINED CA_EXTERNAL_{{cookiecutter.target_name_capitals}}_HAL_DIR)
  list(APPEND HAL_{{cookiecutter.target_name_capitals}}_DIR_NAMES ${CA_EXTERNAL_{{cookiecutter.target_name_capitals}}_HAL_DIR})
endif()

foreach(HAL_{{cookiecutter.target_name_capitals}}_PATH ${HAL_{{cookiecutter.target_name_capitals}}_DIR_NAMES})
  file(RELATIVE_PATH HAL_{{cookiecutter.target_name_capitals}}_DIR ${CMAKE_CURRENT_SOURCE_DIR} ${HAL_{{cookiecutter.target_name_capitals}}_PATH})
  string(REGEX REPLACE "^hal_" "" HAL_{{cookiecutter.target_name_capitals}}_NAME ${HAL_{{cookiecutter.target_name_capitals}}_DIR})

  # Give the option for each HAL that was found to be built or not.
  get_filename_component(HAL_{{cookiecutter.target_name_capitals}}_BASENAME ${CA_HAL_{{cookiecutter.target_name_capitals}}_NAME} NAME)  
  string(TOUPPER ${HAL_{{cookiecutter.target_name_capitals}}_BASENAME} HAL_{{cookiecutter.target_name_capitals}}_NAME_UPPER)

  # If this HAL is the one specified in CA_HAL_{{cookiecutter.target_name_capitals}}_NAME then we want to build it by
  # default. Otherwise it is disabled unless the user explicitly enables it.
  if (DEFINED CA_HAL_{{cookiecutter.target_name_capitals}}_NAME)
    string(TOUPPER ${CA_HAL_{{cookiecutter.target_name_capitals}}_NAME} CA_HAL_{{cookiecutter.target_name_capitals}}_NAME_UPPER)
    if (CA_HAL_{{cookiecutter.target_name_capitals}}_NAME_UPPER STREQUAL HAL_{{cookiecutter.target_name_capitals}}_NAME_UPPER)
      set(DEFAULT_{{cookiecutter.target_name_capitals}}_VAL ON)
    else()
      set(DEFAULT_{{cookiecutter.target_name_capitals}}_VAL OFF)
    endif()
  else()
    set(DEFAULT_{{cookiecutter.target_name_capitals}}_VAL OFF)
  endif()
  option(HAL_{{cookiecutter.target_name_capitals}}_ENABLE_${HAL_{{cookiecutter.target_name_capitals}}_NAME_UPPER} "Enable building the '${HAL_{{cookiecutter.target_name_capitals}}_NAME}' HAL."
    ${DEFAULT_{{cookiecutter.target_name_capitals}}_VAL})

  # Include the current HAL, if configured by the user.
  if (${HAL_{{cookiecutter.target_name_capitals}}_ENABLE_${HAL_{{cookiecutter.target_name_capitals}}_NAME_UPPER}})
    if (EXISTS ${HAL_{{cookiecutter.target_name_capitals}}_PATH}/CMakeLists.txt)
      add_subdirectory(${HAL_{{cookiecutter.target_name_capitals}}_PATH}  ${CMAKE_CURRENT_BINARY_DIR}/hal)
    else()
      # The HAL directory has no CMakeLists.txt so error out as we cannot build it
      message(FATAL_ERROR
        "${HAL_{{cookiecutter.target_name_capitals}}_PATH}/CMakeLists.txt not found - unable to build ${HAL_{{cookiecutter.target_name_capitals}}_NAME}")
    endif()
  endif()
endforeach()

# If we have no hals here we cannot build so error out
# Note if we have compiled an out of tree hal KNOWN_HAL_{{cookiecutter.target_name_capitals}}_DEVICES will be set
get_property(HAL_{{cookiecutter.target_name_capitals}}_DEVICES GLOBAL PROPERTY KNOWN_HAL_DEVICES)
if (NOT HAL_{{cookiecutter.target_name_capitals}}_DEVICES)
  message(FATAL_ERROR
    "No HAL built (either under ${CMAKE_CURRENT_SOURCE_DIR} or defined by KNOWN_HAL_DEVICES)")
endif()
