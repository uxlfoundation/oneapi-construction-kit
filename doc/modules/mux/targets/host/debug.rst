Host Debug Features
===================

C Library Call Interception
---------------------------

When compiling for ``Host``, LLVM will sometimes insert calls to standard
library functions. This might include calls to ``memcpy()`` when data is copied
between OpenCL address spaces, ``__chkstk()`` on Windows to ensure enough stack
memory has been paged in, or ``__floatdidf()`` on 32-Bit Arm to perform
division in software. In the case of an offline-compiled kernel, calls to these
functions are stored as relocations in the kernel's ELF file, and the addresses
of the functions in memory are only filled in when the ELF file is loaded to be
executed. A useful side-effect of performing relocations is that it's possible
to provide non-standard implementations of these functions to aid debugging.

.. note::
  Calls to C library functions are quite rare in UnitCL, and their presence is
  fragile. It is difficult to predict whether a given LLVM option or pass will
  generate or remove a call to a library function. Intercepting C library calls
  is therefore only useful for debugging a small subset of bugs, but for those
  bugs it is invaluable.

The addresses of library functions are stored in ``relocs`` in
``modules/core/source/host/source/executable.cpp``. In Debug builds, calls to
``memcpy()`` and ``memset()`` are intercepted by default and directed to
``dbg_memcpy()`` and ``dbg_memset()``. The debug versions of these functions
perform rudimentary out-of-bounds access checks.

.. note::
  As of this writing (2020-03-31), no UnitCL kernel calls ``memset()``.

Tutorial: Inspecting a ``memcpy()`` Call
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``memcpy()`` calls normally end up calling an optimized, vectorized
implementation in libc. Even if you do manage to find debug symbols for it,
good luck figuring out what's going on. In ComputeAorta Debug builds,
offline-compiled kernels will call into ``dbg_memcpy()`` instead.
``dbg_memcpy()`` can be inspected with GDB. For example:

.. code-block:: console

   gdb --args ./bin/UnitCL --gtest_filter=OfflineExecution*Regression_90*
   (gdb) b dbg_memcpy
   run

How to: Intercept a Function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The replacement standard library functions are in an anonymous namespace in
``modules/core/source/host/source/executable.cpp``. Simply add the new function
to this namespace, and then add the function's name and address to
``relocs``.

.. note::
  It is only possible to intercept functions that are called by offline
  kernels. If an offline kernel calls a function for which a relocation does
  not exist, then the kernel will seg fault. Consequently, if an offline kernel
  can run, then all the library functions it needs are already in
  ``relocs``, and those are the only functions that can be intercepted.

.. warning::
  The compiler does not check that the intercepting function has the same
  signature as the function it's replacing. **Make sure that the signatures
  match.**

Explanation
^^^^^^^^^^^

Offline-compiled kernel binaries contain *relocations* at function call sites.
A relocation is just a relative or absolute hardware jump instruction with a
blank target address. When ComputeAorta loads the binary, it writes the address
of the function into the jump instruction (after possibly doing some math on
the address). This is why ``relocs`` stores function addresses as
``uint64_t`` types --- the bytes of the address may just be written directly
into a binary executable.

.. warning::
  Since library function call interception happens on the binary level of a
  compiled kernel, **all** of the normal protections offered by compilers are
  gone. There is no function prototype checking. The compiler is not able to
  check that the target address you have provided is even a valid function
  entry point. If somehow the OpenCL kernel and ComputeAorta use a different
  calling convention, then you're on your own.

Relocations are requested by function name by the ELF file stored inside an
offline-compiled kernel. Adding new entries into ``relocs`` will not affect
existing kernels, because existing kernels do not request those relocations. If
a relocation is requested that does not exist in ``relocs``, then the ELF
loader will report and error.

On Arm32, the ``Host`` finalizer
(``modules/core/source/host/source/finalizer.cpp``) enables various hardware
math features (e.g., ``features.push_back("+hwdiv");``). If these features are
not enabled, then LLVM will emit calls to library functions instead, and these
calls will then need to be added to ``relocs`` in the same way as
``__floatdidf()``. Intercepting these calls could potentially be a means of
debugging math functions on Arm32.

``dbg_memcpy()`` attempts to read all the source memory before copying it. Both
``dbg_memcpy()`` and ``dbg_memset()`` attempt to zero out the destination
memory before writing data to it. The reads and writes provide a rudimentary
bounds checking; if either fails, then ComputeAorta will abort with a
descriptive error.

.. warning::
  ``dbg_memcpy()`` and ``dbg_memset()`` can only catch out-of-bounds reads and
  writes that access memory outside of ComputeAorta's address space. I.e.,
  either function call must go horribly wrong before the illegal access is
  caught. It is trivial for a kernel calling ``memcpy()`` to completely clobber
  ComputeAorta's owned memory, and ``dbg_memcpy()`` cannot prevent that.
