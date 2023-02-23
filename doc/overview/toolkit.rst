Driver Development Kit
======================

ComputeAorta can be thought of as a Driver Development Kit (DDK). A suite of
software enabling developers to easily expose the performance of their hardware
though open standards.

.. todo::
  CA-3718: Things to help when implementing Mux, but not CL/VK, such
  as compiler passes in utils and Abacus.

  Document not in scope for end of October.

Abacus
------

Abacus is ComputeAorta's kernel library implementing the builtins required for
compute languages, including all the `math functions`_ required by OpenCL.
Abacus implements half, float, and double floating-point functions to the high
precision requirements needed for conformance. It provides specialized
implementations for both scalar and vector data types, allowing optimized
algorithms to be used for vector architectures.

Software conversions between the OpenCL :ref:`overview/compiler/ir:Types` are
also provided by Abacus, including the rounding and saturating variants defined
for `OpenCL explicit conversions`_.

.. tip::
  If a device contains hardware capable of performing a subset of builtin
  operations, then the :doc:`/overview/compiler/computemux-compiler` should use
  a more performant hardware specific implementation for the relvant maths
  builtins in place of the Abacus software implementation.

.. _OpenCL explicit conversions:
 https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#explicit-conversions

Requirements
~~~~~~~~~~~~

Abacus relies on the IEEE-754 format in its implementation, and on
the add (``+``), subtract (``-``), and multiply (``*``) operations being
correctly rounded. These are the minimum capabilities for a device to use the
software maths library. Abacus avoids using division (``/``) operations in its
algorithms, as it is typically expensive.

Hardware support for denormal numbers is **not** required, although a feature of
Abacus is that it does work with denormals. Abacus will avoid denormal numbers
in intermediate calculations if they are not available.

Vector hardware is not a requirement to support Abacus, the ComputeMux Compiler
implementation will be able to transform the Abacus vector algorithms as
appropriate for the hardware.

.. seealso::
  :doc:`/overview/hardware/floating-point-requirements` defines the expectations
  on a device when supporting floating-point types.

Configurable
~~~~~~~~~~~~

A mechanism for setting compile time flags exists in Abacus for the ComputeMux
target to report that
:ref:`overview/hardware/floating-point-requirements:optimization options`
have been set, or that the device implements a lower precision profile of the
high level heterogeneous language. These configurations allow Abacus to provide
more optimal algorithms for implementing the builtins to the ComputeMux target.

.. _math functions:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_C.html#math-functions
