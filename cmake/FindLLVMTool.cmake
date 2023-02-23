# Copyright (C) Codeplay Software Limited. All Rights Reserved.

#[=======================================================================[.rst:
This module searches for command line tools which target the host system
upon which ComputeAorta is being built, this is usually x86_64. These tools
are intended for use in the build process and are searched for in user
specified CMake variable ``CA_BUILTINS_TOOLS_DIR`` then falling back to
``CA_LLVM_INSTALL_DIR``.

This module searches for all LLVM tools defined by the ``COMPONENTS`` keyword
argument to `find_package`_.

.. seealso::
  Depends on :doc:`/cmake/CAPlatform` for ``CA_HOST_EXECUTABLE_SUFFIX``.

.. rubric:: Targets

This module adds the following targets:

LLVM::<component>
  For each specifed component

.. rubric:: Variables

When all components are found the following variables are set:

.. cmake:variable:: LLVMTool_FOUND

  Set to ``TRUE`` when all components are found, ``FALSE`` otherwise

.. cmake:variable:: LLVMTool_DIR

  Set to the root directory where components are found


When a component was requested and is found the following are defined per
component:

.. cmake:variable:: LLVMTool_<component>_FOUND

  A boolean variable set to ``TRUE`` if found and ``FALSE`` otherwise

.. cmake:variable:: LLVMTool_<component>_EXECUTABLE

  A variable containing the absolute path to the component

.. rubric:: Usage

.. code:: cmake

  # Finds the FileCheck LLVM tool for testing
  find_package(LLVMTool COMPONENTS FileCheck)

.. _find_package:
  https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html
#]=======================================================================]

if(EXISTS ${CA_BUILTINS_TOOLS_DIR})
  set(LLVMTool_DIR ${CA_BUILTINS_TOOLS_DIR})
elseif(EXISTS ${CA_LLVM_INSTALL_DIR}/bin)
  set(LLVMTool_DIR ${CA_LLVM_INSTALL_DIR}/bin)
endif()

set(required LLVMTool_DIR)

include(CAPlatform)  # Bring in CA_HOST_EXECUTABLE_SUFFIX.

foreach(component ${LLVMTool_FIND_COMPONENTS} ${LLVMTool_FIND_REQUIRED})
  set(LLVMTool_${component}_FOUND FALSE)
  list(APPEND required LLVMTool_${component}_EXECUTABLE)

  # Check if LLVM binary directory has changed (because of a reconfigure)
  if(NOT "${LLVMTool_${component}_EXECUTABLE}" STREQUAL
        "${CA_BUILTINS_TOOLS_DIR}/${component}${CA_HOST_EXECUTABLE_SUFFIX}" OR
      NOT "${LLVMTool_${component}_EXECUTABLE}" STREQUAL
        "${CA_LLVM_INSTALL_DIR}/bin/${component}${CA_HOST_EXECUTABLE_SUFFIX}")
    unset(LLVMTool_${component}_EXECUTABLE CACHE)
  endif()

  # If the component executable variable has not been set search for it.
  if(DEFINED LLVMTool_${component}_EXECUTABLE AND
      EXISTS LLVMTool_${component}_EXECUTABLE)
    set(LLVMTool_${component}_FOUND TRUE)
  else()
    find_program(LLVMTool_${component}_EXECUTABLE
      ${component}${CA_HOST_EXECUTABLE_SUFFIX}
      PATHS ${CA_BUILTINS_TOOLS_DIR} ${CA_LLVM_INSTALL_DIR}/bin
      NO_DEFAULT_PATH)
  endif()

  if(LLVMTool_${component}_EXECUTABLE STREQUAL
      LLVMTool_${component}_EXECUTABLE-NOTFOUND)
    # Display help when components are not found and QUIET is not set.
    if(${component} STREQUAL FileCheck)
      message(WARNING "FileCheck was not found, LLVM does not install "
        "FileCheck  by default, you must specify -DLLVM_INSTALL_UTILS=ON "
        "when  configuring LLVM's CMake, recompile LLVM or set "
        "LLVMTool_FileCheck_EXECUTABLE manually whilst reconfiguring CMake.")
    else()
      message(WARNING "${component} was not found, set "
        "LLVMTool_${component}_EXECUTABLE manually whilst reconfiguring "
        "CMake.")
    endif()
    # Search for the next component.
    continue()
  endif()

  set(LLVMTool_${component}_FOUND TRUE)

  set(target LLVM::${component})
  if(NOT TARGET ${target})
    add_executable(${target} IMPORTED GLOBAL)
    set_target_properties(${target} PROPERTIES
      IMPORTED_LOCATION ${LLVMTool_${component}_EXECUTABLE})
  endif()
endforeach()

set(LLVMTool_FOUND TRUE)
foreach(component ${LLVMTool_FIND_COMPONENTS})
  if(NOT LLVMTool_${component}_FOUND)
    set(LLVMTool_FOUND FALSE)
    break()
  endif()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LLVMTool
  REQUIRED_VARS ${required} HANDLE_COMPONENTS)
mark_as_advanced(${required})
