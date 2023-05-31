Compiler View
=============

oneAPI Construction Kit provides a compiler suite that can consume kernels
written using open standards including OpenCL C, SPIR and SPIR-V. These compilers
accept source code or IR provided by an application and compile it into an
executable that is used by runtime APIs to execute work on custom processor
architectures.

The :doc:`/overview/compiler/computemux-compiler` is exposed as collection of
C++ interfaces and a base implementation of those interfaces that are designed
to be extended by ComputeMux targets to implement code generation for disparate
hardware.

In summary, source code or IR is passed to one of the front-ends exposed by
``compiler::Module``, optimization passes are run, then the target backend is
called to link builtins and generate code to run on a ComputeMux device.
Optionally, kernel compilation can be deferred as late as kernel enqueue time,
using the ``compiler::Kernel`` interface.

The following diagram illustrates the order of operations that drives the
compilation process:

.. include:: diagram-compiler-view.rst

OpenCL-C Consumption
--------------------

OpenCL C source code is passed to the ``compiler::Module::compileOpenCLC``
method, which is implemented by the oneAPI Construction Kit.

Clang, a component of LLVM, is used to parse the source code, generate LLVM IR
and run initial optimization passes ready to pass to the backend. Clang fully
supports the OpenCL C 1.1 and 1.2 standards. Clang experimentally supports
the OpenCL C 3.0 standard, and is currently only tested to work with LLVM 10
after applying an internal patch.

oneAPI Construction Kit also provides the headers for OpenCL C builtin functions
to Clang as a pre-compiled header, which later get linked as part of
``compiler::Module::finalize``.

SPIR Consumption
----------------

SPIR binaries are passed to the ``compiler::Module::loadSPIR`` and
``compiler::Module::compileSPIR`` methods, which are implemented by
the oneAPI Construction Kit.

As SPIR binaries are serialized LLVM modules, ``loadSPIR`` deserializes
the LLVM module. ``compileSPIR`` then runs a number of fixup passes and a
generic optimization pipeline based on the one Clang normally runs during
code generation.

``loadSPIR`` and ``compileSPIR`` expect a binary that follows the
`SPIR 1.2 specification <https://www.khronos.org/registry/SPIR/specs/spir_spec-1.2.pdf>`_.
Note that SPIR is only supported for legacy reasons, with SPIR-V being the way
forwards.

SPIR-V Consumption
------------------

SPIR-V binaries are passed to the ``compiler::Module::compileSPIRV`` method,
which is implemented by the oneAPI Construction Kit.

SPIR-V is an intermediate representation for kernel programs used by Vulkan, and
can also be passed to an OpenCL driver that is at least version 2.1, or has the
``cl_khr_il_program`` extension enabled.

oneAPI Construction Kit comes with ``spirv-ll``, a static library that implements
translation from binary SPIR-V modules to an ``llvm::Module``. ``spirv-ll``
fully supports `SPIR-V 1.0 <https://www.khronos.org/registry/SPIR-V/specs/unified1/SPIRV.html>`_
and a number of SPIR-V extensions:

* ``SPV_KHR_no_integer_wrap_decoration``
* ``SPV_KHR_variable_pointers``
* ``SPV_KHR_16bit_storage``
* ``SPV_KHR_float_controls``
* ``SPV_KHR_storage_buffer_storage_class``
* ``SPV_KHR_vulkan_memory_model``
* ``SPV_codeplay_usm_generic_storage_class``

``compileSPIRV`` uses ``spirv-ll`` to convert the SPIR-V module into an
``llvm::Module``, then runs a number of fixup passes and a generic optimization
pipeline based on the one Clang normally runs during code generation.

Binaries
--------

Once the source code has been consumed, ``compiler::Module::finalize`` is
called to run any remaining backend specific passes, such as linking builtins
and vectorization, then device specific passes are run. The LLVM module is now
ready for either offline or deferred compilation.

Offline Compilation
~~~~~~~~~~~~~~~~~~~

Offline compilation is the process of generating a binary from a finalized
module, ready to be passed to the ``muxCreateExecutable`` endpoint in the
ComputeMux Runtime. This binary contains all the kernels contained within the
program, and associated metadata. This is usually represented using the ELF
binary format, but it's up to the ComputeMux target to determine which format
is used.

Offline binaries are generated using the ``compiler::Module::createBinary``
method. Unlike the ``compile*`` methods mentioned above, ``createBinary`` needs
to be implemented by a ComputeMux target, and the binary returned by this
function must be loadable by ``muxCreateExecutable``.

Deferred Compilation
~~~~~~~~~~~~~~~~~~~~

Deferred compilation is the process of deferring the finalization and code
generation of a kernel as late as possible, to take advantage of additional
information only available at runtime, such as the local size, global size, or
descriptors. This feature is optional and is not required to be supported by
a ComputeMux target.

Instead of generating a binary from a program using
``compiler::Module::createBinary``, the function
``compiler::Module::createKernel`` is called instead. This creates a
``compiler::Kernel`` object, representing a single kernel function within the
module, and is called once for each kernel function that is enqueued.

Before enqueuing a kernel, the method
``compiler::Kernel::createSpecializedKernel`` will be called with the
execution options. At this point, the ``compiler::Kernel`` object can, for
example, run optimization passes knowing the exact local size that the kernel
will be executed with. ``compiler::Kernel::createSpecializedKernel`` must return
a binary containing *at least* the kernel function represented by the
``compiler::Kernel`` object, and the binary must also be loadable by
``muxCreateExecutable``.

More details about the deferred compilation flow can be found in the
:ref:`ComputeMux Compiler Specification
<specifications/mux-compiler-spec:Kernel>`.
