Find Modules
============

To streamline finding external tools our build depends on, we provide several
`Find Modules`_ in our ``cmake/`` directory which are discoverable by the CMake
`find_package`_ command. Encapsulating code for wrangling dependencies in
separate modules makes our ``CMakeLists.txt`` files less cluttered and more
readable, as the `find_package`_ interface forces explicit definition of
requirements. Failing to find dependencies can result in portions of our build
being disabled, often testing components.

For example, Clang provides many clang based tools as part of a package, but we
don't require them all. Using a `FindClangTools Module`_ lets us select only
the individual tools we need via the ``COMPONENTS`` keyword. The ``VERSION``
keyword can also be used to mandate the version of the tools needed, useful
when we only support fixed versions as part of our build.

.. seealso::
  All `Find Modules`_ are implemented using `FindPackageHandleStandardArgs`_

.. _FindPackageHandleStandardArgs:
  https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html

FindClangTools Module
#####################

.. cmake-module:: ../../cmake/FindClangTools.cmake

FindClspv Module
################

.. cmake-module:: ../../cmake/FindClspv.cmake

FindGitClangFormat Module
#########################

.. cmake-module:: ../../cmake/FindGitClangFormat.cmake

.. seealso::
  Used to implement the :doc:`/cmake/Format`.

FindLLVMTool Module
###################

.. cmake-module:: ../../cmake/FindLLVMTool.cmake

FindLit Module
##############

.. cmake-module:: ../../cmake/FindLit.cmake

FindSpirvTools Module
#####################

.. cmake-module:: ../../cmake/FindSpirvTools.cmake

.. _find_package:
 https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html
.. _Find Modules:
 https://cmake.org/cmake/help/latest/manual/cmake-developer.7.html#find-modules

Namespaces
##########

Our `Find Modules`_ make use of CMake's double colon namespace prefix when
creating component targets, conforming to syntax '``<package>::<component>``'.
This is in accordance with policy `CMP0028`_ where CMake will recognize that
values passed to :cmake-command:`target_link_libraries` that contain ``::`` in
their name are supposed to be `Imported Targets`_ rather than just library
names, and will produce appropriate diagnostic messages.

.. _Imported Targets:
 https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#imported-targets
.. _CMP0028:
 https://cmake.org/cmake/help/latest/policy/CMP0028.html
