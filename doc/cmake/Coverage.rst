Coverage Module
===============

.. seealso::
  For more details on ComputeAorta code coverage see
  :doc:`/scripts/coverage`.

.. cmake-module:: ../../cmake/Coverage.cmake

.. note::
  The root ComputeAorta ``CMakeLists.txt`` uses :cmake:command:`ca_option` to
  create the variable ``CA_ENABLE_COVERAGE``. When set the functions
  :cmake:command:`add_coverage_xml_input` and
  :cmake:command:`add_coverage_custom_xml` will be invoked by our CMake.

CoverageXMLInput Module
#######################

.. seealso::
  XML format defined in :ref:`scripts/coverage:XML Input File Option`.

``CoverageXMLInput.cmake`` is used as part of the implementation of
:command:`add_coverage_xml_input` from the `Coverage Module`_.

.. cmake-module:: ../../cmake/CoverageXMLInput.cmake
