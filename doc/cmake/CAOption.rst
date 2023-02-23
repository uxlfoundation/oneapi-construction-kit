CAOption Module
===============

.. cmake-module:: ../../cmake/CAOption.cmake

Option Naming Conventions
##########################

Current :ref:`developer-guide:Computeaorta Cmake Options` exposed by the project
are prefixed with '``CA_``' to differentiate them from CMake or third-party
provided options. This convention should be followed when adding any new
options to the project. For boolean type options in particular we also try to
adhere to the form '``CA_[COMPONENT_]ENABLE_<FEATURE>``', conforming to this
format is preferred.
