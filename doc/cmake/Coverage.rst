Coverage Module
===============

.. note::
   Coverage support is deprecated and will be removed in a future version of
   the oneAPI Construction Kit.

.. cmake-module:: ../../cmake/Coverage.cmake

.. note::
  The root oneAPI Construction Kit ``CMakeLists.txt`` uses
  :cmake:command:`ca_option` to create the variable
  ``CA_ENABLE_COVERAGE``. When set the functions
  :cmake:command:`add_coverage_xml_input` and
  :cmake:command:`add_coverage_custom_xml` will be invoked by our CMake.

CoverageXMLInput Module
#######################

``CoverageXMLInput.cmake`` is used as part of the implementation of
:command:`add_coverage_xml_input` from the `Coverage Module`_.

.. cmake-module:: ../../cmake/CoverageXMLInput.cmake
