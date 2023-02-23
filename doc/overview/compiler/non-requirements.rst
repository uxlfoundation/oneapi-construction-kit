ComputeMux Compiler Non-Requirements
====================================

ComputeMux compiler implementations are **not** required to support any of the
following features:

* Recursion
* Function pointers
* Dynamic allocation

  * All allocations **shall** be statically sized and occur in the prologue of
    a given function.

* Dynamic stack sizes

  * With no recursion or dynamic allocation, the maximum stack size is known at
    compile time.

* Exception-handling
* Garbage collection
* C runtime (CRT) support
* Debug information (which is optional)
* Half-precision floating-point, :ref:`if unsupported
  <overview/compiler/ir:Optional 16-bit half support>`
* Double-precision floating-point, :ref:`if unsupported
  <overview/compiler/ir:Optional 64-bit double support>`
* Dynamic linking (if the compiler implementation never emits external calls
  e.g., to LLVM's `compiler-rt <https://compiler-rt.llvm.org/>`_)

  * The ability to link against a small support library including compiler-rt
    and functions like ``memcpy``/``memset`` can help to simplify the compiler
    implementation and produce smaller code, but is **not** required.

* A full tool-chain: custom binary formats are permitted
