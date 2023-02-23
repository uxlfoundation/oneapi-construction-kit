OpenCL External Extensions
==========================

A :doc:`/modules/mux` target can provide OpenCL device extensions to expose
target-specific features. There are two types of device extensions which a
target can expose:

* Runtime extensions - modify the behaviour of the OpenCL runtime API.
* Compiler extensions - modify the behaviour of the OpenCL C/SPIR/SPIR-V
  compiler.

The :doc:`/modules/host` target includes an example of each extension type, the
``codeplay_set_threads`` runtime extension, and the ``codeplay_host_builtins``
compiler extension.

.. note::
   The external extension mechanism also allows for *platform* extensions in
   addition to *device* extensions. Platform extensions are used to extend the
   entire ComputeAorta platform instead of just one :doc:`/modules/mux`
   device, but are otherwise similar to device extensions. Platform extensions
   are beyond the scope of this documentation, since ComputeMux devices should
   generally not need to introduce platform extensions.

Tutorials
---------

.. note::
   The tutorials below focus on the creation of runtime extensions but the
   information is equally applicable to compiler extensions.

Tutorial 1: Confirm the Example Extension Works
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``cl_codeplay_set_threads`` extension to the :doc:`/modules/host` target is
built by default along with ComputeAorta. An example program that calls the
extension is also built. Test that it works, e.g., on a 64-bit x86 machine
with:

.. code-block:: console

   $ <ca-build-dir>/bin/cl_ext_codeplay_example
   Available platforms are:
     1. ComputeAorta

   Selected platform 1

   Running example on platform 1
   Available devices are:
     1. ComputeAorta x86_64

   Selected device 1

   Running example on device 1
   Successfully called clSetNumThreadsCODEPLAY_ptr()

If instead the output ends with the following then it is possible that
ComputeAorta was configured with the extension disabled. Make sure that
ComputeAorta is configured with ``-DOCL_EXTENSION_cl_codeplay_set_threads=ON``
(the option defaults to ``ON``).

.. code-block:: console

   Failed to call clSetNumThreadsCODEPLAY_ptr()

Tutorial 2: Create a new runtime extension for Host CPU
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This tutorial shows how to create a new extension for :doc:`/modules/host` that
is a duplicate of the ``codeplay_set_threads`` extension. The example extension
provides the bare minimum functionality required of an extension, so this
tutorial serves as a starting point for creating an extension for a target
other than :doc:`/modules/host` are basically the same, with changes to names,
file paths, etc.

The example extension uses the following naming conventions:

+--------------------------------+-------------------------------+
| Field                          | Name                          |
+================================+===============================+
| Customer name                  | ``CODEPLAY``                  |
+--------------------------------+-------------------------------+
| Extension name                 | ``codeplay_set_threads``      |
+--------------------------------+-------------------------------+
| Extension fully qualified name | ``cl_codeplay_set_threads``   |
+--------------------------------+-------------------------------+
| Extension entry point          | ``clSetNumThreadsCODEPLAY()`` |
+--------------------------------+-------------------------------+
| Extension tag                  | ``host-cl-runtime-exts``      |
+--------------------------------+-------------------------------+

A new extension must have names for all of these fields.

To duplicate the example extension, follow these steps:

1. Copy the extension directory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Copy ``modules/core/source/host/extension/cl_ext_codeplay`` to an appropriate
location and rename it with a customer-specific name. For example, if the
customer is ACME Corp. then the directory might be called ``cl_ext_acme``. This
directory will contain the extension sources for the ACME hardware, which will
have one or more individual extensions.

2. Rename the source files
~~~~~~~~~~~~~~~~~~~~~~~~~~

Rename the following files:

* ``include/CL/cl_ext_codeplay_host.h``
* ``include/extension/codeplay_set_threads.h``
* ``source/codeplay_set_threads.cpp``

``cl_ext_codeplay_host.h`` is the single header that defines the interface to
all of this customer's extension. For ACME Corp. in might be called
``cl_ext_acme.h``.

.. note::
   The extension header has a ``_host`` suffix to avoid a name conflict with
   another extension header in ComputeAorta. In general, this will not be
   required.

``codeplay_set_threads.h`` and ``codeplay_set_threads.cpp`` are the header and
source for one particular extension. A customer extension can contain any
number of extensions, but ``codeplay`` has only one. If ACME hardware has a
``coyote`` feature, then these files might be called ``acme_coyote.h`` and
``acme_coyote.cpp``.

3. Update fields in files
~~~~~~~~~~~~~~~~~~~~~~~~~

In ``CMakeLists.txt``` update the arguments of
:cmake:command:`add_ca_cl_runtime_extension`:

* ``tag`` is the unique name for this set of extensions, e.g. ``acme_coyote``.
* ``EXTENSIONS`` is the list of all extension names this extension set, e.g.
  ``acme_coyote``.
* The ``HEADER`` and ``SOURCES`` fields should be updated with the file names
  used in `2. Rename the source files`_.

In ``include/CL/cl_ext_acme.h``:

* Update the header ``#ifndef #define`` flags.
* Replace the entry point name, ``clSetNumThreadsCODEPLAY``, with an
  appropriate name for the extension, e.g., ``clResetCoyoteACME``.

In ``include/extension/acme_coyote.h``:

* Update the header ``#ifndef #define`` flags.
* Update the extension class name. This should be the same as the name defined
  in the ``EXTENSIONS`` field in the ``CMakeLists.txt`` file above. I.e., it is the
  fully qualified name of the extension without a leading ``cl_``; for ACME
  Corp., it would be ``acme_coyote``.

In ``source/acme_coyote.cpp``:

* Update the ``#include`` commands to point to ``CL/cl_ext_acme.h`` and
  ``extension/acme_coyote.h`` (i.e., the ACME header and this extension's
  header).
* Update the extension class name to match the extension header.
* Update the fully qualified extension name string in the class constructor
  (``cl_codeplay_set_threads`` => ``cl_acme_coyote``).
* Update the ``#ifdef`` to enable or disable the extension. This is of the form
  ``OCL_EXTENSION_<fully_qualified_extension_name>``, e.g.,
  ``OCL_EXTENSION_cl_acme_coyote``. The option is automagically generated by
  CMake from the ``EXTENSIONS`` field.
* Update the ``GetDeviceInfo`` function to return an error if it's called with
  a device other than the device the extension is for (not necessary for this
  tutorial as the function already checks for the CPU host device).
* Update the entry point function name: ``clSetNumThreadsCODEPLAY`` =>
  ``clResetCoyoteACME``. Note that the function name also shows up as a string
  literal in ``GetExtensionFunctionAddressForPlatform()``.

4. Include the extension in Host CPU
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In ``modules/core/source/host/CMakeLists.txt``, add the new extension
directory. If the directory is located outside of the ComputeAorta tree, then
``add_subdirectory()`` will need to specify a binary directory:

.. code-block:: cmake

   add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/extension/cl_ext_codeplay)
   add_subdirectory(${path_to}/cl_ext_acme) # Add this

5. Configure and build
~~~~~~~~~~~~~~~~~~~~~~

Reconfigure ComputeAorta. There is now a new CMake option,
``-DOCL_EXTENSION_cl_acme_coyote``, which defaults to ``ON``. The new extension
should be listed in the CMake output:

.. code-block:: console

   ...
   -- OpenCL extension cl_codeplay_set_threads : ENABLED
   -- OpenCL extension cl_acme_coyote: ENABLED
   ...

Install ComputeAorta. ``<install_dir>/include/CL/`` should now contain
``cl_ext_acme.h`` for developers to include in their programs.

6. Make a test program
~~~~~~~~~~~~~~~~~~~~~~

``modules/core/source/host/extension/example/`` contains a bare-bones example of
using the ``cl_codeplay_set_threads`` extension. Copy this directory to an
appropriate location. Then update the files as follows:

* In ``CMakeLists.txt``, change all instances of ``cl_ext_codeplay_example`` to
  the name of the new example program (e.g., ``cl_ext_acme_example``).
* In ``main.c``, update the ``#include`` to use ``CL/cl_ext_acme.h``.
* In ``main.c``, update the ``main()`` function to get and use the new
  extension's interface. The ``main()`` function does the following:

  1. Gets the platform. This does not need to change.
  2. Gets the device. This does not need to change.
  3. Creates a function pointer to the extension entry point. This must be
     changed to use the types in the new customer header
     (``CL/cl_ext_acme.h``). I.e., ``clSetNumThreadsCODEPLAY`` =>
     ``clResetCoyoteACME``.
  4. ``clGetExtensionFunctionAddressForPlatform()`` queries OpenCL for the
     entry point name. The string must be updated to match the one used in
     ``acme_coyote.cpp``.
  5. Calls the entry point. The name of the entry point should be updated.
* Finally, add the test program's directory to ``host``'s CMake at
  ``modules/core/source/host/CMakeLists.txt``. If the directory is located
  outside of the ComputeAorta tree, then ``add_subdirectory()`` will need to
  specify a binary directory:

.. code-block:: cmake

   add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/extension/example)
   add_subdirectory(${path_to_acme_example})  # Add this

Reconfigure and build ComputeAorta. Then run the new test program, e.g., on a
64-bit x86 machine with:

.. code-block:: console

   $ <ca-build-dir>/bin/cl_ext_acme_example
   Available platforms are:
     1. ComputeAorta

   Selected platform 1

   Running example on platform 1
   Available devices are:
     1. ComputeAorta x86_64

   Selected device 1

   Running example on device 1
   Successfully called clResetCoyoteACME_ptr()

How to ...
----------

How to Add a Second Extension
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A customer extension can have any number of individual extensions. For example,
the ACME Corp. from `Tutorial 2: Create a new runtime extension for Host CPU`_
might have a second extension, ``cl_acme_anvil``, which requires a second entry
point, ``clDropAnvilACME()``. Adding this second extension requires the
following in the extension's source:

1. The library header in ``include/CL/`` must ``typedef`` the entry point
   function pointer and declare the entry point function.
2. ``include/extension/`` must contain a new header for the extension (e.g.,
   ``acme_anvil.h``).
3. ``source/`` must contain the source file for the extension (e.g.,
   ``acme_anvil.cpp``).
4. In ``CMakeLists.txt``, the ``EXTENSIONS`` parameter on
   :cmake:command:`add_ca_cl_runtime_extension` must list both extensions
   (e.g., ``EXTENSIONS acme_coyote acme_anvil``).
5. In ``CMakeLists.txt``, the ``SOURCES`` parameter on
   :cmake:command:`add_ca_cl_runtime_extension` must list the new source and
   header file.

How to Add a UnitCL test of a new Extension
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

UnitCL has an interface for adding extra tests, described in
:doc:`test/unitcl`. An appropriate ``CMakeLists.txt`` of the
:doc:`/modules/mux` target pulls in the required test files. An example is the
``cl_codeplay_set_threads`` extension from :doc:`/modules/host`. The use of the
UnitCL interface can be seen in
``modules/mux/targets/host/test/CMakeLists.txt``. The extension test is
contained in ``modules/mux/targets/host/test/UnitCL/cl_ext_codeplay.cpp``.

Explanation
-----------

Extension API Headers in ``CL``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Each extension set provides a header in ``CL/<name>.h`` for developers making
use of the extensions in that set. This file is merely the developer API, and
it is the only interface to the extensions. For each extension, it will
normally define an entry point function and a pointer type for that function.
It should not define anything internal to the extensions.  Developers will link
against this file.

Extension Source
^^^^^^^^^^^^^^^^

The extension sources (including any headers other than the single header in
``CL``) are internal to the extensions, and are compiled into ComputeAorta.
Developers writing OpenCL applications will not see these files. The only
interface into the extension implementation is through the API header in
``CL``.

Extension Entry Points
^^^^^^^^^^^^^^^^^^^^^^

.. danger::
   A device extension may be called with a different OpenCL device that does
   not support that extension. **It is the responsibility of the extension
   developer to ensure that a mismatch between extension and device is handled
   correctly**.

ComputeAorta queries extensions' entry points to determine what they support.
Consequently, a device extension must be able to correctly handle a situation
where it's called with a different device. For example, OpenCL users use
``clGetDeviceInfo()`` with either the ``CL_DEVICE_EXTENSIONS`` or the
``CL_DEVICE_EXTENSIONS_WITH_VERSION`` parameter to determine which extensions a
device supports. All extensions derive from the ``extension`` base class, and
the base class provides the necessary functionality for reporting extension
name strings in ``extension::GetDeviceInfo()``. An extension **must not** allow
the base class's ``GetDeviceInfo()`` to be called if the device does not
support the extension. Otherwise, the base class will report the extension name
string, and the OpenCL user will be led to believe that the device supports the
extension.


Extension CMake
^^^^^^^^^^^^^^^

Extensions are integrated into the ComputeAorta build system using the
:cmake:command:`add_ca_cl_runtime_extension` and
:cmake:command:`add_ca_cl_compiler_extension` CMake commands. These functions
store the information required later in the build and is used in
``source/cl/source/extension/CMakeLists.txt``. Any CMake target which depends
on the ``CL`` target also gains access to extensions enabled during CMake
configuration.

Extension Testing
^^^^^^^^^^^^^^^^^

Every extension should be thoroughly tested with unit tests. See `How to Add a
UnitCL test of a new Extension`_.

Testing should include the extensions' entry points, all possible return
values, and any interactions extensions may have with other parts of OpenCL.
For example, if an extension creates a special type of memory object, unit
tests should check that existing OpenCL API calls correctly handle this new
memory object. Existing unit tests of OpenCL calls can be found in
``source/cl/test/UnitCL/source``.

Extension Documentation
^^^^^^^^^^^^^^^^^^^^^^^

Extensions should be documented. An example is the
:doc:`/modules/mux/targets/host/extension/cl_codeplay_set_threads` extension.

Reference
---------

The :cmake:command:`add_ca_cl_runtime_extension` and
:cmake:command:`add_ca_cl_compiler_extension` CMake commands should be used to
integrate target extensions into the ComputeAorta build.
