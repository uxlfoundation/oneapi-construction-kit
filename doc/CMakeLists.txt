# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

find_program(DOXYGEN doxygen)
if(DOXYGEN STREQUAL "DOXYGEN-NOTFOUND")
  message(WARNING
    "doxygen not found, documentation generation disabled")
  return()
endif()

find_program(VIRTUALENV virtualenv REQUIRED)
if(VIRTUALENV STREQUAL "VIRTUALENV-NOTFOUND")
  message(WARNING
    "virtualenv not found, documentation generation disabled, "
    "try installing virtualenv: pip install virtualenv")
  return()
endif()

# Configure the Python virtualenv at CMake configure time as this is only
# required to occur once, the environment will be used to install packages
# listed in doc/requirements.txt which are required to build the documentation
# without changing the users Python configuration.
if(NOT IS_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/env)
  execute_process(
    COMMAND ${VIRTUALENV} env -p ${PYTHON_EXECUTABLE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} OUTPUT_QUIET)
endif()

# To avoid activating the virtualenv use the installed tools directly with
# absolute paths, tools are installed in a different location on Windows.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  set(ENV_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/env/Scripts)
else()
  set(ENV_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/env/bin)
endif()

# Check that pip is installed in the virtualenv.
set(PIP ${ENV_BINARY_DIR}/pip${CMAKE_EXECUTABLE_SUFFIX})
if(NOT EXISTS ${PIP})
  message(WARNING
    "pip not found in virtualenv, documentation generation disabled")
  return()
endif()

# Install the documentation build requirements into the virtualenv created
# above, this will not affect the users global Python configuration.
execute_process(
  COMMAND ${PIP} install -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} OUTPUT_QUIET ERROR_QUIET)

# Check that sphinx-build is installed in the virtualenv.
set(SPHINX_BUILD
  ${ENV_BINARY_DIR}/sphinx-build${CMAKE_EXECUTABLE_SUFFIX})
if(NOT EXISTS ${SPHINX_BUILD})
  message(WARNING
    "sphinx-build not found in virtualenv, documentation generation disabled")
  return()
endif()

# Configure paths in documentation config files.
configure_file(Doxyfile.in Doxyfile @ONLY)
configure_file(conf.py.in ${CMAKE_CURRENT_SOURCE_DIR}/conf.py @ONLY)

# Target to generate XML from Doxygen comments.
add_custom_target(doxygen
  COMMAND ${DOXYGEN}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  USES_TERMINAL COMMENT "Generate Doxygen XML")
add_dependencies(doxygen abacus_generate)

# Copy out-of-tree documentation sources to the doc directory.
add_custom_target(doc-copy
  COMMENT "Copying documentation sources")
if(EXISTS ${PROJECT_SOURCE_DIR}/modules/mux/external/HAL/docs)
  add_custom_target(doc-copy-hal
    COMMAND ${CMAKE_COMMAND} -E copy
      ${PROJECT_SOURCE_DIR}/modules/mux/external/HAL/docs/hal.md
      ${CMAKE_CURRENT_SOURCE_DIR}/specifications/hal.md
    COMMAND ${CMAKE_COMMAND} -E make_directory
      ${CMAKE_CURRENT_SOURCE_DIR}/modules/riscv/hal
    COMMAND ${CMAKE_COMMAND} -E copy
      ${PROJECT_SOURCE_DIR}/modules/mux/external/HAL/docs/dynamic_loading.md
      ${CMAKE_CURRENT_SOURCE_DIR}/modules/mux/hal/dynamic_loading.md)
  add_dependencies(doc-copy doc-copy-hal)
endif()

# Stand alone target to build the HTML documentation without first building the
# Doxygen XML, useful for iterating on documentation.
add_custom_target(sphinx-build
  COMMAND ${SPHINX_BUILD} --color -b html
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/html
  DEPENDS doc-copy
  USES_TERMINAL COMMENT "Build Sphinx Documentation")
set(SPHINX_AUTOBUILD ${ENV_BINARY_DIR}/sphinx-autobuild${CMAKE_EXECUTABLE_SUFFIX})
if(EXISTS ${SPHINX_AUTOBUILD})
  add_custom_target(sphinx-autobuild
    COMMAND ${SPHINX_AUTOBUILD} -b html
    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/html
    DEPENDS doc-copy
    USES_TERMINAL COMMENT "Build/Serve Sphinx Documentation (Live Preview)")
endif()

# Target to build HTML documentation, to view generated documentation open
# ${PROJECT_BINARY_DIR}/doc/html/index.html in your browser.
add_custom_target(doc_html
  COMMAND ${SPHINX_BUILD} --color -b html
    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/html
  DEPENDS doc-copy doxygen
  USES_TERMINAL COMMENT "Generate Documentation HTML")

# PDF documentation generatation is not currently supported on Windows since
# Sphinx generates a Makefile to perform the PDF generation.
if(NOT CMAKE_SYSTEM_NAME STREQUAL Windows)
  # Finding make shouldn't fail on non Windows systems but check anyway.
  find_program(MAKE make)
  if(MAKE STREQUAL "MAKE-NOTFOUND")
    message(WARNING "make not found, PDF documentation generation disabled")
    return()
  endif()

  # Sphinx generates a Makefile which depends on latexmk when generating PDF
  # documentation, check its available on the system even though we don't call
  # it directly.
  find_program(LATEXMK latexmk)
  if(LATEXMK STREQUAL "LATEXMK-NOTFOUND")
    message(WARNING "latexmk not found, PDF documentation generation disabled, "
      "try installing latexmk: sudo apt-get install latexmk")
    return()
  endif()

  # Check that the system has latex installed by checking if the tex command
  # exists on the system even though we don't call it directly.
  find_program(TEX tex)
  if(TEX STREQUAL "TEX-NOTFOUND")
    message(WARNING "tex not found, PDF documentation generation disabled, "
      "try installing latex: sudo apt-get install texlive-full")
    return()
  endif()

  # Create the PDF output directory so that first build of the doc_pdf target
  # won't fail when removing it.
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pdf)

  # Target to build PDF documentation, to view generated documentation open
  # ${PROJECT_BINARY_DIR}/doc/pdf/ComputeAorta.pdf in your PDF viewer.
  add_custom_target(doc_pdf
    # Remove existing latex files, not doing so can cause rebuild failures.
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/pdf
    COMMAND ${SPHINX_BUILD} --color -b latex
      ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/pdf
    COMMAND make -C ${CMAKE_CURRENT_BINARY_DIR}/pdf
    DEPENDS doxygen
    USES_TERMINAL COMMENT "Generate Documentation PDF")
endif()
