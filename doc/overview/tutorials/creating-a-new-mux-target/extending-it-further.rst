Extending it Further
====================

Setting LLVM command-line compiler options
------------------------------------------

A common situation is passing command-line options to control the compiler's
code generation. This is left to the target as is it ultimately a
target-specific situation; some guidance is provided here on how this may be
achieved.

The snippet of code below can be used to parse command-line options from an
array of strings into the global in-process LLVM state. Note that this state is
truly *global* and not per ``LLVMContext``.

.. code:: cpp

  std::array<const char *, 3> cl_args = {
    "ComputeAortaCL",
    "--foo", "2",
  };

  llvm::cl::ParseCommandLineOptions(cl_args.size(), cl_args.data(), "");

In a debug build of ComputeMux, command-line options may be provided by the
user via the ``CA_LLVM_OPTIONS`` environment variable. These options are
automatically parsed in the ``BaseContext`` constructor.

The snippet of code above will *reset* all previously set options, including
``CA_LLVM_OPTIONS`` options.

Instead, targets may wish to use the following:

.. code:: cpp

  #if defined(NDEBUG)
    llvm::cl::ParseCommandLineOptions(cl_args.size(), cl_args.data(), "");
  #else
    llvm::cl::ParseCommandLineOptions(cl_args.size(), cl_args.data(), "",
                                      nullptr, "CA_LLVM_OPTIONS");
  #endif

This form will preserve ``CA_LLVM_OPTIONS`` in debug builds.

Parsing of command-line options could take place in several places.

If options are inherent to a target and not dictated by the specific compilation
process, then they should be parsed somewhere such as the initialization of
target's derived ``compiler::Info`` class. This would ensure that the options
are parsed and the LLVM state is set up once per target.

If options depend on the compilation process (such as on the compiler build
options) then they may need to be parsed before the ``llvm::TargetMachine`` is
created, or before the IR passes are run, or before the backend pass pipeline
is added via ``llvm::TargetMachine::addPassesToEmitFile``. This is ultimately
up to the target. One suggestion is parsing options in the target's derived
``compiler::Module::createPassMachinery`` as these are created before all of
the main compilation pipelines in ComputeMux.

.. warning::

  Targets using the ``lld`` linker should be aware that ``lld::elf::link`` (and
  ``compiler::utils::lldLinkToBinary`` by extension) resets the LLVM global
  command-line state except for ``CA_LLVM_OPTIONS`` (in a debug build).

  Targets may supply additional options prefixed with ``-mllvm`` to ensure they
  are reparsed by ``lld``. for example, ``--foo 1 --bar`` would be need to be
  passed to ``lld`` as ``-mllvm --foo=1 -mllvm --bar``. A helper function --
  ``compiler::utils::appendMLLVMOptions`` is provided to make this process
  easier.

  Targets may alternatively need to re-parse options after linking, or before
  starting the next compilation process, being careful on LLVM 14 and below for
  the `"may only occur one or more times"` error detailed above.
