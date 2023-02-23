Group Async Copies Extended Instruction Set - ``Codeplay.GroupAsyncCopies``
===========================================================================

Contributors
------------

* Kenneth Benzie, Codeplay Software Ltd.

Introduction
------------

.. danger::
   This specification is a work-in-progress and will likely be superceeded.

This is the specification of **Codeplay.GroupAsyncCopies** extended instruction
set. It provides instructions to represent the builtins provided by the
cl_khr_extended_async_copies_ OpenCL C extension.

The library is imported into a SPIR-V module in the following manner:

.. code-block:: none

   <ext-inst-id> = OpExtInstImport "NonSemantic.Codeplay.GroupAsyncCopies"

.. note::
   The ``NonSemantic.`` prefix to the ``OpExtInstImport`` instruction set
   string literal is a workaround used to bypass checks in ``spirv-as`` which
   reject unknown extended instruction set names. This is necessary until
   tooling has been updated to become aware of this new set of extended
   instructions.

The library can only be imported when the *Kernel* capability is specified.

.. _cl_khr_extended_async_copies:
   https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_Ext.html#cl_khr_extended_async_copies

Binary Form
-----------

GroupAsyncCopy2D2D
++++++++++++++++++

Perform an async copy of (*Num Elements Per Line* * *Num Lines*) elements of
size *Num Bytes Per Element* from (*Source* + (*Source Offset* * *Num Bytes Per
Element*)) to (*Destination* + (*Destination Offset* * *Num Bytes Per
Element*)). All pointer arithmetic is performed with implicit casting to
``char*`` by the implementation. Each line contains *Num Elements Per Line*
elements of size *Num Bytes Per Element*. After each line of transfer, *Source*
address is incremented by *Source Line Length* elements (i.e. *Source Total
Line Length* * *Num Bytes Per Element* bytes), *Destination* address is
incremented by *Destination Line Length* elements (i.e. *Destination Line
Length* * *Num Bytes Per Element* bytes), for the next line of transfer.

All *Source Offset*, *Destination Offset*, *Source Line Length* and
*Destination Line Length* values are expressed in elements.

Both *Source Line Length* and *Destination Line Length* describe the number of
elements between the beginning of the current line and the beginning of the
next line.

Returns an event object that can be used by :spirv:`OpGroupWaitEvents` to wait
for the async copy to finish. The *Event* operand can also be used to associate
the **GroupAsyncCopy2D2D** with a previous async copy allowing an event to be
shared by multiple async copies; otherwise *Event* should be
:spirv:`OpConstantNull` of :spirv:`OpTypeEvent`.

If *Event* operand is non-null, the event object supplied in *Event* operand
will be the *Result*.

This instruction does not perform any implicit synchronization of source data
such as using a barrier before performing the copy.

The behavior of **GroupAsyncCopy2D2D** is undefined if the *Source Line Length*
or *Destination Line Length* or *Source Offset* or *Destination Offset* values
cause the *Source* or *Destination* addresses to exceed the upper bounds of the
address space during the copy.

The behavior of **GroupAsyncCopy2D2D** is undefined if the *Source Line Length*
or *Destination Line Length* values are smaller than *Num Elements Per Line*,
i.e. overlapping of lines is undefined.

The async copy is performed by all work-items in a work-group and this built-in
function must therefore be encountered by all work-items in a work-group
executing the kernel with the same operands; otherwise the results are
undefined.

*Result Type* must be an object of :spirv:`OpTypeEvent`.

*Destination* must be :spirv:`OpTypePointer` with a storage class of either
``Workgroup`` or ``CrossWorkgroup``.

*Source* must be :spirv:`OpTypePointer` with a storage class of ``CrossWorkgroup`` when
*Destination* is ``Workgroup`` or ``Workgroup`` when *Destination* is
``CrossWorkgroup``.

*Destination Offset*, *Source Offset*, *Num Bytes Per Element*, *Num Elements
Per Line*, *Nun Lines*, *Source Line Length*, and *Destination Line Length*
must be a 32-bit `OpTypeInt` in when the addressing model is ``Physical32`` and
a 64-bit :spirv:`OpTypeInt` when the addressing modle is ``Physical64``.

*Event* must be an object of `OpTypeEvent`.

+---+--------+--------+-------------+-------------+--------+--------+----------+----------+-------+--------+-------------+-------+
| 1 | <id>   | Result | <id>        | <id>        | <id>   | <id>   | <id> Num | <id> Num | <id>  | <id>   | <id>        | <id>  |
|   | Result | <id>   | Destination | Destination | Source | Source | Bytes    | Elements | Num   | Source | Destination | Event |
|   | Type   |        |             | Offset      |        | Offset | Per      | Per      | Lines | Line   | Line        |       |
|   |        |        |             |             |        |        | Element  | Line     |       | Length | Length      |       |
+---+--------+--------+-------------+-------------+--------+--------+----------+----------+-------+--------+-------------+-------+

GroupAsyncCopy3D3D
++++++++++++++++++

Perform an async copy of (*Num Elements Per Line* * *Num Lines*) * *Num Planes*)
elements of size *Num Bytes Per Element* from (*Source* + (*Source Offset* *
*Num Bytes Per Element* to (*Destination* + (*Destination Offset* * *Num Bytes
Per Element*)), arranged in *Num Planes* planes. All pointer arithmetic is
performed with implicit casting to ``char*`` by the implementation. Each plane
contains *Num Lines* lines. Each line contains *Num Elements Per Line*
elements. After each line of transfer, *Source* address is incremented by
*Source Total Line Length* elements (i.e. *Source Total Line Length* * *Num
Bytes Per Element* bytes), *Destination* address is incremented by *Destination
Line Length* elements (i.e. *Destination Line Length* * *Num Bytes Per Element*
bytes), for the next line of transfer.

All *Source Offset*, *Destination Offset*, *Source Line Length*, *Destination
Line Length*, *Source Plane Area* and *Destination Plane Area* values are
expressed in elements.

Both *Source Line Length* and *Destination Line Length* describe the number of
elements between the beginning of the current line and the beginning of the
next line.

Both *Source Plane Area* and *Destination Plane Area* describe the number of
elements between the beginning of the current plane and the beginning of the
next plane.

Returns an event object that can be used by :spirv:`OpGroupWaitEvents` to wait
for the async copy to finish. The *Event* operand can also be used to associate
the **GroupAsyncCopy3D3D** with a previous async copy allowing an event to be
shared by multiple async copies; otherwise *Event* should be
:spirv:`OpConstantNull` of :spirv:`OpTypeEvent`.

If *Event* operand is non-null, the event object supplied in *Event* operand
will be the *Result*.

This instruction does not perform any implicit synchronization of source data such
as using a barrier before performing the copy.

The behavior of **GroupAsyncCopy3D3D** is undefined if the *Source Offset* or
*Destination Offset* values cause the *Source* or *Destination* addresses to
exceed the upper bounds of the address space during the copy.

The behavior of **GroupAsyncCopy3D3D** is undefined if the *Source Line Length*
or *Destination Line Length* values are smaller than *Num Elements Per Line*,
i.e. overlapping of lines is undefined.

The behavior of **GroupAsyncCopy3D3D** is undefined if *Source Plane Area* is
smaller than (*Num Lines* * *Source Line Length*), or *Destination Plane Area*
is smaller than (*Num Lines* * *Destination Line Length*), i.e. overlapping of
planes is undefined.

The async copy is performed by all work-items in a work-group and this built-in
function must therefore be encountered by all work-items in a work-group
executing the kernel with the same operands; otherwise the results are
undefined.

*Result Type* must be an object of :spirv:`OpTypeEvent`.

*Destination* must be :spirv:`OpTypePointer` with a storage class of either
``Workgroup`` or ``CrossWorkgroup``.

*Source* must be :spirv:`OpTypePointer` with a storage class of
``CrossWorkgroup`` when *Destination* is ``Workgroup`` or ``Workgroup`` when
*Destination* is ``CrossWorkgroup``.

*Destination Offset*, *Source Offset*, *Num Bytes Per Element*, *Num Elements
Per Line*, *Nun Lines*, *Num Planes*, *Source Line Length*, *Source Plane
Area*, *Destination Line Length*, and *Destination Plane Area* must be a 32-bit
:spirv:`OpTypeInt` in when the addressing model is ``Physical32`` and a
64-bit :spirv:`OpTypeInt` when the addressing model is ``Physical64``.

*Event* must be an object of :spirv:`OpTypeEvent`.

+---+--------+--------+-------------+-------------+--------+--------+----------+----------+-------+--------+--------+--------+-------------+-------------+-------+
| 2 | <id>   | Result | <id>        | <id>        | <id>   | <id>   | <id> Num | <id> Num | <id>  | <id>   | <id>   | <id>   | <id>        | <id>        | <id>  |
|   | Result | <id>   | Destination | Destination | Source | Source | Bytes    | Elements | Num   | Num    | Source | Source | Destination | Destination | Event |
|   | Type   |        |             | Offset      |        | Offset | Per      | Per      | Lines | Planes | Line   | Plane  | Line        | Plane       |       |
|   |        |        |             |             |        |        | Element  | Line     |       |        | Length | Area   | Length      | Area        |       |
+---+--------+--------+-------------+-------------+--------+--------+----------+----------+-------+--------+--------+--------+-------------+-------------+-------+

Revision History
----------------

+-----+------------+-------------------+----------------------------------+
| Rev | Date       | Author            | Changes                          |
+=====+============+===================+==================================+
| 1   | 2022/03/15 | Kenneth Benzie    | Initial revision                 |
+-----+------------+-------------------+----------------------------------+
