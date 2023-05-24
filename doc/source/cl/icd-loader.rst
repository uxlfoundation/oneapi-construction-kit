OpenCL ICD Loader
=================

oneAPI Construction Kit supports including the Khronos `OpenCL ICD Loader`_ in
a build when the repository is present in the
``source/cl/external/OpenCL-ICD-Loader`` directory. Internally this is a
`Git Submodule`_ however cloning the official repository at this location in a
source release should also work. Support is disabled by default but can be
enabled when configuring the build.

.. warning::
   When the `OpenCL ICD Loader`_ is enabled and OpenCL drivers are installed on
   the system, failure to explicitly specify either :envvar:`OCL_ICD_FILENAMES`
   or :envvar:`OCL_ICD_VENDORS` when running a test suite or application will
   result in it using the system driver rather than a driver from the
   oneAPI Construction Kit build.

.. hint::
   Building the `OpenCL ICD Loader`_ can be enabled with the
   :cmake:variable:`CA_CL_ENABLE_ICD_LOADER` CMake option. Note that on
   windows it will be necessary to set the registry setting to the built
   `bin/CL.ll`. The `source/cl/tools/icd-register.ps1` can be used for this.

.. _OpenCL ICD Loader: https://github.com/KhronosGroup/OpenCL-ICD-Loader

During development, when the `OpenCL ICD Loader`_ is enabled, executables which
link against it must specify an Installable Client Driver (ICD) which
implements the OpenCL API.

.. envvar:: OCL_ICD_FILENAMES

   An environment variable containing a ``:`` separated list of ICD's. If
   present, it is parsed by the `OpenCL ICD Loader`_ and used to populate the
   list of OpenCL drivers available to an OpenCL application. Example:

   .. code-block:: console

      $ OCL_ICD_FILENAMES=/path/to/libCL.so /path/to/UnitCL

oneAPI Construction Kit also supports the creation of ``.icd`` files which
are output to ``<build>/share/OpenCL/vendors`` and can be read by the
`OpenCL ICD Loader`_ to populate the list of OpenCL drivers for an
application to choose from.

.. envvar:: OCL_ICD_VENDORS

   An environment variable specifying the directory to search for ``.icd``
   files. If present, the `OpenCL ICD Loader`_ uses it to override the default
   search path of ``/etc/OpenCL/vendors`` on Linux or the registry entry
   ``HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\OpenCL\Vendors`` on Windows. For
   example:

   .. code-block:: console

      $ OCL_ICD_VENDORS=/path/to/OpenCL/vendors /path/to/UnitCL

.. note::
   The oneAPI Construction Kit ``check`` target infrastructure automatically
   makes use of the `OpenCL ICD Loader`_ when enabled, selecting the ``CL``
   OpenCL driver by specifying :envvar:`OCL_ICD_FILENAMES`.

.. _OpenCL ICD Loader:
   https://github.com/KhronosGroup/OpenCL-ICD-Loader
.. _Git Submodule:
   https://git-scm.com/book/en/v2/Git-Tools-Submodules
