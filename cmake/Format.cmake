# Copyright (C) Codeplay Software Limited. All Rights Reserved.

#[=======================================================================[.rst:
Module adding a ``format`` CMake target to automatically format modified source
files using `clang-format`_.

To add the target to a project include this module in the root
``CMakeLists.txt``.

.. code:: cmake

  include(Format)

Users can run the target to format modified files like so:

.. code:: console

  $ ninja format
  [0/1] clang-format modified source files.

.. _clang-format:
  https://clang.llvm.org/docs/ClangFormat.html
#]=======================================================================]

# Don't include this file twice.
if(${GIT_CLANG_FORMAT_INCLUDED})
  return()
endif()
set(GIT_CLANG_FORMAT_INCLUDED TRUE)

find_package(GitClangFormat)
if(NOT PythonInterp_FOUND OR NOT TARGET ClangTools::clang-format OR
    NOT GitClangFormat_FOUND)
  message(WARNING "Dependencies for format target not met, disabled.")
  return()
endif()

# Add format target to clang-format modified source files.
add_custom_target(format
  COMMAND ${PYTHON_EXECUTABLE} ${GitClangFormat_EXECUTABLE}
    --binary ${ClangTools_clang-format_EXECUTABLE} --style=file --force
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  COMMENT "clang-format modified source files.")

# TODO(CA-692): Add support for prettier to format markdown documents, this
# will require finding the prettier tool and scanning git status for editing
# .md files to format.
