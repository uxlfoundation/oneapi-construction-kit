muxc
====

``muxc`` is a compiler pass pipeline testing tool, similar to `opt` and `veczc`.
It takes in a pipeline of passes in textual form, creates a pipeline and runs
those passes on a ``LLVM IR`` module.

It can either work per target or use the default set of utility passes.
Target-specific execution requires the use of ``--device``/``--device-idx``.
Targets can support additional pass parsing and debug names as explained in
`Supporting muxc passes in targets`_.

The tool assumes that all targets provide a compiler interface which creates a
derived class from ``compiler::BaseModule`` when creating the ``Module`` from the
``Target`` interface.

The ``LLVM IR`` module may passed directly to the tool or may be compiled from
OpenCL C using ``compiler::BaseModule``'s APIs; this requires a target to work.

Executing muxc
--------------

Its most common usage requires the following arguments:

* ``<input_filename>``. Default is ``'-'`` (stdin).
* ``--passes <pipeline_text>`` - pass pipeline. Optional. Default is ``""``.
* ``-o <output_filename>``. Default is ``'-'`` (stdout).
* ``--device <device_name>`` or ``--device-idx <device_idx>`` - target-specific
  device to run on. Optional.

The input can be an LLVM IR module or an OpenCL C kernel (see below). The file
type is automatically worked out by the filename extension, but LLVM IR is
assumed. The input file type can be specifically controlled by passing ``-x
ir`` for IR or ``-x cl`` for OpenCL C.

The ``--passes <pipeline_text>`` option allows passes separated by a comma
(``,``). These passes include standard LLVM passes, such as ``instcombine``.
Parameters for such passes are included inside ``<>`` and generally have
default values. For example, ``link-builtins`` or ``link-builtins<early>``.

Additionally, several standard LLVM command line arguments for tools such
``opt`` can be used.

The ``--device`` option is only required if target-specific testing is needed,
either to set the LLVM target triple or to register pass information. This is
also required in order for the tool to accept OpenCL C inputs.

Examples of running on LLVM IR modules:

.. code::

   > # Run instcombine on a textual LLVM module.
   > muxc --passes instcombine test.ll
   > # Run instcombine,simplifycfg on a bitcode LLVM module,
   > # saving the result to a new textual file.
   > muxc --passes instcombine,simplifycfg test.bc -o output.ll
   > # Run instcombine,simplifycfg on a bitcode LLVM module,
   > # saving the result to a new bitcode file.
   > muxc --passes instcombine,simplifycfg test.bc --filetype obj -o output.bc

   > # Pipe a program from stdin and run no passes on it (pass-through)
   > echo 'define i32 @foo() { ret i32 0 }' | muxc

   > # Run a refsi-specific compiler pass
   > muxc --device "RefSi G1 RV64" --passes ir-to-builtins < test.ll

   > # Run a compiler pass with options on the first known device
   > muxc --device-idx 0 --passes link-builtins < test.ll
   > muxc --device-idx 0 --passes "link-builtins<early>" < test.ll

   > # Run instcombine, logging its debug output to stderr (requires a debug
   > # build of LLVM).
   > muxc --passes instcombine --debug test.ll

Examples of compiling OpenCL C (requires a known device):

.. code::

   > # Compile an OpenCL C kernel to IR, printing it to stdout.
   > muxc --device-idx 0 kernel.cl

   > # Compile an OpenCL C kernel to IR, providing compiler options
   > muxc --device-idx 0 -cl-options '-cl-fast-relaxed-math' kernel.cl

   > # Compile an OpenCL C kernel from stdin to IR
   > echo 'kernel void foo() { }' | muxc --device-idx 0 -x cl

   > # Compile an OpenCL C kernel to IR and run instcombine on it
   > muxc --device-idx 0 --passes instcombine kernel.cl


Other useful options include:

* ``--list-devices`` - list all known devices.
* ``--print-passes`` - print available passes that can be specified in ``--passes=foo``.

All of the passes available to ``opt`` are available as well as those shown
using ``--print-passes``.

A full list of options can be found by passing ``--help`` to ``muxc``.

Supporting muxc passes in targets
---------------------------------

``ComputeMux`` targets provide an interface to ``muxc`` through the
``PassMachinery`` class. Specifically, the interface needed is based off the
virtual function in ``BaseModule``, ``createPassMachinery()``. The default for
this in ``BaseModule`` creates a ``BaseModulePassMachinery``. This implements
the parse and debug name support for the majority of utility passes that the
oneAPI Construction Kit provides. This method should always be overridden even
if a ``BaseModulePassMachinery`` is used so that a ``TargetMachine``
appropriate to the target can be attached to the ``PassMachinery`` during
creation.

In order to extend for target specific passes, the virtual function
``createPassMachinery()`` should create a ``PassMachinery`` derived from
``BaseModulePassMachinery``. The ``PassMachinery`` created should have the
methods ``registerPassCallbacks`` and ``addClassToPassName`` overridden to
support parsing and debug names for the target passes. Note that these methods
should still call the ``BaseModulePassMachinery`` methods in order to ensure that
the util passes are available.
