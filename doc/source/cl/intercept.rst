OpenCL Intercept Layer
======================

The `OpenCL Intercept Layer`_ allows developers to do a number of things with
existing OpenCL binary applications, largely focusing on working with OpenCL
program objects. The primary use case in the oneAPI Construction Kit is to
enable testing of OpenCL drivers which do not contain a runtime compiler by
applications but assume a runtime compiler is available, such as the
`OpenCL Conformance Test Suite`_. For more information see
`Injection Testing`_ below. The `OpenCL Intercept Layer`_ requires the use
of the :doc:`/source/cl/icd-loader`.

.. _OpenCL Intercept Layer: https://github.com/intel/opencl-intercept-layer
.. _OpenCL Conformance Test Suite: https://github.com/KhronosGroup/OpenCL-CTS

Configuration
-------------

There are a large number of `configuration controls
<https://github.com/intel/opencl-intercept-layer/blob/master/docs/controls.md>`_
made available by the `OpenCL Intercept Layer`_ which can be specified either
in the configuration file or through environment variables. When working with
multiple concurrent build directories editing a centralized configuration file
can become tedious and error prone so use of environment variables, prefixed
with ``CLI_``, is preferred. Below the configuration controls most relevant to
`Injection Testing`_ are described.

.. envvar:: CLI_OpenCLFileName

   The :envvar:`CLI_OpenCLFileName` environment variable specifies where to
   find the :doc:`/source/cl/icd-loader`, i.e. the absolute path to the
   ``libOpenCL.so`` or ``OpenCL.dll`` which should be used to load a client
   driver.

   See `OpenCLFileName`_ for more information.

.. envvar:: CLI_DumpDir

   The :envvar:`CLI_DumpDir` environment variable specifies the absolute path
   of the directory to dump program binaries when
   :envvar:`CLI_DumpProgramBinaries` is enabled, or to lookup program binaries
   for injection when :envvar:`CLI_InjectProgramBinaries` is enabled.

   See `DumpDir`_ for more information.

.. envvar:: CLI_DumpProgramBinaries

   The :envvar:`CLI_DumpProgramBinaries` environment variable, when set to a
   non-zero value, enables dumping program binaries resulting from calls to
   :opencl-1.2:`clCreateProgramWithSource` followed by
   :opencl-1.2:`clBuildProgram`. Dumped program binary files are written to the
   directory specified by :envvar:`CLI_DumpDir`. Generated file names include a
   hash of the build options passed to :opencl-1.2:`clBuildProgram`.

   See `DumpProgramBinaries`_ for more information.

.. envvar:: CLI_OmitProgramNumber

   The :envvar:`CLI_OmitProgramNumber` environment variable, when set to a
   non-zero value, disables inclusion of the unique program number in program
   binary filenames when :envvar:`CLI_DumpProgramBinaries` is enabled. This is
   especially useful for OpenCL applications which perform compilation in a
   multi-threaded context which can result in non-deterministic program
   numbering.

   See `OmitProgramNumber`_ for more information.

.. envvar:: CLI_InjectProgramBinaries

   The :envvar:`CLI_InjectProgramBinaries` environment variable, when set to a
   non-zero value, enables injection of previously dumped program binaries when
   the application calls :opencl-1.2:`clCreateProgramWithSource` followed by
   :opencl-1.2:`clBuildProgram`. Program binaries are loaded from the
   ``Inject`` subdirectory of the directory specified by :envvar:`CLI_DumpDir`.
   The search pattern for program binary file names does not include the hash
   generated from the build options passed to :opencl-1.2:`clBuildProgram` as
   this information is not available during the call to
   :opencl-1.2:`clCreateProgramWithSource` where injection occurs, thus dumped
   program binary file names must have the hash removed before they can be
   injected.

   Since the `OpenCL Intercept Layer`_ only searches for program binaries to
   inject in the ``Inject`` subdirectory of :envvar:`CLI_DumpDir`, additional
   preparation must be done in order to use previously dumped program binaries.
   The ``inject-prepare-bins.py`` script should be used to perform this
   preparation, this involves removing the build options hash from the file
   name and copying the file to the ``Inject`` subdirectory.

   .. code-block:: console

      $ python scripts/testing/inject-prepare-bins.py $CLI_DumpDir

   See `InjectProgramBinaries`_ for more information.

.. _OpenCLFileName: https://github.com/intel/opencl-intercept-layer/blob/master/docs/controls.md#openclfilename-string
.. _DumpDir: https://github.com/intel/opencl-intercept-layer/blob/master/docs/controls.md#dumpdir-string
.. _DumpProgramBinaries: https://github.com/intel/opencl-intercept-layer/blob/master/docs/controls.md#dumpprogrambinaries-bool
.. _OmitProgramNumber: https://github.com/intel/opencl-intercept-layer/blob/master/docs/controls.md#omitprogramnumber-bool
.. _InjectProgramBinaries: https://github.com/intel/opencl-intercept-layer/blob/master/docs/controls.md#injectprogrambinaries-bool

Injection Testing
-----------------

Injection testing is a method of enabling testing of an OpenCL driver which
does not contain a runtime compiler by applications which assume a runtime
compiler is available.

This process has three main steps:

1. Run the OpenCL application through the `OpenCL Intercept Layer`_ with an
   OpenCL driver which contains a runtime compiler to dump all program object
   that are built from source.
2. Prepare the dumped program object binary files for injection.
3. Run the OpenCL application through the `OpenCL Intercept Layer`_ with an
   OpenCL driver which does not contain a runtime compiler, using the prepared
   program object binaries.

.. _OpenCL CTS: https://github.com/KhronosGroup/OpenCL-CTS

.. caution::
   The OpenCL Intercept Layer has a number of limitations which should be
   considered if unexpected results are experienced:

   * The OpenCL Intercept Layer was not designed to operate in a multi-threaded
     context, doing so can cause unexpected behavior. This is a known `upstream
     issue <https://github.com/intel/opencl-intercept-layer/issues/42>`_.
   * Program objects are created using :opencl-1.2:`clCreateProgramWithBinary`
     rather than :opencl-1.2:`clCreateProgramWithSource`. Any calls to
     :opencl-1.2:`clCompileProgram` or :opencl-1.2:`clLinkProgram` using this
     program object will fail returning ``CL_INVALID_OPERATION``, which is
     behavior required by the specificaion.
   * Dumped program binaries with differing
     :opencl-1.2:`clBuildProgram`/:opencl-1.2:`clCompileProgram` options are
     not differentiated when used for injection due to the ordering of program
     object creation and compilation stages, this causes issues when a program
     object is compiled multiple times by the application with different
     options. For example, an application which compiles a program object with
     the same source multiple times, using compile options that can change
     behaviour like `-D`, only the first program binary will be injected when a
     subsequent version is expected by the application, this can cause:

       * Positive tests to erroneously fail.
       * Negative tests to erroneously succeed.
       * Test fixtures which share a program object to receive an incorrect
         program binary.

   * Injected program binaries pass to :opencl-1.2:`clGetKernelInfo` to query
     for attributes will fail as there is not source code to query for the
     attribute values.
   * Injected program binaries result in program objects that will not trigger
     any build status changes which can cause unexpected queries using
     :opencl-1.2:`clGetProgramBuildInfo` if the application expects compilation
     to occur and uses this information to trigger other work.

Prerequisites
+++++++++++++

The required preparation for each step is as follows.

* Step 1. requires the oneAPI Construction Kit built with a runtime compiler, i.e.
  a build configured with :cmake:variable:`CA_RUNTIME_COMPILER_ENABLED` set to
  ``ON``.
* Step 2. requires the ``scripts/testing/inject-prepare-bins.py`` script.
* Step 3. requires the oneAPI Construction Kit built without a runtime compiler,
  i.e. a second build configured with :cmake:variable:`CA_RUNTIME_COMPILER_ENABLED`
  set to ``OFF``.

Dumping Program Binaries
++++++++++++++++++++++++

The following environment variables, when used for dumping program binaries,
have been found to provide the best results.

* :envvar:`OCL_ICD_FILENAMES` set to the absolute path of an OpenCL driver
  which has a runtime compiler available.
* :envvar:`CLI_OpenCLFileName` set to the absolute path to the
  :doc:`/source/cl/icd-loader`.
* :envvar:`CLI_DumpDir` set to the absolute path of the desired program binary
  output directory.
* :envvar:`CLI_DumpProgramBinaries` set to ``1`` to enable dumping of program
  binaries.
* :envvar:`CLI_OmitProgramNumber` set to ``1`` to disable appending the program
  number to dumped program binary filenames.

Injecting Dumped Binaries
+++++++++++++++++++++++++

The following environment variables, when used for injecting program bianries,
have been found to provide the best results.

* :envvar:`OCL_ICD_FILENAMES` set to the absolute path of an OpenCL driver
  which does not contain a runtime compiler.
* :envvar:`CLI_OpenCLFileName` set to the absolute path to the
  :doc:`/source/cl/icd-loader`.
* :envvar:`CLI_DumpDir` set to the absolute path of the desired program binary
  output directory.
* :envvar:`CLI_DumpProgramBinaries` set to ``1`` to enable dumping of program
  binaries.
* :envvar:`CLI_OmitProgramNumber` set to ``1`` to disable appending the program
  number to dumped program binary filenames.

OpenCL CTS Examples
+++++++++++++++++++

.. note::
   While the examples below are using the `OpenCL CTS`_ the steps are
   transferable to any OpenCL application or test suite.

In the following examples, the steps to perform injection testing with the
`OpenCL CTS`_ are given. A modified list of tests is used as a number of tests
rely on calling :opencl-1.2:`clBuildProgram` with different build options
multiple times or calling
:opencl-1.2:`clCompileProgram`/:opencl-1.2:`clLinkProgram` to create program
objects. Both methods of program creation are not supported for injection
testing due to limitations of the `OpenCL Intercept Layer`_. The full list of
disabled tests can be found in the
``opencl_conformance_tests_wimpy_offline.csv`` file which can be found in the
``scripts/jenkins/cts_summary`` source directory or the ``share/OpenCL-CTS``
install directory, lines which begin with ``#`` are disabled tests.

.. todo CA-2636 Update this section when image testing has been fixed.

.. important::
   During research of the injection testing technique, OpenCL image support was
   disabled, thus any `OpenCL CTS`_ test failures when image support is enabled
   due to the failure methods described above have not been disabled in
   ``opencl_conformance_tests_wimpy_offline.csv``.

OpenCL CTS Dual Build Example
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Example on Linux using a prebuilt ``install`` from ``$PWD/build-online`` which
contains a runtime compiler enabled ``libCL.so`` and another prebuilt
``install`` from ``$PWD/build-offline`` which contains a compiler-less
``libCL.so``, :cmake:variable:`CMAKE_INSTALL_PREFIX` is set to
``$PWD/build-online/install`` and ``$PWD/build-offline/install`` respectively.
Testing the `OpenCL CTS`_ using City Runner being invoked from the root of the
oneAPI Construction Kit repository.

Firstly, dump the program bianries using the runing compiler in the
``$build-online/install/lib/libCL.so`` driver.

.. code-block:: console

   $ python scripts/testing/run_cities.py \
     -b $PWD/build-online/install/bin -L $PWD/build-online/install/lib \
     -e OCL_ICD_FILENAMES=$PWD/build-online/install/lib/libCL.so \
     -e CLI_OpenCLFileName=$PWD/build-online/install/lib/OpenCL/lib/libOpenCL.so \
     -e CLI_DumpDir=$PWD/build-online/CTSDump \
     -e CLI_DumpProgramBinaries=1 \
     -e CLI_OmitProgramNumber=1 \
     -s $PWD/build-online/install/share/cts_summary/opencl_conformance_tests_wimpy_offline.csv

Then, prepare the dumped program binaries for injection in the next step.

.. code-block:: console

   $ python scripts/testing/inject-prepare-bins.py $PWD/build-online/CTSDump

Finally, test using the prepared dumped program binaries using the
compiler-less ``$PWD/build-offline/install/lib/libCL.so``.

.. code-block:: console

   $ python scripts/testing/run_cities.py \
     -b $PWD/build-offline/install/bin -L $PWD/build-offline/install/lib \
     -e OCL_ICD_FILENAMES=$PWD/build-offline/install/lib/libCL.so \
     -e CLI_OpenCLFileName=$PWD/build-offline/install/lib/OpenCL/libOpenCL.so \
     -e CLI_DumpDir=$PWD/build-online/CTSDump \
     -e CLI_InjectProgramBinaries=1 \
     -s $PWD/build-offline/install/share/cts_summary/opencl_conformance_tests_wimpy_offline.csv
