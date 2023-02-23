Floating-Point Requirements
===========================

This section describes the floating-point requirements to support OpenCL though
ComputeAorta for a new device.

.. important::
  Floating-point is not required to be natively implemented in hardware, support
  may be emulated to meet conformance. Fast-math mechanisms exposed to
  developers by ComputeAorta allow high performance code to be written using the
  precision that exists on hardware.

OpenCL C mandates use of IEEE 754-2008 as the floating-point format to be
`Numerically Compliant`_ for conformance, and therefore a device should
support IEEE-754. However, floating-point exceptions are disabled in OpenCL and
hardware is not expected to handle these. Other optional parts of the IEEE-754
standard are denormal numbers and fused multiply-add.

If hardware makes use of any IEEE-754 alternative floating-point formats such
as `bfloat16`_ or `posit`_, then the
:doc:`/overview/compiler/computemux-compiler` interface can be extended to
support these as part of the work done by Codeplay. ComputeAorta would then
expose these types to the user through language extensions, e.g. an OpenCL C
extension.

.. _Numerically Compliant:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#opencl-numerical-compliance
.. _bfloat16:
  https://en.wikipedia.org/wiki/Bfloat16_floating-point_format
.. _posit:
  https://en.wikipedia.org/wiki/Unum_(number_format)#Unum_III

Floating Point Types
--------------------

OpenCL defines three different floating-point precision formats which can
optionally be exposed to developers through
:doc:`/overview/compiler/computemux-compiler`. These are not required
internally, but if present in hardware then ComputeAorta will make them
available.

Half Precision
  16-bit floats, a format sometimes known as *fp16*. **Not required** for a
  device to support OpenCL, this format is optionally available to the user via
  the `cl_khr_fp16`_ extension.

Single Precision
  32-bit floats, **required** for devices to support OpenCL.

Double Precision
  64-bit floats, **not required** for a device to support OpenCL as it is an
  optional feature.

Hardware only needs to handle the scalar datatypes for these formats. ComputeMux
Compiler will handle vector datatypes in the way most applicable to the hardware.

.. tip::
  32-bit single-precision floating point is mandatory across all OpenCL profiles
  unless of device type `CL_DEVICE_TYPE_CUSTOM`_. Custom OpenCL devices are
  **not** conformant, as they do not fully support OpenCL C. Integer-only
  hardware may either report as a customer device or use a software
  floating-point library in :doc:`/overview/compiler/computemux-compiler`.

.. note::
  ComputeAorta tests all three of these IEEE-754 floating-point precisions for
  OpenCL. As `cl_khr_fp16`_ is an OpenCL extension, the OpenCL
  `Conformance Test Suite`_ doesn't provide any conformance testing, instead
  ComputeAorta contains its own comprehensive unit testing for the extension.

.. _cl_khr_fp16:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_Ext.html#cl_khr_fp16
.. _Conformance Test Suite:
  https://github.com/KhronosGroup/OpenCL-CTS
.. _CL_DEVICE_TYPE_CUSTOM:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#CL_DEVICE_TYPE_CUSTOM

Operations
~~~~~~~~~~

OpenCL compliance defines precision for the add (``+``), subtract (``-``),
multiply (``*``), and division (``/``) operations on floating-point types.
A software implementation of these is not a feature of the
:ref:`overview/toolkit:Abacus` library, but a ComputeMux Compiler target may
still implement these in software if hardware is not precise enough. Division
is the most likely candidate for this as it has stricter precision required for
conformance compared to the other operations.

.. seealso::
  See :ref:`overview/compiler/ir:floating-point precision requirements`
  for the precision requirements of these operations.

Conversions
~~~~~~~~~~~

:ref:`overview/toolkit:Abacus` provides a software implementation of all the
conversions operations defined in OpenCL, including those taking floating-point
types as an input/output. If hardware doesn't provide native support for
conversions then the :doc:`/overview/compiler/computemux-compiler`
implementation may use the Abacus software conversions instead.

Optimization Options
--------------------

Developers can pass options to the kernel code compiler when writing high
performance programs to enable floating-point optimizations. In OpenCL, the
`Optimization Options`_ relating to floating-point behaviour are:

``-cl-mad-enable``
  Allow ``a * b + c`` to be replaced by a **mad** instruction. The **mad**
  instruction may compute ``a * b + c`` with reduced accuracy in the embedded
  profile. On some hardware the **mad** instruction may provide better
  performance than the expanded computation.

``-cl-no-signed-zeros``
  Allow optimizations for floating-point arithmetic that ignore the signedness
  of zero. IEEE-754 arithmetic specifies the distinct behavior of ``+0.0`` and
  ``-0.0`` values, which then prohibits simplification of expressions such as
  ``x + 0.0`` or ``0.0 * x`` (even with ``-cl-finite-math-only``). This option
  implies that the sign of a zero result isn't significant.

``-cl-unsafe-math-optimizations``
  Allow optimizations for floating-point arithmetic that (a) assume that
  arguments and results are valid, (b) may violate the IEEE-754 standard, (c)
  assume relaxed OpenCL numerical compliance requirements as defined in the
  unsafe math optimization section of the OpenCL C or OpenCL SPIR-V Environment
  specifications, and (d) may violate edge case behavior in the OpenCL C or
  OpenCL SPIR-V Environment specifications. This option includes the
  ``-cl-no-signed-zeros``, ``-cl-mad-enable``, and ``-cl-denorms-are-zero``
  options.

``-cl-finite-math-only``
  Allow optimizations for floating-point arithmetic that assume that arguments
  and results are not NaNs, +Inf, -Inf. This option may violate the OpenCL
  numerical compliance requirements for single precision and double precision
  floating-point, as well as edge case behavior.

``-cl-fast-relaxed-math``
  Sets the optimization options ``-cl-finite-math-only`` and
  ``-cl-unsafe-math-optimizations``. This option causes the preprocessor macro
  ``__FAST_RELAXED_MATH__`` to be defined in the OpenCL program.

All these options are passed through the
:doc:`/overview/compiler/computemux-compiler` interface in ComputeAorta for a
ComputeMux target to optimize as appropriate. It is also possible for
ComputeAorta to provide new non-standard optimization options to the developer
for enabling hardware specific optimizations.

.. _Optimization Options:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#optimization-options

Builtin Maths Functions
-----------------------

Compute languages often define builtin functions for use in kernel code, of
particular relevance to floating-point is the domain of builtins relating to
mathematical operations on scalar and vector types. These maths builtins have
associated precision requirements which must be met for an implementation to be
conformant, but this level of precision is **not** required for high performance
code.

Faster, but less accurate maths builtins are also available to the user in
in OpenCL for writing high performance code. ComputeAorta uses these to expose
the true hardware capabilities without any overhead for extra precision.
A developer can therefore choose the level of maths precision they need for
their application, faster native precision or conformant high precision.

.. tip::
  The OpenCL single precision `math functions`_ contain a set of functions
  prefixed with ``native_``, of implementation-defined accuracy, which can be
  used by :doc:`/overview/compiler/computemux-compiler` for exposing
  high-performance device instructions.

ComputeAorta provides the :ref:`overview/toolkit:Abacus` maths library
which implements OpenCL `math functions`_ to specification required precision.
This can be used as a software implementation of builtins where hardware isn't
available or does not meet precision requirements.

.. note::
  In SPIR-V the math functions are defined in `SPIR-V Extended Maths
  Instructions`_ as part of the OpenCL extended instruction set.

.. _SPIR-V Extended Maths Instructions:
  https://www.khronos.org/registry/spir-v/specs/unified1/OpenCL.ExtendedInstructionSet.100.html#_a_id_math_a_math_extended_instructions

.. _math functions:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#math-functions


OpenCL Full Profile ULP
~~~~~~~~~~~~~~~~~~~~~~~

High-level compute languages use *profiles* to mandate different sets minimum
capabilities that a device must support to be conformant. This allows the
compute language to be applicable across a range of domains which have each
have separate concerns.

The default profile of OpenCL is *Full Profile* intended for less constrained
domains, and as a result the precision requirements of `math functions`_ are
fairly strict so that OpenCL is applicable to the scientific computing domain.

OpenCL half, single, and double precision `math functions`_ all have separate
ULP requirements defined in the OpenCL specifications.
Single and Double precision errors are defined as separate tables in the main
OpenCL C specification under `Relative Errors As ULPs`_. The table for 32-bit
single precision is labelled **ULP values for single precision built-in math
functions**, and for 64-bit double labelled **ULP values for double precision
built-in math functions**.

The OpenCL extension specification defines the half precision requirements in
a section on `cl_khr_fp16 ULP error`_, there is no allowance for
`Embedded Profile`_ with 16-bit half or 64-bit double.

.. note::
  Precision is measured in ULP (Units in Last Place), defined as:

  ULP
    If :math:`x` is a real number that lies between two finite consecutive
    floating-point numbers :math:`a` and :math:`b`, without being equal to one of
    them, then :math:`ulp(x) = |b - a|`, otherwise :math:`ulp(x)` is the distance
    between the two non-equal finite floating-point numbers nearest :math:`x`.
    Moreover, :math:`ulp(NaN)` is :math:`NaN`.

.. _Relative Errors As ULPs:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#relative-error-as-ulps

.. _cl_khr_fp16 ULP error:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_Ext.html#cl_khr_fp16-relative-error-as-ulps

OpenCL Embedded Profile ULP
~~~~~~~~~~~~~~~~~~~~~~~~~~~

OpenCL `Embedded Profile`_ targets low-power devices unlikely to be used in
the scientific compute domain. Therefore it defines weaker precision
requirements than *Full* profile for 32-bit float, allowing devices to implement
faster maths builtins.

These ULP error requirements are also defined in a table under
`Relative Errors As ULPs`_, labelled **ULP values for the embedded profile**.

Capability Queries
------------------

A ComputeMux device reports the level of support provided for each individual
floating-point format relating to rounding, denormal numbers, and
availability of optimization operations. The following capabilities are
reported by the :doc:`/overview/runtime/computemux-runtime` for 16-bit, 32-bit,
and 64-bit floats using the ``mux_floating_point_capabilities_e`` bitfield.

``mux_floating_point_capabilities_full``
  Binary format conforms to the IEEE-754 specification.

``mux_floating_point_capabilities_fma``
  IEEE 754-2008 fused multiply-add is supported.

``mux_floating_point_capabilities_soft``
  Basic floating-point operations (such as addition, subtraction,
  multiplication) are implemented in software.

``mux_floating_point_capabilities_rte``
  Round To Nearest Even supported.

  .. note::
    Round To Nearest Even is the default rounding mode in kernel code.

``mux_floating_point_capabilities_rtz``
  Round to Zero supported.

``mux_floating_point_capabilities_rtp``
  Round to Positive Infinity supported.

``mux_floating_point_capabilities_rtn``
  Round to Negative Infinity supported.

``mux_floating_point_capabilities_inf_nan``
  INF and NaNs are supported. Support for signalling NaNs is not required.

``mux_floating_point_capabilities_denorm``
  Support for denormal (aka subnormal) floating-point numbers.

  .. note::
    The :ref:`overview/toolkit:Abacus` maths library in ComputeAorta supports
    denormal numbers.

ComputeAorta primarily uses these values to respond to user queries made in
high-level languages, but the capabilities are also used to determine whether
the device meets any criteria imposed by the high-level language.

Conformance Capabilities
~~~~~~~~~~~~~~~~~~~~~~~~

The requirements for OpenCL devices not of type `CL_DEVICE_TYPE_CUSTOM`_, for
which there are no requirements, are documented in the table below using the
equivalent OpenCL capability to those reported by ComputeMux. The table
shows that `Embedded Profile`_ devices have a reduced set of requirements for
single precision floating-point compared to the default *Full* Profile.

+-----------------------------------+-------------------------------------------------------+
| **Floating-Point Format**         | **Required Capabilities**                             |
+-----------------------------------+-------------------------------------------------------+
| `16-bit Half`_                    | * `CL_FP_ROUND_TO_ZERO`_ or `CL_FP_ROUND_TO_NEAREST`_ |
|                                   | * `CL_FP_INF_NAN`_                                    |
+-----------------------------------+-------------------------------------------------------+
| `32-bit Single Full Profile`_     | * `CL_FP_ROUND_TO_NEAREST`_                           |
|                                   | * `CL_FP_INF_NAN`_                                    |
+-----------------------------------+-------------------------------------------------------+
| `32-bit Single Embedded Profile`_ | * `CL_FP_ROUND_TO_ZERO`_ or `CL_FP_ROUND_TO_NEAREST`_ |
+-----------------------------------+-------------------------------------------------------+
| `64-bit Double`_                  | * `CL_FP_FMA`_                                        |
|                                   | * `CL_FP_ROUND_TO_NEAREST`_                           |
|                                   | * `CL_FP_INF_NAN`_                                    |
|                                   | * `CL_FP_DENORM`_                                     |
+-----------------------------------+-------------------------------------------------------+

.. _Embedded Profile:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#opencl-embedded-profile
.. _rounding modes:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#rounding-modes-1
.. _32-bit Single Full Profile:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#CL_DEVICE_SINGLE_FP_CONFIG
.. _32-bit Single Embedded Profile:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#embedded-profile-single-fp-config-requirements
.. _64-bit Double:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#CL_DEVICE_DOUBLE_FP_CONFIG
.. _16-bit Half:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_Ext.html#cl_khr_fp16-ieee754-compliance
.. _CL_FP_FMA:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#CL_FP_FMA
.. _CL_FP_DENORM:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#CL_FP_DENORM
.. _CL_FP_INF_NAN:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#CL_FP_INF_NAN
.. _CL_FP_ROUND_TO_NEAREST:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#CL_FP_ROUND_TO_NEAREST
.. _CL_FP_ROUND_TO_ZERO:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#CL_FP_ROUND_TO_ZERO
