muxc
====

``muxc`` is a compiler pass pipeline testing tool, similar to `opt` and `veczc`.
It takes in a pipeline of passes in textual form, creates a pipeline and runs
those passes on the input ``LLVM IR`` or ``bitcode`` module.

It can either work per target or use the default set of utility passes. Per
target requires the use of ``--device``. Targets can support additional pass
parsing and debug names as explained in `Supporting muxc passes in targets`_.

The tool assumes that all targets provide a compiler interface which creates a
derived class from ``compiler::BaseModule`` when creating the ``Module`` from the
``Target`` interface.

Executing muxc
----------------

It has the following arguments:

* <input_filename>. Default is '-' (stdin).
* --passes <input_pipeline_text>. Default is "".
* -o `file` output IR or bitcode file. Default is '-' (stdout).
* -S Output as textual IR.
* --device=<device name>.
* --list-devices List all known devices.
* --print-passes Print available passes that can be specified in "--passes=foo"

Additionally standard LLVM command line arguments for tools such ``opt`` can be
used. ``--device`` is only need if per target testing is needed, either to set 
the LLVM target triple or to register pass information.

``--passes <input_pipeline_text>`` allows passes separated by a ``,``. These passes
include standard LLVM passes, such as ``instcombine``. Parameters for such passes
are included inside ``<>`` and generally have default values. For example,
``add-kernel-wrapper<packed>`` or ``add-kernel-wrapper<>``.

All of the passes available to `opt` are available as well as those shown using
`--print-passes`.

Supporting muxc passes in targets
---------------------------------

`mux` Compute targets provide an interface to ``muxc`` through the
``PassMachinery`` class. Specifically, the interface needed is based off the
virtual function in ``BaseModule``, ``createPassMachinery()``. The default for
this in ``BaseModule`` creates a ``BaseModulePassMachinery``. This implements
the parse and debug name support for the majority of utility passes that
the oneAPI Construction Kit provides. This method should always be overridden even
if a ``BaseModulePassMachinery`` is used so that a ``TargetMachine`` appropriate to
the target can be attached to the ``PassMachinery`` during creation.

In order to extend for target specific passes, the virtual function
``createPassMachinery()`` should create a ``PassMachinery`` derived from
``BaseModulePassMachinery``. The ``PassMachinery`` created should have the
methods ``registerPassCallbacks`` and ``addClassToPassName`` overridden to
support parsing and debug names for the target passes. Note that these methods
should still call the ``BaseModulePassMachinery`` methods in order to ensure that
the util passes are available.

