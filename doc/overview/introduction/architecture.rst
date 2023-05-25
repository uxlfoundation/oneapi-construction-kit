oneAPI Construction Kit Architecture
====================================

oneAPI Construction Kit accelerates software by mapping heterogeneous software to custom
processor architectures. By enabling custom hardware, such as DSPs, to be
exposed in open standard programming models, a wide range of existing
applications are immediately made available for acceleration on the hardware.
To achieve maximum performance from the architecture, oneAPI Construction Kit has been
carefully designed for flexibility of implementation via its internal
`ComputeMux`_ interfaces. Included is a robust toolkit of optimization
utilities for both the compiler and runtime. oneAPI Construction Kit
:doc:`oneAPI Construction Kit </overview/toolkit>` facilitates the creation
of driver code enabling compute APIs.

.. seealso::
   For more information about oneAPI Construction Kit (previously known as ComputeAorta)
   see Codeplay's `IWOCL talk <https://www.youtube.com/watch?v=enyvwRWJ7PA>`_.

To achieve these goals oneAPI Construction Kit is organized in a modular fashion.
The :ref:`figure-architecture` depicts the main components.

.. ca_arch.png was created from ca_arch.svg with Inkscape then exported to png
.. to correclty render in PDFs which do error when attemping to render SVG files

.. _figure-architecture:
.. figure:: /_static/ca_arch.png
  :alt: oneAPI Construction Kit Architecture

  oneAPI Construction Kit Architecture Block Diagram

ComputeMux
----------

ComputeMux enables hardware vendors to support high-performance open standards
implementations at a lower cost of entry, in less time, and with higher
quality than feasible when starting from scratch. ComputeMux also makes
oneAPI Construction Kit highly extensible, as new compute APIs may be implemented on-top
of ComputeMux without dealing directly with hardware.

ComputeMux is a low-level component, the foundation, that enables oneAPI Construction Kit
to support a wide variety of disparate hardware. To utilize hardware through
ComputeMux, it is exposed as a target. Each target consists of an
implementation of the `ComputeMux Runtime`_ API and of the `ComputeMux
Compiler`_ API. The open standards provided by oneAPI Construction Kit are implemented in
terms of these ComputeMux APIs.

.. These APIs are separated to enable specialized teams to work on
.. the runtime and compiler independently.

.. seealso::
   :doc:`/overview/hardware` describes how ComputeMux APIs can be mapped onto
   hardware.

To enable optimizations for a wide variety of hardware, the :doc:`toolkit
</overview/toolkit>` contains a set of standards-based components. Being able
to plug-in existing solutions reduces development time for a target, as any
areas where hardware lacks support can be filled in software with mature,
rigorously tested code.

.. seealso::
   :doc:`/overview/example-scenarios` shows how hardware specific features
   can be made available to the user in oneAPI Construction Kit for writing high performance
   code.

Now lets give some addition context to each component in the
:ref:`figure-architecture`.

ComputeMux Runtime
^^^^^^^^^^^^^^^^^^

The runtime component is invoked by the application via open standards and is
responsible for managing devices, such that device-side kernel code can be
executed on them. These responsibilities include memory management,
synchronization, and scheduling of commands for execution.

.. seealso::
   See :doc:`/overview/runtime/computemux-runtime` for full details.

Host Runtime
""""""""""""

A ComputeMux Runtime API implementation targeting the CPU device on which the OpenCL
or Vulkan driver is running. This includes implementation of entry points for:

Memory Transfers
  Reading, writing, copying, filling etc. memory buffers/images.
Program Loading
  Loading a kernel binary into the appropriate memory region so that it can be
  executed by the CPU.
Scheduling
  Distributing the work items executed by the kernel invocation across the
  threads of the CPU.
Kernel Execution
  Execution of the kernel program on the CPU.
Synchronization
  Coordination of signaling and waiting between memory access and execution on
  the CPU.

Custom Runtime
""""""""""""""

A Custom ComputeMux Runtime API implementation targeting an accelerator device,
this includes implementation of entry points for:

Memory Transfers
  Reading, writing, copying, filling etc. memory buffers/images.
Program Loading
  Loading a kernel binary into the appropriate memory region so that it can be
  executed by the accelerator.
Scheduling
  Distributing the work items executed by the kernel invocation across the
  execution elements of the accelerator.
Kernel Execution
  Execution of the kernel program on the accelerator.
Synchronization
  Coordination of signaling and waiting between memory access and execution on
  the accelerator.

.. seealso::
  See :doc:`/overview/runtime` for details on implementing ComputeMux Runtime
  for a device.

Custom Driver
^^^^^^^^^^^^^

Custom target specific device driver used to implement the ComputeMux Runtime and
communicate with the custom device.

ComputeMux Compiler
^^^^^^^^^^^^^^^^^^^

The compiler component is used to compile kernel code into a form which can be
executed on the device via ComputeMux Runtime. oneAPI Construction Kit invokes the
ComputeMux Compiler API either through an offline compilation tool or from the
relevant compute API entry-points.

There are two modes of compilation:

Online Compilation
   In online mode, compilation of kernel code is performed during application
   runtime prior to being passed on to the `ComputeMux Runtime`_ for execution.

Offline Compilation
   In offline mode, compilation of kernel code is performed by a command-line
   tool prior to application runtime resulting in an object file. At
   application runtime the object file content is passed on to the `ComputeMux
   Runtime` for execution.

.. seealso::
   See :doc:`/overview/compiler/computemux-compiler` for full details.

Host Compiler
"""""""""""""

A ComputeMux Compiler API implementation targeting the CPU device on which the
OpenCL or Vulkan driver is running.

Custom Compiler
"""""""""""""""

A custom implementation of the ComputeMux Compiler API targeting a customer
accelerator device. This may include target specific front, middle and back end
compiler functionality.

.. seealso::
  See :doc:`/overview/compiler/ir` for details of the IR a customer compiler is
  expected to process.

Front End
"""""""""

Generic front end compiler components including:

Clang
  The LLVM project's C language compiler front end, used for consuming OpenCL C
  from the OpenCL API.
SPIR-V Parser
  An in house SPIR-V to LLVM IR translator for consuming SPIR-V from the OpenCL
  and Vulkan APIs.

Middle End
""""""""""

Generic middle end compiler functionality including:

Preparation and Optimizations
  In house and upstream execution model specific preparation compiler passes
  and optimizations that a target can use.
Whole Function Vectorizer
  An aggressive vectorization technique that is facilitated by the execution
  models of the low level compute APIs support by oneAPI Construction Kit.

Linking
"""""""

Linking of any libraries including:

Builtins
  Any execution model specific buitlins required by the compute kernel to be
  executed.
Math Library
  An in house performant math library supporting common math operations for
  integers, 64-bit doubles as well as 16-bit and 32-bit floating point types.

Host Target Back End
""""""""""""""""""""

CPU specific compiler backend codegen producing a binary object ready for execution
on the CPU.

Custom Target Back End
""""""""""""""""""""""

Custom compiler backend codegen producing a binary object ready for execution
on the custom device.

OpenCL
------

oneAPI Construction Kit provides OpenCL 1.2 or OpenCL 3.0 implemented in terms of
`ComputeMux Runtime`_ and `ComputeMux Compiler`_ APIs. We support the following
OpenCL extensions:

- cl_khr_command_buffer
- cl_khr_extended_async_copies
- cl_khr_global_int32_base_atomics
- cl_khr_global_int32_extended_atomics
- cl_khr_local_int32_base_atomics
- cl_khr_local_int32_extended_atomics
- cl_khr_byte_addressable_store
- cl_khr_fp64
- cl_khr_spir
- cl_khr_icd
- cl_codeplay_wfv
- cl_codeplay_extra_build_options
- cl_codeplay_kernel_exec_info
- cl_codeplay_program_snapshot
- cl_codeplay_performance_counters
- cl_codeplay_soft_math
- cl_intel_unified_shared_memory

.. note::
   Integration of custom extensions for vendor hardware is supported.

oneAPI Construction Kit's host implementation supports images and doubles in OpenCL 1.2.

Of the `optional OpenCL 3.0 features`_ oneAPI Construction Kit's host implementation
supports the `Intermediate Language Programs`_ feature and all `core features`_
in OpenCL 3.0.

.. _Intermediate Language Programs:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#_intermediate_language_programs
.. _core features:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#changes_to_opencl
.. _optional OpenCL 3.0 features:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#opencl-3.0-backwards-compatibility

SYCL
----

oneAPI Construction Kit is designed to slot into a `SYCL`_ technology stack, as the
:ref:`overview/introduction/architecture:OpenCL` and
:ref:`overview/introduction/architecture:Vulkan` APIs exposed can be used as a
`SYCL Backend`_, or oneAPI Construction Kit can be used to write an implementation of 
SYCL directly to a system or device.

oneAPI Construction Kit has thoroughly tested integration with `ComputeCpp`_, Codeplay's
implementation of the `SYCL 1.2.1`_ and `SYCL 2020`_ specifications. This
allows `SYCL`_ applications to be run through oneAPI Construction Kit, encouraging the
development of OpenCL extensions and optimizations in oneAPI Construction Kit that are of
benefit to software using `SYCL`_.

To learn more about ComputeCpp see the :doc:`ComputeCpp Overview
</computecpp>`.

.. _SYCL:
  https://www.khronos.org/sycl
.. _SYCL 1.2.1:
  https://www.khronos.org/registry/SYCL/specs/sycl-1.2.1.pdf
.. _SYCL 2020:
  https://www.khronos.org/registry/SYCL/specs/sycl-2020/html/sycl-2020.html
.. _SYCL Backend:
  https://www.khronos.org/registry/SYCL/specs/sycl-2020/html/sycl-2020.html#_the_sycl_backend_model
.. _ComputeCpp:
  https://developer.codeplay.com/products/computecpp/ce/home

Vulkan
------

oneAPI Construction Kit provides a pre-conformant Vulkan driver implementing the compute
subset of Vulkan 1.0 functionality.
