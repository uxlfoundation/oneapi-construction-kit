# Copyright (C) Codeplay Software Limited. All Rights Reserved.

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_LIBRARY_NAME

  A string CMake option to override the output name of the OpenCL shared
  library.

  Default value
    ``"CL"``
#]=======================================================================]
ca_option(CA_CL_LIBRARY_NAME STRING
        "Output name of the OpenCL shared library" "CL")

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_LIBRARY_VERSION

  A string CMake option of the form ``<major>.<minor>`` to override the
  version of the OpenCL shared library.

  Default value
    ``"${ComputeAorta_VERSION_MAJOR}.${ComputeAorta_VERSION_MINOR}"``
#]=======================================================================]
ca_option(CA_CL_LIBRARY_VERSION STRING
        "Version of the OpenCL shared library" "")

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_FORCE_PROFILE

  A string CMAke option to force the OpenCL platform to report ``"full"`` or
  ``"embedded"`` profile.

  Default value
    ``""``
#]=======================================================================]
ca_option(CA_CL_FORCE_PROFILE STRING
        "Force cl to use 'full' or 'embedded' profile" "")

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_PUBLIC_LINK_LIBRARIES

  A list of additional libraries that the ``CL`` target should be linked
  against.

  Default value
    ``""``
#]=======================================================================]
ca_option(CA_CL_PUBLIC_LINK_LIBRARIES STRING
        "Set additional libraries that CL should link against" "")

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_ENABLE_ICD_LOADER

  A boolean CMake option to enable building with the
  :doc:`/source/cl/icd-loader`.

  Default value
    ``OFF``
#]=======================================================================]
ca_option(CA_CL_ENABLE_ICD_LOADER BOOL
        "Enable building the OpenCL-ICD-Loader" OFF)

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_EXTERNAL_ICD_LOADER_SOURCE_DIR

  A external path to the OpenCL-ICD-Loader repository, which will be used
  instead of the OpenCL-ICD-Loader repository in
  source/cl/external/OpenCL-ICD-Loader.
  This option is only used if CA_CL_ENABLE_ICD_LOADER is ON.

  Default value
    ``""``
#]=======================================================================]
ca_option(CA_CL_EXTERNAL_ICD_LOADER_SOURCE_DIR PATH
        "External path to the OpenCL-ICD-Loader repository" "")

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_ENABLE_INTERCEPT_LAYER

   A CMake option to enable an in-tree build of the `OpenCL Intercept Layer`_,
   the default value is ``OFF``. To enable this option set
   :cmake:variable:`CA_CL_ENABLE_INTERCEPT_LAYER` to ``ON`` during
   configuration. When set to ``ON``, :cmake:variable:`CA_CL_ENABLE_ICD_LOADER`
   must also be set to ``ON``, otherwise CMake will emit an error.

   When enabled all targets created using :cmake:command:`add_ca_cl_executable`
   or :cmake:command:`add_ca_cl_library` will link against the ``OpenCL``
   target defined by the `OpenCL Intercept Layer`_ instead of linking against
   the :doc:`/source/cl/icd-loader`. Additionally, to avoid CMake configuration
   errors where both projects attempt to define the ``OpenCL`` target, the
   :doc:`/source/cl/icd-loader` will be built as an `ExternalProject`_.

.. _OpenCL Intercept Layer: https://github.com/intel/opencl-intercept-layer
.. _ExternalProject: https://cmake.org/cmake/help/latest/module/ExternalProject.html
#]=======================================================================]
ca_option(CA_CL_ENABLE_INTERCEPT_LAYER BOOL
        "Enable building the OpenCL-Intercept-Layer" OFF)

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_EXTERNAL_INTERCEPT_LAYER_SOURCE_DIR

  A external path to the OpenCL-Intercept-Layer repository, which will be used
  instead of the OpenCL-Intercept-Layer repository in
  source/cl/external/OpenCL-Intercept-Layer.
  This option is only used if CA_CL_ENABLE_INTERCEPT_LAYER is ON.

  Default value
    ``""``
#]=======================================================================]
ca_option(CA_CL_EXTERNAL_INTERCEPT_LAYER_SOURCE_DIR PATH
        "External path to the OpenCL-Intercept-Layer repository" "")

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_ENABLE_VECZ_VP_CHECK

  A boolean CMake option to enable vector-predication checks, including the
  `check-UnitCL-prevec-vecz-vp-partial-scalarization` target.

  Default value
    ``OFF``
#]=======================================================================]
# TODO(CA-3944): Vectorizing with vector predication soft-fails on LLVM 12
# and below (meaning it's not interesting) but on LLVM 13 it crashes the LLVM
# (RVV) backend. We need to build LLVM with extra patches for this to work.
ca_option(CA_CL_ENABLE_VECZ_VP_CHECK BOOL
        "Enable check-UnitCL targets covering vecz vector predication" OFF)

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_ENABLE_OFFLINE_KERNEL_TESTS

  A boolean CMake option to enable UnitCL offline compiler kernel tests.

  Default value
    ``ON``
#]=======================================================================]
ca_option(CA_CL_ENABLE_OFFLINE_KERNEL_TESTS BOOL
        "Enable UnitCL offline compile kernel tests" ON)

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_ENABLE_EXTENDED_CHECKS

  A boolean CMake option to enable extended UnitCL testing.

  Default value
    ``ON``
#]=======================================================================]
# Too many tests cause MR tester to time out.
ca_option(CA_CL_ENABLE_EXTENDED_CHECKS BOOL
        "Enable extended UnitCL testing" ON)

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_ENABLE_GENERATEDTESTS_CHECK

  A boolean CMake option to enable the `check-GeneratedTests` target.

  Default value
    ``OFF``
#]=======================================================================]
ca_option(CA_CL_ENABLE_GENERATEDTESTS_CHECK BOOL
        "Enable check-GeneratedTests target" OFF)

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_CTS_ENABLE_WARNINGS

  A boolean CMake option to enable CTS compiler warnings.

  Default value
    ``OFF``
#]=======================================================================]
if(EXISTS ${PROJECT_SOURCE_DIR}/source/cl/test/OpenCL-CTS/test_conformance/CMakeLists.txt)
    ca_option(CA_CL_CTS_ENABLE_WARNINGS BOOL "Enable CTS compiler warnings" OFF)
endif()

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_CTS_ENABLE_CHECK

  A boolean CMake option to enable CTS checke target.

  Default value
    ``ON``
#]=======================================================================]
ca_option(CA_CL_CTS_ENABLE_CHECK BOOL "Enable check-cl-cts testing" ON)

#[=======================================================================[.rst:
.. cmake:variable:: CA_CL_TEST_STATIC_LIB

  A boolean CMake option to enable testing with the static CL target. This
  forces all CL executables to link the static library, meaning lit tests with
  clc/oclc are included in this.

  Default value
    ``OFF``
#]=======================================================================]
ca_option(CA_CL_TEST_STATIC_LIB BOOL
  "Perform unit and lit testing with the static CL library." OFF)
if(CA_CL_TEST_STATIC_LIB)
  message(WARNING
    "Testing with the static lib has known issues with CL 3.0, see CA-3650.")
endif()
