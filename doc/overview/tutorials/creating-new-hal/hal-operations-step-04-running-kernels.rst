Running Kernels
===============

The last, and by far most complex, remaining HAL operation needed to execute a
kernel is ``kernel_exec``. Given a program handle, kernel handle, scheduling
information and list of kernel arguments, the operation executes a kernel on the
device. Due to this complexity and in order to explain how kernels can be
executed on a RefSi device, implementing this operation will be divided into
several sub-steps.

.. toctree::
   :maxdepth: 3
   
   running-kernels/creating-command-buffers
   running-kernels/executing-kernels-without-scheduling
   running-kernels/simple-kernel-scheduling
   running-kernels/kernel-argument-packing
   running-kernels/2d-and-3d-kernel-scheduling
