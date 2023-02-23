Sanitizers Module
=================

.. cmake-module:: ../../cmake/Sanitizers.cmake

.. note::
  The root ComputeAorta ``CMakeLists.txt`` uses :cmake:command:`ca_option` to
  create the variable ``CA_USE_SANITIZER``. If set by the user, this variable
  is used as an argument to :cmake:command:`ca_enable_sanitizer`.
