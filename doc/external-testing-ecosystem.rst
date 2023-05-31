External Testing Ecosystem
==========================

Usage
-----

* All scripts are known to build on Linux Ubuntu 18.04 and are not guaranteed to
  work on ARM, and will not work on Windows since the scripts are bash files.

* All testing scripts must be executed from the directory they live in.

* Each project ``PROJECT`` has between two to three files:

  test-``PROJECT``.sh
    Mandatory
    Sets up the project and runs the actual testing script ``PROJECT``.sh

  ``PROJECT``.sh
    Mandatory
    Runs the tests in the project repository
  
  ``PROJECT``.patch
    Optional
    Only exists if the project is known to have failures unrelated to
    the oneAPI Construction Kit and that can be fixed/worked around with
    a patch.

.. note::
   The jenkins job makes use of a docker container defined by
   ``develop/scripts/Dockerfile`` as this provides an environment that can
   install arbitrary dependencies for projects. If you are trying to test a
   project locally and it isn't building correctly check this file for packages
   you might be missing.


Ecosystem scripts are designed to test either OpenCL or Vulkan.

Ecosystem scripts that test CL rely on having an OpenCL ICD loader install
present to link projects against, and the ``OCL_ICD_FILENAMES`` environment
variable to be set correctly, for detailed documentation on setting this up see
the :ref:`source/cl/icd-loader:OpenCL ICD Loader` page. The important thing is
to make sure of is that invoking ``clinfo`` should list oneAPI Construction Kit
as the first available implementation.

Ecosystem scripts that test VK rely on having a Vulkan ICD loader to be present,
such as the ``vulkan-icd`` Ubuntu package, and the ``VK_ICD_FILENAMES``
environment variable to be set to the location of ``vulkan/icd.d/VK.json`` in
your oneAPI Construction Kit installation directory.

Once your environment is set up correctly for the ICD loader most projects can
be tested simply by invoking the appropriate script:

.. code-block:: console

   $ ./test-PROJECT.sh


Some projects also require either a ComputeCpp or LLVM installation to build.
These can be provided by setting environment variables like so:

.. code-block:: console

   $ export COMPUTECPP=/path/to/computecpp/install/
   $ export LLVM=/path/to/llvm/install

If a project requires one of these additional environment variables it will be
noted next to its description below.

`ArrayFire`_
------------

General purpose CPU/GPU library.

`BabelStream`_
--------------

Collection of benchmarks.

`Beignet`_
----------

Internal tests of the Beignet implementation of the OpenCL specification.

`Boost.Compute`_
----------------

C++ GPU Computing Library for OpenCL.

`clBLAS`_
---------

Library containing BLAS functions written in OpenCL.

`CLBlast`_
----------

Tuned OpenCL BLAS library.

`clFFT`_
--------

Library containing FFT functions written in OpenCL.

`clGPU`_
--------

Framework implementation of BLAS accelerated using Intel(TM) Processor Graphics.

`CloverLeaf`_
-------------

Hydrodynamics mini-app solving compressible Euler equations in 2D.

`clRNG`_
--------

OpenCL based software library containing random number generation functions.

`clSPARSE`_
-----------

Software library containing Sparse functions written in OpenCL.

`ComputeApps`_
--------------

Various compute applications.

`ComputeCpp SDK`_
-----------------

Collection of sample code for Codeplay’s ComputeCpp.

.. note::
   Requires a ComputeCpp install.

`Eigen`_
--------

C++ template library for linear algebra: matrices, vectors, numerical solvers.

.. note::
   Requires a ComputeCpp install.

`Glow`_
-------

Machine learning compiler and execution engine for hardware accelerators.

.. note::
   Requires an LLVM install.

`Halide`_
---------

Language for fast, portable data-parallel computation.

.. note::
   Requires an LLVM install.

`IREE`_
-------

An MLIR-based end-to-end compiler that lowers ML models to a unified IR
optimized for real-time mobile/edge inference. Contains a Vulkan SPIR-V backend.

.. note::
    IREE uses its own pinned version of the LLVM repo, which is built as part
    of ``test-iree.sh``.


`OpenCL Book Samples`_
----------------------

Source code to the example programs from the OpenCL Programming Guide.

`OpenCV`_
---------

Open Source Computer Vision Library.

`Piglit`_
---------

Collection of automated tests for OpenCL implementations.

`PolyBench`_
------------

Collection of benchmarks.

.. note::
   We actually run a slightly modified version of the code that's on
   github hosted in our internal PerfCL repo.

`PyOpenCL`_
-----------

OpenCL integration for Python.

`SYCL-BLAS`_
------------

Implementation of BLAS using SYCL for acceleration on OpenCL devices.

.. note::
   Requires a ComputeCpp install.

`SYCL-DNN`_
-----------

Library implementing various neural network algorithms using the SYCL API.

.. note::
   Requires a ComputeCpp install.

`TensorFlow`_
-------------

Library for numerical computation using data flow graphs. Additional tests we
run come from the internal ``tf_scripts`` repo.

In addition to requiring a ComputeCpp install the tensorflow script needs
a tensorflow wheel to be provided like this:

.. code-block:: console

   $ export TF_WHEEL=/path/to/tensorflow/wheel

A wheel can be obtained either by pulling the ``artefact.tensorflow`` artefact
from shared storage, or by building one locally with the ``build-tensorflow.sh``
script.

`TVM`_
------

Open deep learning compiler stack for cpu, gpu and specialized accelerators.

.. note::
   Requires an LLVM install of at least version 4.0.

`VexCL`_
--------

C++ vector expression template library for OpenCL.

`ViennaCL`_
-----------

Linear algebra library for computations on multi-core CPUs.

.. _Arrayfire:
   https://github.com/arrayfire/arrayfire
.. _BabelStream:
   https://github.com/UoB-HPC/BabelStream
.. _Beignet:
   https://github.com/intel/beignet
.. _Boost.Compute:
   https://github.com/boostorg/compute
.. _clBLAS:
   https://github.com/clMathLibraries/clBLAS
.. _CLBlast:
   https://github.com/CNugteren/CLBlast
.. _clFFT:
   https://github.com/clMathLibraries/clFFT
.. _clGPU:
   https://github.com/intel/clGPU
.. _CloverLeaf:
   https://github.com/UK-MAC/CloverLeaf_OpenCL
.. _clRNG:
   https://github.com/clMathLibraries/clRNG
.. _clSPARSE:
   https://github.com/clMathLibraries/clSPARSE
.. _ComputeApps:
   https://github.com/AMDComputeLibraries/ComputeApps
.. _ComputeCpp SDK:
   https://github.com/codeplaysoftware/computecpp-sdk
.. _Eigen:
   https://bitbucket.org/codeplaysoftware/eigen
.. _Glow:
   https://github.com/pytorch/glow
.. _Halide:
   https://github.com/halide/Halide
.. _IREE:
   https://github.com/google/iree
.. _OpenCL Book Samples:
   https://github.com/bgaster/opencl-book-samples
.. _OpenCV:
   https://github.com/opencv/opencv
.. _Piglit:
   https://github.com/mesa3d/piglit
.. _PolyBench:
   https://github.com/cavazos-lab/PolyBench-ACC
.. _PyOpenCL:
   https://github.com/inducer/pyopencl
.. _SYCL-BLAS:
   https://github.com/codeplaysoftware/sycl-blas
.. _SYCL-DNN:
   https://github.com/codeplaysoftware/SYCL-DNN
.. _TensorFlow:
   https://github.com/codeplaysoftware/tensorflow
.. _TVM:
   https://github.com/dmlc/tvm
.. _VexCL:
   https://github.com/ddemidov/vexcl
.. _ViennaCL:
   https://github.com/viennacl/viennacl-dev
