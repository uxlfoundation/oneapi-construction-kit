ComputeMux
==========

.. toctree::
   :maxdepth: 1

   Change Log <mux/changes>
   Compiler <mux/compiler>
   CMake <mux/cmake>
   Bumping the Specification Version <mux/bumping-the-spec>

.. toctree::
   :hidden:

   mux/hal/dynamic_loading

.. seealso::
   The :doc:`/specifications/mux-runtime-spec` and the
   :doc:`/specifications/mux-compiler-spec`.

.. tip::
   Throughout this documentation and the codebase as a whole, ComputeMux has
   been shortened to Mux for brevity, but for the avoidance of all doubt, "Mux"
   refers to "ComputeMux".

ComputeMux is a set of API layers which provides an interface between client
sensitive source code and licensed source code provided by Codeplay. ComputeMux
supports one or more client targets and provides a selection mechanism to
determine which target is currently in use. ComputeMux attempts to be quick and
easy to integrate into new client projects and provides generation of the client
source files to help getting started. ComputeMux uses the CMake build system and
is designed to be integrated into a clients CMake.

Dependencies
------------

ComputeMux depends on the following being available on the ``PATH`` in
addition to your C/C++ tool chain.

-  ``cmake`` version 3.4.3 or above.
-  ``python`` version 2.x is tested, version 3.x should work.
-  ``clang-format`` ComputeMux will optionally format generated source files
   automatically, beware of differences in output between versions.

Integrating a Runtime Target
----------------------------

Runtime Header and Static Library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ComputeMux Runtime requires the client to provide a header and a static library,
there are restrictions on the naming convention for these items outlined below
where ``<target>`` is the desired client identifier.

-  ``<target>/<target>.h``
-  ``lib<target>.a`` or ``<target>.lib`` depending on the platform.

In order to get started quickly ComputeMux provides the ability to
automatically generate the header, additionally in the event the API
changes the header can be regenerated aiding in the refactoring process.

Runtime CMake Integration
~~~~~~~~~~~~~~~~~~~~~~~~~

ComputeMux provides the CMake function ``add_mux_target`` which should be
used in the target ``CMakeLists.txt``, this function performs a number
of tasks required to integrate a target into the ComputeAorta build.
These are:

-  Provide the CMake target for the static library containing the ComputeMux
   Runtime target.
-  Specify the list of builtins capabilities the ComputeMux Runtime target
   supports, see `capability reporting <#capbility-reporting>`__ below.
-  Specify the output directory of the header, this is used to create a
   CMake target which (re)generates the header when the ComputeMux Runtime API
   is updated.
-  Specify the list of device names the ComputeMux Runtime target provides.
-  Optionally override the ``clang-format`` executable used to format
   the header.

.. code:: cmake

   add_mux_target(<target> CAPABILITIES 64bit fp64
     HEADER_DIR <include-dir-path>/<target> DEVICE_NAMES <name>)

It is also possible specify the path to the ``clang-format`` executable
to use to override the version required by ComputeAorta like this:

.. code:: cmake

   add_mux_target(<target> CAPABILITIES 64bit fp64
     HEADER_DIR <include-dir-path>/<target> DEVICE_NAMES <name>
     CLANG_FORMAT_EXECUTABLE <clang-format-path>)

ComputeMux must also be made aware of the generated header include directory
by using the following CMake snippet, where ``<path-to-include>`` points
to the directory which contains a subdirectory ``<target>``, which in
turn contains the generated header ``<target>.h``.

.. code:: cmake

   target_include_directories(<target> SYSTEM PUBLIC <include-dir-path>)

ComputeMux requires that targets are compiled with exceptions and runtime type
information (RTTI) disabled. Additionally for UNIX platforms ComputeMux
enables Position Independent Code generation with ``-fPIC``, this is
required for linking archives into shared libraries. The recommended way
to ensure the required compiler flags are set is to use the
``add_ca_library`` CMake function in place of ``add_library`` when
creating the static library containing the implementation.

Capability Reporting
^^^^^^^^^^^^^^^^^^^^

ComputeMux requires that a target report its capabilities so that the correct
builtins can be built. This is done using the ``CAPABILTIES`` keyword
argument to the ``add_mux_target`` CMake function, see `CMake
Integration <#cmake-integration>`__ above. Below is a table of all
supported capabilities.

========== ====================== ============ ================
Capability Value                  Default      OpenCL Extension
========== ====================== ============ ================
Bit width  ``32bit`` or ``64bit`` None (error) N/A
FP double  ``fp64``               Disabled     ``cl_khr_fp64``
FP half    ``fp16``               Disabled     ``cl_khr_fp16``
========== ====================== ============ ================

A more detailed description of how capabilities are handled in CMake is
given `below <#capability-cmake-details>`__.

Target-Specific Builtins Header
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

ComputeMux Runtime targets will sometimes have target-specific intrinsics that
must be declared in a header. ComputeAorta provides a CMake utility function to
declare this header:

.. code:: cmake

   add_ca_force_header(PREFIX "device_specific_name_prefix"
     DEVICE_NAME "mux device device_name"
     PATH "path/to/header.h")

The header will be baked into CA and force-included in all kernels that
are built for the device.

-  ``PREFIX`` can be any string that meets the requirements for C
   identifiers, though it is strongly recommended that it contain the
   possibly shortened name of the device.
-  ``DEVICE_NAME`` is used to match mux devices to force-included
   headers at kernel build time. It must exactly match the device’s
   ``device_name`` field.
-  ``PATH`` is the path to the header. The header is only accessed at
   ComputeAorta build time.

``add_ca_force_header()`` is intended to add *one device-specific*
header. Adding additional force-included headers is not supported, and
will result in CMake errors. Changing the input parameters to
``add_ca_force_header()`` requires a clean rebuild of ComputeAorta.
Changing the contents of the force-included header triggers a re-link of
ComputeAorta; a clean rebuild is not required.

Mux devices are strongly encouraged to have an extension that indicates
the presence of a device force-included header. The ``host`` device, if
present, has a ``cl_codeplay_host_builtins`` extension, which can serve
as an example; see
``modules/mux/source/host/extension/cl_ext_codeplay/``. The ``host``
force-included header is only used for testing, so the
``cl_codeplay_host_builtins`` extension is only present in non-release
builds.

See the `OpenCL Extension Guide <../../source/cl/external-extensions>`__
for details on the extension mechanism.

Capability CMake Details
------------------------

Capabilities exist so that all the required versions of builtins are
built, and only the required versions of builtins are built. The
alternative would be to build all possible versions of builtins, which
would increase build time and bloat the final binary. The way
capabilities are handled in the build system may not be immediately
obvious because of the number of CMake variables and the number of
components involved. Capability-related variables are present in Abacus
builtins, Mux, and all Mux target devices. The OpenCL and Vulkan
implementations (``source/cl`` and ``source/vk``) do not rely on
capability variables, as they need to generically support more than one
Mux device, each with potentially different capabilities, at the same
time.

The interface between the Mux target build process and the builtins
build process is provided by the ``<target_name>_CAPABILITIES`` CMake
list. This variable is set by the Mux target using the
``add_mux_target`` CMake function and setting the ``CAPABILITIES``
keyword argument. Then ``modules/builtins/CMakeLists.txt`` parses the
list and builds all the versions of builtins required by all the Mux
devices in the current build.

In some situations, it is possible that a Mux target can, itself, be
built with different capabilities that are determined at compile time.
In such a case, the Mux target can have its own capability variables,
which are defined by the user, and used to determine
``<target_name>_CAPABILITIES``. For example, Host, if present, can be
built with or without support for floating point double and floating
point half (``cl_khr_fp64`` and ``cl_khr_fp16``). The ``CMakeLists.txt``
file for Host declares the options ``CA_HOST_ENABLE_FP64`` and
``CA_HOST_ENABLE_FP16``, and then sets the ``host_CAPABILITIES`` based
on these options.

The variable dependency chain looks like this:

::

   +- host/CMakeLists.txt --------------------------+
   |  (if present)                                  |
   |                                                |
   |  ,-<- option(CA_HOST_ENABLE_FP16 ...)          |
   |  |                                             |
   |  +-<- option(CA_HOST_ENABLE_FP64 ...)          |
   |  |                                             |
   | \|/                                            |
   |  '                                             |
   |  set(host_CAPABILITIES CACHE ...) -->-->-->-->-->--,
   |                                                |   |
   +------------------------------------------------+   |
                                                       \|/
   +- <target_name_2>/CMakeLists.txt ---------------+   |
   |                                                |   |
   | set(<target_name_2>_CAPABILITIES CACHE ...) ->-->--+
   |                                                |   |
   +------------------------------------------------+   |
                                                       \|/
   ...                                                  |
                                                        |
   +- <target_name_n>/CMakeLists.txt ---------------+  \|/
   |                                                |   |
   | set(<target_name_n>_CAPABILITIES CACHE ...) ->-->--+
   |                                                |   |
   +------------------------------------------------+   |
                                                       \|/
                                                        |
   +- modules/builtins/CMakeLists.txt --------------+   |
   |                                                |  \|/
   | foreach(mux_target ${MUX_TARGET_LIBRARIES})    |   |
   |   # Determine version of builtins to build  <---<--'
   | endforeach()                                   |
   |                                                |
   +------------------------------------------------+

Integrating a Compiler Target
-----------------------------

Compiler Header and Static Library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ComputeMux Runtime requires the client to provide a header containing a struct
or class which derives from ``compiler::Info``, and a static library. There are
no restrictions on the naming convention of these items.

Compiler CMake Integration
~~~~~~~~~~~~~~~~~~~~~~~~~~

ComputeMux provides the CMake function ``add_mux_compiler_target`` which should
be used in the target ``CMakeLists.txt``, this function performs a number
of tasks required to integrate a compiler target into the ComputeAorta build.
These are:

-  Provide the CMake target for the static library containing the ComputeMux
   Compiler target.
-  Specify the fully qualified name of the class or struct that derives from
   ``compiler::Info``, containing a public static ``get()`` function that
   returns a singleton instance of that compiler info.
-  Specify the path to the header containing a subclass of ``compiler::Info``.

.. code:: cmake

   add_mux_compiler_target(<target>
     COMPILER_INFO <fully-qualified-name>
     HEADER_DIR <include-dir-path>/<target>)

ComputeMux requires that targets are compiled with exceptions and runtime type
information (RTTI) disabled. Additionally for UNIX platforms ComputeMux
enables Position Independent Code generation with ``-fPIC``, this is
required for linking archives into shared libraries. The recommended way
to ensure the required compiler flags are set is to use the
``add_ca_library`` CMake function in place of ``add_library`` when
creating the static library containing the implementation.

Changes
-------

ComputeMux maintains a change log found in :doc:`changes </modules/mux/changes>`
to inform clients of changes which may effect the implementation of a
target, this combined with compile time checks ensure that client
targets do not have an outdated version of the generated headers. The
compile time checks provide an informative message which points to the
change log for quick evaluation of the changes introduced.

The generated headers contain a version triple which follows the
`semantic versioning <http://semver.org/>`__ scheme, the version triple
is controlled by ``modules/mux/tools/api/mux.xml`` and produces the
following preprocessor definitions; ``<TARGET>_MAJOR_VERSION``,
``<TARGET>_MINOR_VERSION``, ``<TARGET>_PATCH_VERSION``, and a combined
``<TARGET>_VERSION``.
