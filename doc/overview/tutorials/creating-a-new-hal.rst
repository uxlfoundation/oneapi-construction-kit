Creating a New HAL
==================

This tutorial is the first of a series on how to add support to ComputeAorta for
new hardware. It explains how to create a new HAL for that hardware and use it
to run basic compute kernels with clik. The HAL interface exposes a set of
device operations, such as allocating and copying device memory, loading
programs on the device as well as running kernels on the device. This interface
allows higher-level libraries or components such as clik and ComputeMux targets
to implement more complex features by building upon the basic device operations
exposed by the HAL.

For this tutorial, we will use the Codeplay Reference Silicon (RefSi) platform
as the example device to expose through clik. The new HAL will use the RefSi
driver (``refsidrv``) in order to control the virtual device. At the end of this
part, clik tests will run successfully when targeting RefSi. The HAL created in
this part will be used in the next tutorial on how to create a new ComputeMux
target for a new hardware device.

While it is possible to develop a new HAL and integrate it with ComputeMux to
run OpenCL kernels from the beginning, due to the scale and depth of both the
OpenCL standard and ComputeAorta, it is much easier to use the clik library as
scaffolding until the HAL is working at a basic level. The clik code base and
test suite are orders of magnitude smaller than ComputeAorta's, which means that
building and testing is vastly faster and simpler. An off-the-shelf compiler
toolchain such as GCC or Clang can be used for compiling kernels instead of
embedding LLVM in the runtime library, which further saves on build time and
complexity. The development of the RefSi HAL can be focused on getting the
basics working on the device before migrating to fully-featured OpenCL and its
integrated compilation process.

.. toctree::
   :maxdepth: 3

   creating-new-hal/initial-setup
   creating-new-hal/kernel-compilation
   creating-new-hal/runing-clik-tests
   creating-new-hal/skeleton-hal-overview
   creating-new-hal/implementing-hal-operations
