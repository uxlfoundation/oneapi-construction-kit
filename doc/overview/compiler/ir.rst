Intermediate Representation
===========================

ComputeMux assumes `LLVM IR <https://llvm.org/docs/LangRef.html#introduction>`_
as its intermediate representation. The precise specification of the
intermediate representation is dependent on the :ref:`version of LLVM
<overview/compiler/supported-llvm-versions:Supported LLVM Versions>` ComputeMux
has been built against.

While ComputeMux itself does not mandate a specific producer of the IR it
consumes, it is assumed to meet certain requirements, e.g., that it comes from
a supported higher-level form, including -- but not limited to -- a tool
converting SPIR-V to LLVM IR or a ``clang`` compiler compiling OpenCL C or SYCL
for the SPIR "triple".

This section describes the various conventions on IR that a ComputeMux compiler
**may** encounter and **must** support during code generation. For a list of
language features a compiler is **not** required to support, see
:ref:`overview/compiler/non-requirements:ComputeMux Compiler Non-Requirements`.

Types
-----

The following tables describe the ComputeMux "built-in" scalar and vector types
and how they are mapped to the LLVM IR type system. They stem from the
higher-level languages supported by ComputeMux, including OpenCL C and SPIR-V.

Built-in Scalar Types:

+--------------------------------+--------------+
| OpenCL C types                 | LLVM IR type |
+================================+==============+
| ``bool``                       | ``i1``       |
+--------------------------------+--------------+
| (``unsigned``) ``char``        | ``i8``       |
+--------------------------------+--------------+
| (``unsigned``) ``short``       | ``i16``      |
+--------------------------------+--------------+
| (``unsigned``) ``int``         | ``i32``      |
+--------------------------------+--------------+
| (``unsigned``) ``long`` [#f1]_ | ``i64``      |
+--------------------------------+--------------+
| ``float``                      | ``float``    |
+--------------------------------+--------------+
| ``double`` [#f2]_              | ``double``   |
+--------------------------------+--------------+
| ``half`` [#f3]_                | ``half``     |
+--------------------------------+--------------+

Built-in Vector Types:

+-------------------------------+------------------+
| OpenCL C type                 | LLVM IR type     |
+===============================+==================+
| ``bool``\ *n* [#f4]_          | ``<n x i1>``     |
+-------------------------------+------------------+
| [``u``]\ ``char``\ *n*        | ``<n x i8>``     |
+-------------------------------+------------------+
| [``u``]\ ``short``\ *n*       | ``<n x i16>``    |
+-------------------------------+------------------+
| [``u``]\ ``int``\ *n*         | ``<n x i32>``    |
+-------------------------------+------------------+
| [``u``]\ ``long``\ *n* [#f1]_ | ``<n x i64>``    |
+-------------------------------+------------------+
| ``float``\ *n*                | ``<n x float>``  |
+-------------------------------+------------------+
| ``double``\ *n* [#f2]_        | ``<n x double>`` |
+-------------------------------+------------------+
| ``half``\ *n* [#f3]_          | ``<n x half>``   |
+-------------------------------+------------------+

.. rubric:: Footnotes

.. [#f1] An optional type. See `Optional 64-bit integer support`_
.. [#f2] An optional type. See `Optional 64-bit double support`_
.. [#f3] An optional type. See `Optional 16-bit half support`_
.. [#f4] A reserved data type in OpenCL C, but one that is common in LLVM IR

Aside from optional types, which have specific conditions detailed below, the
aforementioned built-in types **must** be supported by a conforming ComputeMux
compiler implementation. Note that "support" in this context **does not** mean
requiring hardware for all built-in types; exactly how types are supported is
ultimately left to the compiler implementation but it is assumed that a
ComputeMux compiler can legalize any unsupported types.

.. tip::
  Compilers targeting architectures which lack hardware support for certain
  built-in types **may** typically rely on LLVM's built-in
  ``SelectionDAG``-based legalization framework to handle illegal types and
  operations during instruction selection.

  Illegal vector types are usually scalarized or "unrolled". Illegal integer
  types are usually either extended to larger integers or split into multiple
  operations over smaller ones. Illegal floating-point types typically rely on
  the external LLVM compiler support library `compiler-rt
  <https://compiler-rt.llvm.org/>`_.

  Compilers **may** customize this legalization process if there is a better
  approach for their target architecture.

  A brief overview of the instruction selection process is described `here
  <https://releases.llvm.org/docs/CodeGenerator.html#selectiondag-instruction-selection-process>`_.

Other types
~~~~~~~~~~~

The built-in types have been detailed above only to describe their
relative importance and are *not* the only types a ComputeMux compiler **may**
encounter as part of a conformant implementation. Pointer types, the ``void``
type, array types, structure types and union types **must** all be correctly
handled.

Aside from both scalar and vector ``double`` and ``half`` types, ComputeMux
makes no guarantees about the presence or absence of operations on non-built-in
integer, floating-point or vector types in the IR.

For example, if :ref:`whole-function vectorization <compiler-ir-wfv>` is
enabled, operations on vector types wider than 16 **may** be introduced into
the IR. LLVM itself has been known to turn ``double2`` (``<2 x double>``)
vectors into scalar 128-bit integers (``i128``), and **may** reduce switch
statement values into arithmetic over 4-bit integers (``i4``).

.. note::
  Both ComputeMux and LLVM's middle-end optimizations **may** introduce
  "illegal" types into the module. The concept of "legal types" is not
  generally observed at that level. For instance, some LLVM intrinsics *only*
  take ``i64`` parameters. The reason for this, as mentioned above, is that the
  contract on an LLVM backend is that it should always be able legalize the
  standard operations and intrinsics given any type.

Optional 64-bit integer support
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For targets implementing the embedded profile without supporting the optional
64-bit integer types (``cles_khr_int64``), operations on 64-bit integers **may
still be present in the IR**.

Optional 16-bit half support
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Targets without support for the optional ``half`` type (``cl_khr_fp16``)
**shall not** see arithmetic, conversion or relational operations on neither
scalar nor vector ``half`` types anywhere in the IR module.

.. note::
  Since ``half`` can always be used as a storage format, other operations and
  built-in functions **may** operate on ``half`` values, such as loads and
  stores.

Optional 64-bit double support
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Targets without support for the optional ``double`` type **shall not** see
neither scalar nor vector ``double`` types anywhere in the IR module.

.. _compiler-ir-wfv:

Vector Types and Whole-Function Vectorization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If WFV is enabled, the vectorizer may emit vectors wider than the requested
vectorization factor, if vectors already exist in the incoming IR. The
vectorizer may emit vectors wider than 16, if the target reports that it has
vector registers wide enough to contain such a wider type.

The vectorizer is able to produce scalable vectors (being a vector of the form
``<vscale x n x Ty>`` where ``n`` is a constant factor known at compile-time,
and ``vscale`` being a hardware-dependent factor that can be queried at
runtime) on requesting a scalable vectorization factor. Support for scalable
vectors is not a requirement. Scalable vectors are available starting from LLVM
12, but LLVM 13 is recommended since support in LLVM 12 is very limited.

.. seealso::
  See the overview of
  :ref:`overview/example-scenarios/mapping-algorithms-to-vector-hardware:The
  Vecz Whole-Function Vectorizer` for more information about this particular
  compiler optimization.

Integer and Floating-point Operations
-------------------------------------

The LLVM IR module **shall** contain standard built-in IR instructions which
**may** form some proportion of the "compute" operations in the kernel being
compiled. That is to say the kernel **may not** be entirely comprised of
library functions and compiler intrinsics. The specific language features which
produce these built-in instructions (e.g., operators: see below) depend on a
variety of factors: the compute standard being implemented; the compile
options; the compiler frontend; the version of LLVM being used; any
higher-level intermediate representation being lowered to LLVM IR.

These standard LLVM instructions each have their own semantics as described in
the `LLVM language reference manual <https://llvm.org/docs/LangRef.html>`_.
ComputeMux **shall not** infer any *contradictory* semantics about these
instructions based upon the compute standard it is compiling for. ComputeMux
**shall** fundamentally act as a well-behaving LLVM compiler. Additional
requirements **may** be placed on top of these instructions depending on the
compute standard being implemented, such as `Floating-point Precision
Requirements`_.

.. tip::
  When implementing the OpenCL compute standard in conjunction with the *clang*
  frontend, all built-in operators **shall** be emitted into the IR as standard
  LLVM IR instructions. This includes add (``+``), subtract (``-``), multiply
  (``*``), divide (``/``), unary operators (``+``, ``-``), relational operators
  (``<``, ``>=``, etc.), equality operators (``==``, ``!=``), logical operators
  (``&&``, ``||``), ternary selection (``?:``), and unary logical not (``!``)
  for all built-in scalar and vector types. Additional operators -- for integer
  types only -- include remainder (``%``), shift (``<<``, ``>>``), pre- and
  post-increment (``--``, ``++``), bitwise and (``&``), bitwise or (``|``),
  bitwise exclusive or (``^``), and bitwise not (``~``).

  There is no guarantee that a given operator is mapped to a specific IR
  instruction. While the naive lowering of subtraction (``-``) to LLVM's
  ``sub`` is likely, it is also possible for transformations and optimizations
  to change the ``sub`` of a constant to an ``add`` of the negative constant,
  for example. A ternary expression **may** be identified as a min/max
  operation and represented by an intrinsic, e.g., ``llvm.umin.*``.

  See `OpenCL C Operators
  <https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#operators>`_
  for a full list of the operators described above.

Presented here is an *incomplete* list of instructions which often arise from
the compute standards supported by ComputeMux and which targets **shall** be
expected to handle for **all** supported scalar and vector built-in `Types`_:
the `binary operations <https://llvm.org/docs/LangRef.html#binary-operations>`_
including integer ``add``, ``sub``, ``mul``, ``udiv``, ``sdiv``, ``urem``,
``sdiv``; floating-point ``fadd``, ``fsub``, ``fmul``, ``fdiv``, and ``frem``;
the `bitwise binary operations
<https://llvm.org/docs/LangRef.html#bitwise-binary-operations>`_ ``and``,
``or``, ``xor``, ``shl``, ``lshr``, ``ashr``. Relational and equality
operations are commonly lowered as ``icmp`` or ``fcmp`` instructions.

.. important::
  The four basic floating-point operators -- commonly rendered into LLVM IR as
  ``fadd``, ``fsub``, ``fmul``, and ``fdiv`` instructions -- are special in
  that ComputeMux **does not** provide software implementations to help achieve
  precision requirements. See :ref:`here <overview/toolkit:Requirements>` for
  more information. An example of the precision requirements that OpenCL
  conformance places upon these four operators is given below.

.. tip::
  Targets without hardware support for the fifth operator ``frem`` **may** wish
  to replace all such instructions with calls to the OpenCL built-in ``fmod``
  function or the :ref:`overview/toolkit:Abacus` equivalent, which both have
  overloads for all OpenCL built-in vector `Types`_. This may be preferable to
  LLVM's standard expansion of the instruction which may scalarize vector
  types.

Floating-point Precision Requirements
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The LLVM IR instructions will have precision requirements that vary according
to the higher-level programming language being implemented. These precision
requirements **must** be adhered to for conformance, but note that there may be
:ref:`profiles <overview/hardware/floating-point-requirements:OpenCL Full
Profile ULP>`, :ref:`compiler options
<overview/hardware/floating-point-requirements:Optimization Options>`, or
specific maths library functions (e.g., `native functions in OpenCL
<https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#math-functions>`_)
to relax these requirements when used in performance-sensitive code.

OpenCL Conformance
******************

The OpenCL *full-profile* precision requirements for floating-point arithmetic
operations (for both scalar and vector) are summarized in the table below.

.. note::
  This table is a summary of requirements for the *full* profile. The embedded
  profile and/or presence of certain compilation options may loosen these
  requirements. See `Relative Error as ULPs
  <https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#relative-error-as-ulps>`_
  for the definition of ULP and precision requirements for other profiles,
  compilation flags, and other built-in floating-point operations. See
  the same section in the `cl_khr_fp16 documentation
  <https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_Ext.html#cl_khr_fp16-relative-error-as-ulps>`_
  for the precision requirements on `half` data types.

+----------+-------------------+-------------------------+-----------------------+
| Operator | Min accuracy - ULP values                                           |
+          +-------------------+-------------------------+-----------------------+
|          | Single-precision  | Double-precision [#f5]_ | Half-precision [#f5]_ |
+==========+===================+=========================+=======================+
| ``fadd`` | Correctly rounded | Correctly rounded       | Correctly rounded     |
+----------+-------------------+-------------------------+-----------------------+
| ``fsub`` | Correctly rounded | Correctly rounded       | Correctly rounded     |
+----------+-------------------+-------------------------+-----------------------+
| ``fmul`` | Correctly rounded | Correctly rounded       | Correctly rounded     |
+----------+-------------------+-------------------------+-----------------------+
| ``fdiv`` | <= 2.5 ulp        | Correctly rounded       | <= 1 ulp              |
+----------+-------------------+-------------------------+-----------------------+

.. rubric:: Footnotes

.. [#f5] Support for both half and double is optional

.. seealso::
  See :doc:`/overview/hardware/floating-point-requirements` for details on
  floating point precision requirements for hardware.

.. _compiler-ir-intrinsics:

Intrinsics
----------

LLVM includes the notion of `intrinsic functions
<https://llvm.org/docs/LangRef.html#intrinsic-functions>`_ which serve to
extend the capabilities of stock LLVM IR. ComputeMux provides a number of
points at which intrinsics may be emitted into the IR.

Some intrinsics **may** be immediately present in the IR consumed by
ComputeMux, either generated by the compiler frontend or created during
conversion from a higher-level intermediate representation such as SPIR-V.

The standard ComputeMux pass pipeline includes several standard LLVM
optimizations such as ``InstCombine`` which are known to introduce intrinsics
into the IR module. This pass pipeline is ultimately *configurable* and so
passes can be added, removed or reordered as required. Doing so may introduce
or remove unspecified intrinsics into the IR module.

.. tip::
  A compiler backend can usually rely on LLVM's standard legalization framework
  to expand any unsupported intrinsics into a set of supported operations. If
  an architecture has native support for any intrinsics then custom code will
  be required to 'lower' them to target-specific instruction sequences.

If ComputeMux's :ref:`whole-function vectorizer <compiler-ir-wfv>` (WFV) pass
is enabled, the vectorizer **may** emit the vector reduction intrinsics
``@llvm.vector.reduce.and.*`` or ``@llvm.vector.reduce.or.*``. Up to and
including LLVM 11, if these are not supported by the target, alternative code
will be generated, so supporting these is not a requirement. As of LLVM 12,
these intrinsics **may** be emitted *regardless of the target capabilities*.

If WFV is enabled, load instructions **may** be vectorized to the intrinsics
``@llvm.masked.load.*`` or ``@llvm.masked.gather.*``. Store instructions
**may** be vectorized to the intrinsics ``@llvm.masked.store.*`` or
``@llvm.masked.scatter.*``. These are emitted through an externally-exposed
interface, so the default behaviour **may** be overridden by a target-specific
implementation, if required.

If WFV is enabled, certain operations on scalable vectors other than loads and
stores **shall** use ``@llvm.masked.scatter.*`` and ``@llvm.masked.gather.*``
intrinsics.

If WFV is enabled, the vectorizer **may** emit a number of other
target-independent intrinsics commonly generated by LLVM's middle-end
optimizations (e.g., ``@llvm.maxnum``, ``@llvm.fshr``, etc.), but only if they
exist in the incoming IR.

.. _compiler-ir-address-spaces:

Address Spaces
--------------

In LLVM IR, pointer types are considered as pointing to a particular "address
space" denoted by an integral value, e.g., ``i32 addrspace(2)*``. Address
spaces conceptually describe different regions of memory which may not
necessarily be uniformly addressable. The default address space is the number
zero and the semantics of non-zero address spaces are target-specific. In
LLVM's type system, two otherwise equivalent pointer types are unequal if they
point to different address spaces and may not be used interchangeably.

See :ref:`overview/hardware/memory-requirements:Address Spaces` for a
conceptual overview of the address spaces ComputeMux recognizes and how these
address spaces may be mapped to hardware.

In LLVM IR, ComputeMux maps these address spaces to the numbers 0-4:

+---------------+-----------+-----------------+
| Address Space | OpenCL    | SPIR-V          |
+===============+===========+=================+
| 0             | private   | Function        |
+               +           +-----------------+
|               |           | Private         |
+               +           +-----------------+
|               |           | AtomicCounter   |
+               +           +-----------------+
|               |           | Input           |
+               +           +-----------------+
|               |           | Output          |
+---------------+-----------+-----------------+
| 1             | global    | Uniform         |
+               +           +-----------------+
|               |           | CrossWorkgroup  |
+               +           +-----------------+
|               |           | Image           |
+               +           +-----------------+
|               |           | StorageBuffer   |
+---------------+-----------+-----------------+
| 2             | constant  | UniformConstant |
+               +           +-----------------+
|               |           | PushConstant    |
+---------------+-----------+-----------------+
| 3             | local     | Workgroup       |
+---------------+-----------+-----------------+
| 4             | generic   | Generic         |
+---------------+-----------+-----------------+

Targets **shall not** use these address space numbers for any other purpose.

.. note::
  The conventions in LLVM surrounding address spaces 0-3 stem from the SPIR 1.2
  specification.

  https://www.khronos.org/registry/SPIR/specs/spir_spec-1.2.pdf

Targets **may** use any of the unused address space numbers for their own
purposes. It is recommended that address space numbers above 100 are used to
better accommodate future specifications.

ComputeMux **shall not** make assumptions about how address spaces map to the
target architecture and how conversions between them behave. Conversions
between address spaces in the IR **shall** be preserved. Targets **shall**
expect that ``addrspacecast`` instructions **may** occur in programs in all
supported versions of LLVM. It is up to the target to lower these instructions
accordingly.

.. tip::
  It is common that a ComputeMux compiler backend is targeting an architecture
  where not all address spaces are distinct memory regions, e.g., one with a
  unified address space, or more generally one for which a given pair of
  address spaces 0 to 4 are known to share the same addressable region.

  In this instance, it is recommended that the distinct address spaces are
  *maintained* on pointer types. An LLVM compiler backend does not care for
  particular address spaces and they should not interfere with any
  optimizations.

  A common sticking point is in fact the ``addrspacecast`` instructions which
  hit the instruction selector and expect a lowering to target-specific
  instructions. The target **may** override
  ``TargetMachine::isNoopAddrSpaceCast``. This method allows LLVM to
  automatically elide ``addrspacecast`` instructions during instruction
  selection.

Alignment
---------

As is required by the semantics of LLVM IR, a target **must** adhere to the
alignments specified on operations. These are most commonly found as either the
``align`` argument (or ``!align`` metadata) on instructions including
``alloca``, ``load``, ``store``, etc., or the alignment parameter on
:ref:`intrinsics <compiler-ir-intrinsics>` such as ``@llvm.masked.gather.*``
and ``@llvm.masked.scatter.*``.

.. seealso::
  The `LLVM language reference manual <https://llvm.org/docs/LangRef.html>`_
  contains the full details on how alignment is expressed by each operation.

The specific alignments on each operation are ultimately defined by the
higher-level programming environment that ComputeMux is compiling for.

.. note::
  Built-in datatypes in OpenCL `are specified as being aligned to their own
  size in bytes
  <https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#alignment-of-types>`_.

  Note that vectors of 3 elements are specified as having a size as if they had
  4 elements. Therefore their alignment is that of the equivalent 4-element
  vector.

  It is for this reason that the following accesses **must** be supported for
  any ComputeMux implementation that is required to support OpenCL conformance:

  * 1-byte-aligned ``i8``
  * 2-byte-aligned ``i16``, ``half``, ``<2 x i8>``, etc.
  * 4-byte-aligned ``i32``, ``float``, ``<2 x i16>``, etc.
  * 8-byte-aligned ``i64``, ``double``, ``<3 x half>``, etc.
  * 16-byte-aligned ``<2 x i64>``, ``<4 x i32>``, etc.
  * 32-byte-aligned ``<4 x i64>``, ``<8 x i32>``, etc.
  * 64-byte-aligned ``<8 x i64>``, ``<16 x i32>``, etc.
  * 128-byte-aligned ``<16 x i64>``, ``<16 x double>``, etc.

  Refer back to the section on `Types`_ to see how higher-level types are
  lowered to the IR types mentioned above.

These alignments **may** not coincide with the target hardware's natural
alignment capabilities. It is the compiler's job to correctly lower accesses
that are unaligned with respect to the target hardware. This is typically done
in the compiler backend during instruction selection.

.. tip::
  A compiler backend may have to support storing an ``i8`` to a byte-aligned
  address by loading a naturally-aligned word from around the destination
  pointer, blending in the ``i8`` value (e.g., using masks and shifts), and
  storing the blended word back to memory.

  Note that this workaround may have to be done atomically, e.g., if work-items
  in a work-group are executed in parallel. See :ref:`an overview of work-group
  scheduling
  scenarios<overview/example-scenarios/mapping-algorithms-to-vector-hardware:Work-group
  Scheduling>` for more information.

Note that this -- in conjunction with the :ref:`private address space 0
<compiler-ir-address-spaces>` conceptually being mapped :ref:`to the stack
<overview/hardware/memory-requirements:private>` -- means that the compiler
**may** have to handle stack objects which require a high alignment, e.g., 128
bytes for OpenCL conformance (see above).

.. tip::
  This **may** involve dynamically realigning the stack from its natural ABI
  alignment in the function prologue.

ComputeMux
~~~~~~~~~~

ComputeMux **shall** preserve any existing alignment when mutating existing
memory accesses.

.. important::
  If :ref:`whole-function vectorization <compiler-ir-wfv>` is enabled, this
  means that vectorized access **shall** maintain the alignment of the original
  accesses. Therefore this pass may introduce vector accesses whose alignment
  is *smaller* than their size in bytes.

  For example, when vectorizing by a factor of 8, ``i8 align 1`` **may** be
  vectorized to ``<8 x i8> align 1`` and ``<2 x i16> align 4`` **may** be
  vectorized to ``<16 x i16> align 4``.

All *new* memory accesses created by ComputeMux compiler passes **shall** use
the alignment specified by the target's `data layout string
<https://llvm.org/docs/LangRef.html#data-layout>`_ contained in the LLVM IR
module and so **shall** be correctly aligned for the target architecture.

User control over alignment
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Users of OpenCL can additionally explicitly specify minimum alignment on
``enum``, ``struct`` and ``union`` types using the `aligned attribute
<https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#specifying-attributes-of-types>`_.
This attribute is optional and **may** be supported in a target-specific way as
part of conformant ComputeMux implementation.

Debug Info
----------

ComputeMux Compiler expects standard `LLVM IR debug metadata`_ to be used as the
format for source debug information. The reusable passes provided by
oneAPI Construction Kit to ComputeMux Compiler targets make a best-effort attempt to
preserve debug info, but no guarantees are provided.

.. tip::
  In OpenCL, the ``-cl-opt-disable`` `Compilation Option`_ can be used by
  developers to disable optimizations for a better debugging experience. The
  default is optimizations are enabled. oneAPI Construction Kit uses this flag to
  skip front-end compiler transformations used for performance, but places no
  requirements on the ComputeMux Compiler implementation to act on the flag.

LLVM debug information is designed to be agnostic regarding the final format and
target debugger. oneAPI Construction Kit does nothing to compromise this, and it is
at the discretion of the ComputeMux Compiler back-end to choose the most suitable
output format for the target, e.g. DWARF, Stabs, etc.

Debug information metadata is not used for any other purpose in the oneAPI
Construction Kit and **may** be discarded by a ComputeMux Compiler target without
sacrificing either correctness or performance.

In the future, the ComputeMux Compiler specification may define functions the
target can optionally implement for the purposes of debugging. For example, with
target specific debugger hooks.

.. note::
  OpenCL 1.2 does not provide a `Compilation Option`_ to developers to enable
  debug information in the kernel. Instead, oneAPI Construction Kit provides the
  ``cl_codeplay_extra_build_options`` OpenCL extension which introduces the
  following options (amongst others) to aid debugging:

  ``-g``
    Build program with debug info.

  ``-S <path/to/source/file>``
    Point debug information to a source file on disk. If this does not exist,
    the runtime creates the file with cached source.

  These options make use of existing LLVM debug info metadata, and place no
  additional responsibilities on the ComputeMux Compiler target.

.. _Compilation Option:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#optimization-options

.. _LLVM IR debug metadata:
  https://llvm.org/docs/SourceLevelDebugging.html

DMA
---

The ComputeMux compiler specification defines several DMA builtins that a
compiler implementation **should** provide in the form of an IR pass or
library. A target that does not provide definitions of the DMA builtins cannot
take advantage of the optimizations described below.

Defining these builtins using platform specific DMA features enables optimized
memory operations in any frameworks built on top of Mux.

For targets unable to support hardware DMA oneAPI Construction Kit provides
software implementations of the DMA builtins in the form of compiler passes
that any target may use. Software implementations of the DMA builtins **may**
have a performance overhead and any target that can provide platform optimized
implementations of the builtins **should** do so.

A full list of the DMA builtins along with their signatures and semantics can
be found in the :ref:`Builtins <specifications/mux-compiler-spec:Builtins>`
section of the ComputeMux compiler specification.

Atomics and Fences
------------------

Atomic and fence instructions **shall** be emitted into the IR consumed by a
ComputeMux compiler implementation. As outlined in the
:ref:`overview/hardware/atomic-requirements:Atomics and Fences`
section, if a target has hardware support for atomic operations it **should**
map them to these instructions. As a fallback, if a target does not support
hardware atomics or fences it **may** implement these instructions in software
using synchronization primitives such as mutexes.

For a full list of the atomic and fence instructions a ComputeMux compiler
implementation **must** handle see the
:ref:`specifications/mux-compiler-spec:Atomics and Fences` section of the
CompilerMux specification.

The required set of instructions allows the oneAPI Construction Kit to support
the `OpenCL C atomic`_ and `OpenCL C fence`_ operations and the `SPIR-V atomic`_
and `SPIR-V barrier`_ operations. Synchronization on non-atomic memory access
is defined by a *memory consistency model*. The memory consistency requirements
made on the instruction listed in the Mux compiler spec enables the oneAPI
Construction Kit to support the higher level `OpenCL memory consistency model`_
and the `Vulkan memory model`_.

.. _OpenCL C atomic:
   https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#atomic-functions
.. _OpenCL C fence:
   https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#_overview_of_atomic_and_fence_operations
.. _SPIR-V atomic:
   https://www.khronos.org/registry/SPIR-V/specs/unified1/SPIRV.html#_a_id_atomic_a_atomic_instructions
.. _SPIR-V barrier:
   https://www.khronos.org/registry/SPIR-V/specs/unified1/SPIRV.html#_a_id_barrier_a_barrier_instructions
.. _OpenCL memory consistency model:
   https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#_memory_consistency_model_for_opencl_1_x

Barriers
--------

The ComputeMux compiler specification defines a set of barrier builtins which
provide a limited ability to synchronize between kernel execution threads.
These are designed to support the barrier functions defined in the `OpenCL C
specification`_. A full list of these builtins as we define them, and a brief
description of their semantics can be found in the
:ref:`specifications/mux-compiler-spec:builtins` section of the ComputeMux
compiler specification.

.. _OpenCL C specification:
   https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#synchronization-functions

ComputeMux provides a compiler pass that transforms kernels containing barriers
such that execution and memory dependencies created by them can be satisfied
without the need for synchronization primitives on the device. This pass makes
use of the fence instructions described in the section above, so barrier
support can benefit from hardware support for such operations. Instead of
relying on the compiler pass, a ComputeMux implementation **may** choose to
implement these builtins with supporting hardware features, as mentioned in the
:doc:`/overview/hardware/synchronization-requirements` section.

Builtins
--------

The ComputeMux compiler has the notion of *builtin functions*. These are
functions that are known by the compiler to exhibit certain semantics and
properties which are useful or essential for the purposes of compilation. These
*always* include the functions defined by :ref:`overview/toolkit:Abacus`. In
addition -- depending on the higher-level language being compiled for -- other
builtin functions are recognized; for example, for OpenCL and SYCL,
the `OpenCL Builtin Functions`_ are considered by ComputeMux to be builtin.

The definitions of builtins **shall** be assumed to be provided by ComputeMux.
ComputeMux **may** modify the implementation of a builtin
function at any point in the compilation pipeline according to its own needs.
It **may** do so regardless of whether that builtin is already defined in the
module being compiled.

.. tip::
  If users wish to provide their own implementations of builtin functions, they
  should do so using a *new* function definition which is not recognized by
  ComputeMux as a builtin. For example, an optimized ``fma`` may safely be
  implemented as ``my_fma``, provided users replace all calls to ``fma`` to the
  new function.

.. _OpenCL Builtin Functions:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#built-in-functions
