# Copyright (C) Codeplay Software Limited. All Rights Reserved.

#[=======================================================================[.rst:
Module defining the list of supported LLVM sanitizers and exposing CMake
commands to enable building ComputeAorta with them.

To access the commands and variables in this module:

.. code:: cmake

  include(Sanitizers)

#]=======================================================================]

# Only include this module once.
if(${SANITIZERS_CMAKE})
  return()
endif()
set(SANITIZERS_CMAKE INCLUDED)

#[=======================================================================[.rst:
.. cmake:variable:: CA_SUPPORTED_SANITIZERS

  Variable storing the list of sanitizers known to work:

  * Address - Enables LLVM `ASAN`_.
  * Thread - Enables LLVM `TSAN`_.
  * Undefined - Enables LLVM `UBSAN`_.
  * Fuzzer - Enables LLVM `libFuzzer`_.
  * Address,Undefined - Enables both `ASAN`_ and `UBSAN`_

  We have additionally tested:

  * Memory - The LLVM `MemorySanitizer`_ is not yet available as LLVM requires
    system libraries to be built with the sanitizer.

.. _ASAN:
  https://clang.llvm.org/docs/AddressSanitizer.html
.. _TSAN:
  https://clang.llvm.org/docs/ThreadSanitizer.html
.. _UBSAN:
  https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
.. _MemorySanitizer:
  https://clang.llvm.org/docs/MemorySanitizer.html
.. _libFuzzer:
  https://llvm.org/docs/LibFuzzer.html
#]=======================================================================]
set(CA_SUPPORTED_SANITIZERS "Address" "Thread" "Undefined" "Fuzzer"
  "Address,Undefined")

#[=======================================================================[.rst:
.. cmake:command:: ca_enable_sanitizer

  A function verifying that a valid sanitizer is requested and checking if
  other internal settings, e.g. the used compiler, work with the sanitizer.
  Manipulation of compilation flags to enable sanitization is then performed.

  LLVM accepts sanitizer settings via the CMake variable
  ``LLVM_USE_SANITIZER:STRING`` and only accepts sanitizer names with the first
  character in upper case. To simplify build scripting we also require
  sanitizer names to start with an upper case character, unifying sanitizer
  settings with LLVM. However, as ComputeAorta can be used independently from
  LLVM the sanitizer handling of both is independent.

  Using a function for sanitizer setup limits the scope of helper variables,
  however we set the following variables in parent scope if required:

  * `CMAKE_C_FLAGS`_
  * `CMAKE_CXX_FLAGS`_
  * :cmake-variable:`CMAKE_EXE_LINKER_FLAGS`
  * :cmake-variable:`CMAKE_MODULE_LINKER_FLAGS`
  * :cmake-variable:`CMAKE_SHARED_LINKER_FLAGS`

  The function sets the following cached variable if a sanitizer is used:

  * ``LLVM_USE_SANITIZER`` - Exports the sanitizer settings to LLVM

  Arguments:
    * ``SANITIZER`` - The name of the sanitizer from
      :cmake:variable:`CA_SUPPORTED_SANITIZERS`

.. _CMAKE_C_FLAGS:
  https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_FLAGS.html
.. _CMAKE_CXX_FLAGS:
  https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_FLAGS.html
#]=======================================================================]
function(ca_enable_sanitizer SANITIZER)
  # Redmine 5311: Suppress linker error on sanitizer builds:
  set(LLVM_ENABLE_BACKTRACES OFF CACHE BOOL
    "Enable embedding backtraces on crash." FORCE)

  # Check if compiler is known.
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND
      NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "Sanitizer '${SANITIZER}' not supported with compiler "
      "'${CMAKE_CXX_COMPILER_ID}'.")
  endif()

  # To avoid confusion with LLVM's equivalent, we allow the same
  # Address;Undefined, forms on input but it's a pain to work with in cmake, and
  # on the shell, so normalize it as early as possible
  string(REPLACE ";" "," SANITIZER "${SANITIZER}")

  # Check if sanitizer is known.
  set(SUPPORTED_SANITIZER_INDEX -1)
  list(FIND CA_SUPPORTED_SANITIZERS "${SANITIZER}" SUPPORTED_SANITIZER_INDEX)
  if(-1 EQUAL ${SUPPORTED_SANITIZER_INDEX})
    message(FATAL_ERROR "Sanitizer '${SANITIZER}' not supported, use one of: "
      "${CA_SUPPORTED_SANITIZERS}.")
  endif()

  # Thread sanitizer only works on x86_64 architectures.
  if(SANITIZER STREQUAL "Thread" AND NOT CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(CMAKE_SYSTEM_PROCESSOR_TOUPPER "")
    string(TOUPPER "${CMAKE_SYSTEM_PROCESSOR}" CMAKE_SYSTEM_PROCESSOR_TOUPPER)
    if(NOT CMAKE_SYSTEM_PROCESSOR_TOUPPER STREQUAL "X86_64")
      message(FATAL_ERROR "'Thread' sanitizer only works on x86_64 "
        "target architectures (see CMAKE_SYSTEM_PROCESSOR: "
        "${CMAKE_SYSTEM_PROCESSOR}).")
    endif()
  endif()

  # Begin set up of sanitizer related compiler flags.
  set(SANITIZER_TOLOWER "")
  string(TOLOWER "${SANITIZER}" SANITIZER_TOLOWER)
  set(SANITIZER_FLAGS "-fsanitize=${SANITIZER_TOLOWER}")

  if(SANITIZER STREQUAL "Fuzzer")
    set(SANITIZER_FLAGS "-fsanitize=fuzzer-no-link")
  endif()

  if(SANITIZER MATCHES Undefined)
    # When using the clang-3.5 provided in Ubuntu 14.04 to build
    # ComputeAorta in an undefined sanitizer build (which includes
    # Clang/LLVM 3.8 at time of writing) there were many issues with
    # segfaults at runtime.  Those issues do not occur with clang-3.6, so
    # check for that as a minimum version.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
        CMAKE_CXX_COMPILER_VERSION VERSION_LESS "3.6.0")
      message(FATAL_ERROR
        "Detected clang-${CMAKE_CXX_COMPILER_VERSION}, but 3.6.0 or later is "
        "required for '${SANITIZER}' sanitizer builds.")
    endif()
    # By default the undefined behaviour sanitizer just prints a message and
    # continues, it doesn't even change the program exit code, this is quite
    # hard to follow with our testing, so just exit on the first issue found.
    string(APPEND SANITIZER_FLAGS " -fno-sanitize-recover=all")

    # The vptr sanitizer is not compatible with -fno-rtti.
    string(APPEND SANITIZER_FLAGS " -fno-sanitize=vptr")

    # The function sanitizer is tough on OpenCL callbacks.  It was
    # generating false positives where function pointers are used as
    # parameters because although the types matched, at runtime this cannot
    # be checked because the type is not exposed in libOpenCL.so.  If we
    # export the types then the sanitizer works, but we do not want to
    # export implementation details, so disable the sanitizer.
    string(APPEND SANITIZER_FLAGS " -fno-sanitize=function")

    # Our test-suites deliberately use invalid enum values to check that we
    # correctly return error codes.
    string(APPEND SANITIZER_FLAGS " -fno-sanitize=enum")

    # View the blacklist file itself for individual justifications, but in
    # general code that resulted in many errors but did not originate from
    # Codeplay is blacklisted.
    string(APPEND SANITIZER_FLAGS
      " -fsanitize-blacklist=${PROJECT_SOURCE_DIR}/scripts/jenkins/ubsan_blacklist.txt")
  endif()

  message(STATUS "Using sanitizer: ${SANITIZER}")

  # Sanitizer options and flags
  #
  # Address sanitizer documentation:
  # http://clang.llvm.org/docs/AddressSanitizer.html
  # https://web.archive.org/web/20150713070812/http://code.google.com/p/address-sanitizer/wiki/AddressSanitizer
  # https://web.archive.org/web/20150714071456/https://code.google.com/p/address-sanitizer/wiki/Flags
  #
  # Thread-sanitizer documentation:
  # http://clang.llvm.org/docs/ThreadSanitizer.html
  # https://web.archive.org/web/20140914041924/http://code.google.com/p/thread-sanitizer/
  # https://web.archive.org/web/20140911183419/https://code.google.com/p/thread-sanitizer/wiki/Flags
  #
  # No -Wl,--no-undefined
  # https://groups.google.com/forum/#!topic/thread-sanitizer/U54eYyn85jc
  #
  # Meaning of PIC (PIE implies PIC)
  # http://stackoverflow.com/questions/18026333/what-does-compiling-with-pic-dwith-pic-with-pic-actually-do
  #
  # Info about PIE
  # http://stackoverflow.com/questions/2463150/fpie-position-independent-executable-option-gcc-ld
  #
  # Info about --allow-shlib-undefined
  # http://stackoverflow.com/questions/23235114/linking-errors-with-wl-no-undefined-wl-no-allow-shlib-undefined

  # Non positional independent executables are not supported in the Thread
  # sanitizer. Don't omit a frame pointer for nicer stack traces in error
  # messages from the Address sanitizer.
  set(SANITIZER_COMPILE_FLAGS
    "${SANITIZER_FLAGS} -fPIE -fno-omit-frame-pointer")
  # Even in debug mode apply a bit of optimization to get any kind of
  # performance out of the sanitizer build.
  set(SANITIZER_COMPILE_FLAGS_DEBUG "-O1 -fno-optimize-sibling-calls")
  # Regardless if normal/debug or release builds, always ensure that a minimal
  # amount of debug information is created to enable useful output from the
  # sanitizers.
  set(SANITIZER_COMPILE_FLAGS_RELEASE "-g -gline-tables-only")

  set(SANITIZER_EXE_LINKER_FLAGS "${SANITIZER_FLAGS} -pie")
  # Settings seemingly implicit when linking: -pie -Wl,--allow-shlib-undefined
  set(SANITIZER_MODULE_LINKER_FLAGS "${SANITIZER_FLAGS}")
  set(SANITIZER_SHARED_LINKER_FLAGS "${SANITIZER_FLAGS}")

  # Export CMake-wide settings into the parent scope.
  # Differentiating between normal/debug and release builds, e.g., to enable
  # sanitizer release builds for faster testing.
  set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} ${SANITIZER_COMPILE_FLAGS}"
    PARENT_SCOPE)
  set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_FLAGS_DEBUG} ${SANITIZER_COMPILE_FLAGS_DEBUG}"
    PARENT_SCOPE)
  set(CMAKE_C_FLAGS_RELEASE
    "${CMAKE_C_FLAGS_RELEASE} ${SANITIZER_COMPILE_FLAGS_RELEASE}"
    PARENT_SCOPE)
  set(CMAKE_C_FLAGS_MINSIZEREL
    "${CMAKE_C_FLAGS_MINSIZEREL} ${SANITIZER_COMPILE_FLAGS_RELEASE}"
    PARENT_SCOPE)
  set(CMAKE_C_FLAGS_RELWITHDEBINFO
    "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${SANITIZER_COMPILE_FLAGS_RELEASE}"
    PARENT_SCOPE)
  set(CMAKE_C_FLAGS_RELEASEASSERT
    "${CMAKE_C_FLAGS_RELEASEASSERT} ${SANITIZER_COMPILE_FLAGS_RELEASE}"
    PARENT_SCOPE)

  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${SANITIZER_COMPILE_FLAGS}"
    PARENT_SCOPE)
  set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} ${SANITIZER_COMPILE_FLAGS_DEBUG}"
    PARENT_SCOPE)
  set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE} ${SANITIZER_COMPILE_FLAGS_RELEASE}"
    PARENT_SCOPE)
  set(CMAKE_CXX_FLAGS_MINSIZEREL
    "${CMAKE_CXX_FLAGS_MINSIZEREL} ${SANITIZER_COMPILE_FLAGS_RELEASE}"
    PARENT_SCOPE)
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
    "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${SANITIZER_COMPILE_FLAGS_RELEASE}"
    PARENT_SCOPE)
  set(CMAKE_CXX_FLAGS_RELEASEASSERT
    "${CMAKE_CXX_FLAGS_RELEASEASSERT} ${SANITIZER_COMPILE_FLAGS_RELEASE}"
    PARENT_SCOPE)

  set(CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_EXE_LINKER_FLAGS}"
    PARENT_SCOPE)
  set(CMAKE_MODULE_LINKER_FLAGS
    "${CMAKE_MODULE_LINKER_FLAGS} ${SANITIZER_MODULE_LINKER_FLAGS}"
    PARENT_SCOPE)
  set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZER_SHARED_LINKER_FLAGS}"
    PARENT_SCOPE)

  # Export the sanitizer setting also to LLVM.
  set(LLVM_USE_SANITIZER ${SANITIZER} CACHE STRING
    "Export ComputeAorta sanitizer setting to LLVM")
endfunction()
