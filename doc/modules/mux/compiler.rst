ComputeMux Compiler
===================

The ComputeMux Compiler is an OpenCL C and SPIR-V compiler that consumes the
source code or IL provided by an application and compiles it into an executable
that can be loaded by the ComputeMux Runtime.

The module aims to provide a boundary beyond which no LLVM type definitions pass
in order to keep logical concerns separate.

Structure
---------

The ``compiler`` module is structured as a set of virtual interfaces and a loader
library, with a number of concrete implementations for different ComputeMux
targets. The virtual interfaces reside in ``include/compiler/*.h``, the library
entry point resides in ``library/include/library.h`` and
``library/source/library.cpp``, the dynamic loader library resides in
``loader/include/loader.h`` and ``loader/source/loader.cpp``, and the various
implementations reside in ``targets/*/``.

Dynamic vs Static loading
^^^^^^^^^^^^^^^^^^^^^^^^^

The compiler is designed to be used either directly as a static library,
or indirectly through a loader library.

Static Library
~~~~~~~~~~~~~~

The simplest way to use the compiler library is to link with the
``compiler-static`` target. Through this target, the compiler is accessed through
the ``compiler/library.h`` header. This target is unavailable if CMake is
configured with :cmake:variable:`CA_RUNTIME_COMPILER_ENABLED` set to ``OFF``

Dynamic Loader
~~~~~~~~~~~~~~

A more flexible option to use the compiler is to instead link wtih the
``compiler-loader`` target. Through this target, the compiler is accessed through
the ``compiler/loader.h`` header. This header is similar to ``compiler/library.h``,
however each method additionally requires a ``compiler::Library`` object,
created using ``compiler::loadLibrary``.

The purpose of the loader is to provide a compiler interface that will be
available when compiling the oneAPI Construction Kit regardless of the value of
:cmake:variable:`CA_RUNTIME_COMPILER_ENABLED`, and regardless of whether the
compiler is loaded at runtime or linked statically.

If :cmake:variable:`CA_RUNTIME_COMPILER_ENABLED` is set to ``ON`` then the
compiler loader can operate in two different ways:


* If :cmake:variable:`CA_COMPILER_ENABLE_DYNAMIC_LOADER` is set to ``ON``, then
  ``compiler::loadLibrary`` will look for ``compiler.dll`` (Windows) or
  ``libcompiler.so`` (Linux) in the default library search paths, depending on the
  platform.

  If the environment variable :envvar:`CA_COMPILER_PATH` is set, then its value
  will be used as the library name instead. Additionally, if
  :envvar:`CA_COMPILER_PATH` is set to an empty string, then
  ``compiler::loadLibrary`` will skip loading entirely and will operate as if no
  compiler is available.

  In this configuration, targets which depend on ``compiler-loader`` should also
  add the ``compiler`` target (the compiler shared library) as a dependency using
  ``add_dependencies``. See ``source/cl/CMakeLists.txt`` for an example.


* If :cmake:variable:`CA_COMPILER_ENABLE_DYMAMIC_LOADER` is set to ``OFF``, then
  ``compiler-loader`` will transitively depend on ``compiler-static``, and
  ``compiler::loadLibrary`` will instead immediately return an instance of
  ``compiler::Library`` that references the static functions directly.

If :cmake:variable:`CA_RUNTIME_COMPILER_ENABLED` is set to ``OFF``, then
``compiler::loadLibrary`` will always return ``nullptr``, and therefore the compiler
will be disabled.

By default, the oneAPI Construction Kit is configured with
:cmake:variable:`CA_RUNTIME_COMPILER_ENABLED` set to ``ON`` and
:cmake:variable:`CA_COMPILER_ENABLE_DYNAMIC_LOADER` set to ``OFF``.

Selecting a compiler implementation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A compiler implementation is represented by a singleton instance of a
``compiler::Info`` object. A list of all available compilers can be obtained by
calling ``compiler::compilers()``, whilst ``compiler::getCompilerForDevice`` can
be used to select the relevant compiler for a particular ``mux_device_info_t``.

Info
^^^^

The ``compiler::Info`` struct (``include/compiler/info.h``)  describes a
particular compiler implementation that can be used to compile programs for a
particular ``mux_device_info_t``. ``Info`` contains information about the
compiler capabilities and metadata, and additionally acts as an interface for
creating a ``compiler::Target`` object.

Context
^^^^^^^

The ``compiler::Context`` interface (``include/compiler/context.h``) serves as
an opaque wrapper over the LLVM context object. This object can also contain
other shared state used by compiler modules, and contains a mutex that is locked
when interacting with a specific instance of LLVM.

Target
^^^^^^

The ``compiler::Target`` interface (``include/compiler/target.h``) represents a
particular target device to generate machine code for. This object is also
responsible for creating instances of ``compiler::Module`` (described below).

Module
^^^^^^

The ``compiler::Module`` interface (``include/compiler/module.h``) is responsible
for driving the compilation process from source code all the way to machine
code. It acts as a container for LLVM IR by wrapping the LLVM Module object, and
executes the required passes.

Compile OpenCL C
~~~~~~~~~~~~~~~~

The clang frontend is instantiated in the ``compiler::Module::compileOpenCLC``
member function, this is where:

* The OpenCL C language options are specified to the frontend
* User specified macro definitions and include directories are set
* ``mux`` device force-include headers (if present) are set
* A diagnostic handler is provided to report compilation errors

This compilation stage also introduces the pre-compiled builtins header
providing the OpenCL C builtin function declarations to the frontend.
Compilation occurs when the ``clang::EmitLLVMOnlyAction`` is invoked, then
ownership of the resulting ``llvm::Module`` is transferred to
``compiler::Module`` to be used in the next stage. Any errors occurring
during compilation are returned in the error log specified during the
construction of ``compiler::Module``, where they can be queried by the
application.

.. note::
    In OpenCL, the ``compiler::Module::compileOpenCLC`` member function directly
    maps to ``clCompileProgram`` but is also invoked by ``clBuildProgram``.

Compile SPIR-V
~~~~~~~~~~~~~~

The ``compiler::Module::compileSPIRV`` member function implements the SPIR-V
frontend. First, the SPIR-V module is handed to ``spirv_ll::Context::translate``
to turn it into a ``llvm::Module``, then some additional fixup passes are applied.

Link
~~~~

During ``compiler::Module::link``, the LLVM module is first cloned before the list
of all provided ``compile::Module``\ 's are linked into the current module. As
before, during ``compiler::Module::compileOpenCLC``, a diagnostics handler is
specified. If linking was successful, the previous module is destroyed and the
linked modules ownership is moved to ``compiler::Module``.

.. note::
    In OpenCL, the ``compiler::Module::link`` member function directory maps to
    ``clLinkProgram`` but is also invoked by ``clBuildProgram``.

Finalize
~~~~~~~~

Finalization is the final compilation stage which executes any remaining LLVM
passes and getting it ready to be passed to the backend implementation. This is
where the majority of the LLVM passes are run, once again on a clone of the
``llvm::Module`` owned by the ``compiler::Module`` object. Once the
``llvm::PassManager`` has run all of the desired passes, the LLVM module is
ready to be turned into machine code, either through
``compiler::Module::createBinary``, or possibly deferred at runtime through the
``compiler::Kernel`` object.

Kernel
^^^^^^

The ``compiler::Kernel`` interface (``include/compiler/kernel.h``) represents a
single function entry point in a finalized ``compiler::Module``. It's main purpose
is to provide an opportunity for the backend to perform optimizations and code
generation as late as possible. Most of the work is driven by the
``compiler::Module::createSpecializedKernel`` method that creates a Mux runtime
kernel potentially optimized for a set of execution options that will be passed
to it during ``muxCommandNDRange``.

OpenCL C Passes
---------------

The ``compiler`` module provides a number of LLVM passes, which are specific to
processing the LLVM IR produced by clang after compiling OpenCL C source code.
The IR is processed into a form that the backend can consume. The passes are
described immediately below in the order they are executed by the LLVM pass
manager.

Fast Math
^^^^^^^^^

The OpenCL standard defines an optional ``-cl-fast-relaxed-math`` flag that can be
set when building programs, allowing optimizations on floating point arithmetic
that could violate the IEEE-754 standard. When this flag is used we run the LLVM
module level pass ``FastMathPass`` to perform these optimizations straight after
frontend parsing from clang.

First the pass looks for any ``llvm::FPMathOperator`` instructions and for those
found sets the ``llvm::FastMathFlags`` attribute to enable all of:


* Unsafe algebra - Operation can be algebraically transformed.
* No ``NaN``\s - Arguments and results can be treated as non-NaN.
* No ``Inf``\s - Arguments and results can be treated as non-Infinity.
* No Signed Zeros - Sign of zero can be treated as insignificant.
* Allow Reciprocal - Reciprocal can be used instead of division.

As well as the above ``compiler::FastMathPass`` replaces maths and geometric
builtin functions with fast variants. Any math builtin functions which have a
native equivalent are replaced with the native function, specified as having an
implementation defined maximum error. For example ``exp2(float4)`` is replaced
with ``native_exp2(float4)``.

Geometric builtins ``distance``, ``length``, and ``normalize`` are all defined in
OpenCL as having fast variants ``fast_distance``, ``fast_length``, and
``fast_normalize`` which use reduced precision maths. If any of these functions
are present we also replace them with the relaxed alternative.

These builtin replacements are done by searching the LLVM module for call
instructions which invoke the mangled name of a builtin function we want to
replace. If the fast version of the builtin isn't already in the module, i.e. it
wasn't called explicitly somewhere else, then we also need to add a function
declaration for the mangled name of the fast builtin. Finally a new call
instruction is created invoking the fast function declaration and the old call
it replaces is deleted.

Bit Shift Fixup
^^^^^^^^^^^^^^^

LLVM IR does not define the results of oversized shift amounts, however some
high-level languages such as OpenCL C do. As a result shift instructions need
to be updated to perform a 'modulo N' by the shift amount prior to the shift
operation itself, where N is the bit width of the value to shift.

``BitShiftFixupPass`` implements this as a LLVM function pass iterating over all
the function instructions looking for shifts. For each shift found the pass uses
the first operand to work out 'N' for the modulo based on the bit width of the
operand type. If the shift amount from the second operand is less than N
however, then we can skip the shift without inserting a modulo operation since
the shift is not oversized. We can also skip shift instructions that already
have the modulo applied, which can happen if the module was created by clang.
Otherwise the pass creates a modulo by generating a 'logical and' instruction
with operands ``N-1`` and the original shift amount, this masked value is then
used to replace the second operand of the shift.

Software Division
^^^^^^^^^^^^^^^^^

The compiler pass ``SoftwareDivisionPass`` is a function level pass designed to
prevent undefined behaviour in division operations. To do this the pass adds
runtime checks using ``llvm::CmpInst`` instructions for two specific cases, divide
by zero and ``INT_MIN / -1``. Due to the specification of undefined behaviour if
one of these cases is detected we are free to update the behaviour of the divide
operation. In both cases we set the divisor operand of the divide instruction to
be ``+1`` using a ``llvm::SelectInst`` with the original operand based on the result
of our checks.

Since IEEE-754 defines these error cases for floating point types our runtime
checks only need to be applied to integer divides. This is ensured in the pass
by checking if the instruction opcode is one of ``SDiv``, ``SRem``, ``UDiv``, ``URem``.
Whereas floating point divide instructions will have opcode ``FDiv`` or ``FRem``.

Image Argument Substitution
^^^^^^^^^^^^^^^^^^^^^^^^^^^

OpenCL image calls with opaque types are replaced to use those coming from the
image library.

MemToReg
^^^^^^^^

A manual implementation of LLVM's MemToReg pass, which promotes allocas
which have only loads and stores as uses to register references. This is needed
because after LLVM 5.0 ``llvm:MemToReg`` has regressed and is not removing all
the allocas it should be.

Builtin Simplification
^^^^^^^^^^^^^^^^^^^^^^

``BuiltinSimplificationPass`` is a module level pass for simplifying builtin
function calls. The pass performs two kinds of optimization on builtins:


* Converts builtins to more efficient variants where possible (for example, a
  call to the math function ``pow(x, y)``, where ``y`` is a constant that is
  representable by an integer, will be converted to ``pown(x, y)``).
* Replace builtins whose arguments are all constant (for example, a call to the
  math function ``cos(x)``, where ``x`` is a constant, will be replaced by a new
  constant value that is the calculation of the cosine of ``x``).

``printf`` Replacement.
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Of the myriad of architectures that have ComputeMux back ends, most do not have
access to an implementation of ``printf`` whereby they can route a call to
``printf`` within a kernel to ``stdout`` of the process running on the host CPU
processor.

To enable our ComputeMux back ends to call ``printf``, we provide an optimized
software implementation. An additional kernel argument buffer is implicitly
added to any kernel that uses ``printf``, and our implementation of ``printf``
that is run on the ComputeMux backend will write the results of the ``printf``
into this buffer instead. Then, when the kernel has completed its execution, the
data that was written to this buffer is streamed out on the host CPU processor
via ``stdout``.

Combine ``fpext`` ``fptrunc``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``CombineFPExtFPTruncPass`` is a function level pass, rather than a module pass,
for removing ``FPExt`` and ``FPTrunc`` instructions that cancel each out. This is
used after the ``printf`` replacement pass because var-args ``printf`` arguments
will be expanded to double by clang even if the device doesn't support doubles.
So if the device doesn't support doubles, the ``printf`` pass will ``fptrunc`` the
parameters back to float. ``CombineFPExtFPTruncPass`` will find and remove the
matching ``fpext`` (added by clang) and ``fptrunc`` (added by the ``printf`` pass) to
get rid of the doubles.

The pass is implemented by iterating over all the instructions looking for any
``llvm::FPExtInst`` instructions. If one is found then we check its uses, if the
``fpext`` is unused, remove it. Otherwise if the instruction only has one use and
it's a ``llvm::FPTruncInst`` then we can replace all uses of the ``fptrunc`` with
the first operand of ``fpext`` and delete both the ``fptrunc`` and ``fpext``.

Set ``convergent`` Attr
^^^^^^^^^^^^^^^^^^^^^^^

In clang the ``convergent`` attribute can be set on a function to indicate to
the optimizer that the function relies on cross work item semantics.  For
OpenCL we need this attribute to be set on the barrier function, for example,
since it's used to control the scheduling of threads.  Recent versions of clang
will proactively set such functions in OpenCL-C kernels as ``convergent``, but
we also set the attribute implicitly in the builtins header out of an abundance
of caution.

This pass iterates over all the functions in the module, including declarations
requiring the pass to be a module pass instead of a function pass. If the
function inspected may be convergent, identified by the compiler's
``BuiltinInfo`` analysis, then we assign the ``llvm::Attribute::Convergent``
attribute to it. When the pass encounters a convergent function, all functions
calling that function are transitively marked convergent.
