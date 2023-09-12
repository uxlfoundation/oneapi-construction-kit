ComputeMux Compiler Specification
=================================

   This is version 0.79.0 of the specification.

ComputeMux is Codeplay’s proprietary API for executing compute workloads across
heterogeneous devices. ComputeMux is an extremely lightweight,
bare-to-the-metal abstraction on modern hardware, and is used to power a suite
of open source standards. ComputeMux consists of a :doc:`Runtime Specification
</specifications/mux-runtime-spec>` and :doc:`Compiler Specification
</specifications/mux-compiler-spec>`. This document refers to the Compiler
component.

Throughout this specification, ComputeMux has been shortened to Mux for brevity,
but for the avoidance of all doubt, "Mux" refers to "ComputeMux".

Glossary
--------

The key statements **must**, **must not**, **shall**, **shall not**,
**should**, **should not**, **may**, in this document are to be
interpreted as described in `IETF RFC
2119 <http://www.ietf.org/rfc/rfc2119.txt>`__.

-  **must** or **shall** - means that the definition is an absolute
   requirement of the specification.
-  **must not** or **shall not** - means that the definition is an
   absolute prohibition of the specification.
-  **should** - means that the definition is highly likely to occur, but
   there are extraneous circumstances that could result in it not
   occurring.
-  **should not** - means that the definition is highly likely to not
   occur, but there are extraneous circumstances that could result in it
   occurring.
-  **may** - means that an item is optional.

Introduction
------------

This document describes the Mux compiler API, how it is used, and requirements
to follow on its usage. The Mux compiler API is a C++ API that provides the
compiler implementation used by a particular customer target (specified by a
``mux_device_info_t`` from the `Runtime Specification <#runtime>`__).

This specification describes a number of C++ classes with methods that a customer
compiler must implement. Some entry points are pure virtual methods and are
therefore mandatory, but others already have base implementations which must be
called if the function is overridden by a customer compiler. Note that
the oneAPI Construction Kit does not use exceptions, and therefore there are no
exception guarantee requirements.

Cargo
-----

Cargo is the oneAPI Construction Kit's STL like container library used by a
number of compiler API methods that conforms to stricter memory requirements
than the C++ STL e.g. constructors do not allocate memory and exceptions are
never thrown from any container.

Context
-------

The ``Context`` object serves as an opaque wrapper over the LLVM context
object. This object satisfies the C++ named requirement 'Lockable', and *must*
be locked when the compiler is interacting with a specific instance of LLVM.
``Context`` exposes the following interface:

.. code:: cpp

    namespace compiler {
    class Context {
    public:
      bool isValidSPIRV(cargo::array_view<const uint32_t> code);

      cargo::expected<spirv::SpecializableConstantsMap, std::string>
      getSpecializableConstants(cargo::array_view<const uint32_t> code);

      void lock();

      bool try_lock();

      void unlock();
    };
    }

The ``compiler::Context`` object is fully implemented by the Mux compiler
library, and customer targets are not required to implement it.

Info
----

An ``Info`` object is a description of a particular compiler implementation
that can be used to compile programs for a particular ``mux_device_info_t``.
``Info`` objects are expected to have a static ``get`` method that returns a
singleton instance containing information about the compiler capabilities and
metadata, similar to ``mux_device_info_t`` objects in the :doc:`Runtime
Specification </specifications/mux-runtime-spec>`.

.. code:: cpp

    namespace compiler {
    struct Info {
     public:
      virtual std::unique_ptr<compiler::Target> createTarget(
        compiler::Context *context,
        cargo::optional<mux_device_t> device,
        mux_allocator_info_t allocator_info) = 0;

      builtins::file::capabilities_bitfield getBuiltinCapabilities();

      mux_device_info_t device_info;
      bool supports_deferred_compilation;
      const char *compilation_options;
      bool vectorizable;
      bool dma_optimizable;
      bool scalable_vector_support;
    };
    }

-  ``device_info`` - The singleton instance of ``mux_device_info_t`` which this
   compiler targets.
-  ``supports_deferred_compilation`` - Is true if this compiler supports
   deferred compilation by implementing ``compiler::Module::createKernel`` and
   the ``compiler::Kernel`` class, otherwise false.
-  ``compilation_options`` - A null-terminated C string of
   semicolon-separated compilation options specific to this compiler.
-  ``vectorizable`` - Is true if the device supports vectorization
   otherwise false.
-  ``dma_optimizable`` - Is true if the device supports DMA
   optimizations otherwise false.
-  ``scalable_vector_support`` - Is true if the device supports scalable vectors
   otherwise false.

.. rubric:: Valid Usage

-  ``compilation_options`` **must** conform to the `compilation options
   syntax <#compilation-options-syntax>`__, defined below.

Info::createTarget
~~~~~~~~~~~~~~~~~~

``Info::createTarget`` creates a new instance of a subclass of
``compiler::Target``.

.. code:: cpp

    std::unique_ptr<compiler::Target> createTarget(
        compiler::Context *context);

-  ``context`` - an instance of ``compiler::Context``.
-  ``callback`` - an optional callback used to provide a message back to the user.

.. rubric:: Return Value

-  If there was an allocation failure, ``nullptr`` **must** be returned.
-  If ``context`` is ``nullptr``, ``nullptr`` **must** be returned.
-  Otherwise an instance of ``compiler::Target`` **should** be returned.

Info::getBuiltinCapabilities
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``Info::getBuiltinCapabilities`` retrieves a bitfield describing the builtin
capabilities of the target device, based on ``Info::device_info``.

.. code:: cpp

    builtins::file::capabilities_bitfield getBuiltinCapabilities();

.. rubric:: Return Value

- A bitfield describing the builtin capabilities should be returned.

Compilation Options Syntax
~~~~~~~~~~~~~~~~~~~~~~~~~~

``compilation_options`` **must** follow the syntax of a comma separated
tuple of (name, [1|0], help) with the following rules:

1. Argument name in the first tuple entry **must** start with a double
   hyphen and not contain whitespace characters.
2. The second element **must** be a ‘1’ or a ‘0’ denoting if a value
   needs to be provided for the option.
3. The final tuple entry is a help message to be displayed by compiler
   tools. All help whitespace **must** only be `` `` characters; other
   whitespace characters (``\t``, ``\n``, etc.) **must not** be used.
4. If multiple options are reported then each tuple **must** be
   separated by a semi-colon.

Example of valid options reported by a device, including both an option
which requires a value and an option which is just a build flag.

.. code:: c

   info_ptr->compilation_options =
     "--dummy-device-option,1,takes an integer value;"
     "--dummy-device-flag,0,enables device optimization";

Enumerating ``compiler::Info``'s
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Compiler targets are required to provide a free-standing function that lists one
or more static instances of the ``compiler::Info`` object for each compiler
configuration that this target supports. The name of this function does not
matter, but it is named ``getCompilers`` in this example.

.. code:: cpp

    void getCompilers(compiler::AddCompilerFn add_compiler);

-  ``add_compiler`` - an object that overloads ``operator()`` which informs
   the oneAPI Construction Kit about a static instance of ``compiler:Info``.
   Used to register a specific compiler configuration.

One way of implementing this requirement is to add a static function to the
``compiler::Info`` object:

.. code:: cpp

    struct MyCompilerInfo : public compiler::Info {
      // ...
      static void get(compiler::AddCompilerFn add_compiler) {
        static MyCompilerInfo info;
        add_compiler(&info);
      }
    };

Then, provide the fully qualified name to this function in CMake:

.. code:: cmake

   add_mux_compiler_target(MyCompiler
     COMPILER_INFO MyCompilerInfo::get
     HEADER_DIR my_compiler/info.h)

Target
------

A ``Target`` object is an instance of the compiler which "targets" a particular
Mux device. It is used as the entry point into customer code from the compiler
library.

.. code:: cpp

    namespace compiler {
    class BaseTarget {
     public:
      BaseTarget(
          const compiler::Info *compiler_info,
          compiler::Context *context,
          compiler::NotifyCallbackFn callback);

      virtual Result initWithBuiltins(std::unique_ptr<llvm::Module> builtins) = 0;

      virtual std::unique_ptr<compiler::Module> createModule(
          uint32_t &num_errors,
          std::string &log) = 0;

      const compiler::Info *getCompilerInfo() const;
    };
    }

BaseTarget Constructor
~~~~~~~~~~~~~~~~~~~~~~

A ``Target`` object which extends ``BaseTarget`` **must** have a constructor
which calls ``BaseTarget``'s constructor with the following arguments

.. code:: cpp

    BaseTarget(
        const compiler::Info *compiler_info,
        compiler::Context *context,
        compiler::NotifyCallbackFn callback);

-  ``compiler_info`` - the compiler info used to create this object.
-  ``context`` - an instance of ``compiler::Context``.
-  ``callback`` - an optional callback used to provide a message back to the user.

BaseTarget::initWithBuiltins
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``BaseTarget::initWithBuiltins`` initializes the given target object after
loading builtins.

.. code:: cpp

    compiler::Result initWithBuiltins(
        std::unique_ptr<llvm::Module> builtins);

-  ``builtins`` - an LLVM module containing the embedded builtins provided by
   the oneAPI Construction Kit.

.. rubric:: Return Value

-  If there was an allocation failure, ``compiler::Result::OUT_OF_MEMORY``
   **must** be returned.
-  Otherwise ``compiler::Result::SUCCESS`` **should** be returned.

BaseTarget::createModule
~~~~~~~~~~~~~~~~~~~~~~~~

``BaseTarget::createModule`` creates a new instance of a subclass of
``compiler::BaseModule`` that supports this target.

.. code:: cpp

    std::unique_ptr<compiler::Module> createModule(
        uint32_t &num_errors,
        std::string &log);

-  ``num_errors`` - a reference to an integer that will contain the number of
   errors reported by the Module object during compilation.
-  ``log`` - a reference to a ``std::string`` that will contain errors reported
   by the Module object during compilation.

.. rubric:: Return Value

-  If there was an allocation failure, ``nullptr`` **must** be returned.
-  Otherwise an instance of ``compiler::Module`` **should** be returned.

Module
------

A ``Module`` object is the top level container for a device program compiled
from one of the supported source types. A Module **may** contain multiple entry
points and **may** have one or more named kernels unless it is a library module.

``Module`` is used to drive the compilation process, starting with the OpenCL C
or SPIR-V front-ends, optionally linking against other Modules, then applying
further optimizations before passing it to the back-end.

``BaseModule`` implements all of the front-end functionality, and it is left to
the Mux target implementation to implement the remaining pure virtual methods
that handle the back-end and code generation.

.. code:: cpp

    namespace compiler {
    class BaseModule {
     public:
      BaseModule(compiler::BaseTarget &target,
                 compiler::ContextImpl &context,
                 uint32_t &num_errors,
                 std::string &log);

      virtual Result createBinary(cargo::array_view<std::uint8_t> &buffer) = 0;

      virtual std::unique_ptr<compiler::utils::PassMachinery> createPassMachinery();

     protected:
      virtual Kernel *createKernel(const std::string &name) = 0;

     public:
      virtual void clear();

      virtual cargo::expected<spirv::ModuleInfo, Result> compileSPIRV(
          cargo::array_view<const std::uint32_t> buffer,
          const spirv::DeviceInfo &spirv_device_info,
          cargo::optional<const spirv::SpecializationInfo &> spirv_spec_info);

      virtual Result compileOpenCLC(
          cargo::string_view device_profile,
          cargo::string_view source,
          cargo::array_view<compiler::InputHeader> input_headers);

      virtual Result link(cargo::array_view<Module *> input_modules);

      virtual Result finalize(
          ProgramInfo *kernel_info,
          std::vector<builtins::printf::descriptor> &printf_calls);

      virtual Kernel *getKernel(const std::string &name);

      virtual std::size_t size();

      virtual std::size_t serialize(std::uint8_t *output_buffer);

      virtual bool deserialize(cargo::array_view<const std::uint8_t> buffer);

      virtual std::unique_ptr<compiler::utils::PassMachinery> createPassMachinery();

      virtual void initializePassMachineryForFrontend(
          compiler::utils::PassMachinery &,
          const clang::CodeGenOptions &) const;

      virtual void initializePassMachineryForFinalize(
          compiler::utils::PassMachinery &) const;

     protected:
      // Utility functions.
      virtual llvm::ModulePassManager getLateTargetPasses(
          compiler::utils::PassMachinery &) = 0;

      virtual Kernel *createKernel(const std::string &name) = 0;

      void addDiagnostic(cargo::string_view message);

      void addBuildError(cargo::string_view message);

      // Member variables.

      std::unique_ptr<llvm::Module> finalized_llvm_module;

      compiler::BaseContext &context;

      compiler::BaseTarget &target;

      compiler::Options options;

     private:
      std::unique_ptr<llvm::Module> llvm_module;
    };
    }

BaseModule Constructor
~~~~~~~~~~~~~~~~~~~~~~

A ``Module`` object which extends ``BaseModule`` **must** have a constructor
which calls ``BaseModule``'s constructor with the following arguments:

.. code:: cpp

    BaseModule(
        compiler::BaseTarget &target,
        compiler::ContextImpl &context,
        uint32_t &num_errors,
        std::string &log);

-  ``target`` - the ``compiler::Target`` object used to create this module.
-  ``context`` - an instance of ``compiler::Context``.
-  ``num_errors`` - a reference to an integer that will contain the number of
   errors reported by the Module object during compilation.
-  ``log`` - a reference to a ``std::string`` that will contain errors reported
   by the Module object during compilation.

BaseModule::finalize
~~~~~~~~~~~~~~~~~~~~

``BaseModule::finalize`` runs IR passes on the ``llvm_module`` which prepare it
for binary creation.

The passes run by the default implementation are a mixture of LLVM middle-end
optimizations and ComputeMux-specific passes that lower the incoming
``llvm_module`` from a higher-level form dependent on the original kernel
source-language (e.g., being produced by ``BaseModule::compileOpenCLC`` or
``BaseModule::compileSPIRV``) into a canonical "ComputeMux" form.

.. note::
  Note that most of the lower-level target-specific passes are left to
   ``BaseModule::getLateTargetPasses`` which **must** be implemented.

Targets may override this method to customize the pipeline.

BaseModule::getLateTargetPasses
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``BaseModule::getLateTargetPasses`` is an internal method called at the end of
``BaseModule::finalize``, and is reponsible for adding any final
target-specific IR passes to the pipeline, in preparation for the creation of
the final binary in ``BaseModule::createBinary``. Note that no
``BaseModule::finalize`` passes have actually been run by the time at which
this method is called, neither is the ``llvm::Module`` that the passes will be
run on exposed.

This method receives the same ``PassMachinery`` used throughout the
``BaseModule::finalize`` pipeline, that has been initialized with
``BaseModule::initializePassMachineryForFinalize``. Targets may therefore rely
on any analyses they've previously registered.

BaseModule::createPassMachinery
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``PassMachinery`` class manages the lifetime and initialization of all
components required to set up a new-style LLVM pass manager. It includes
various methods for registering debug information and parsing pipeline text
(for ``PassBuilder.parsePassPipeline``) and initalizing of state. The default
implementation will cover common passes, but if a user wants to register their
own for debug and parse they can create their own by deriving from
``BasePassMachinery``. 

The ``PassMachinery`` class takes an ``llvm TargetMachine`` pointer in the
constructor. By default this can only be known in the derived class, and so to
support the ``TargetMachine`` being known throughout the compilation pipeline, it
is advised to override the ``BaseModule::createPassMachinery``, even if only to
create the ``BaseModulePassMachinery`` with a known ``TargetMachine``. A derived
version of ``PassMachinery`` is also advised to support parsing and debugging of
target specific passes. This should generally be derived from
``BaseModulePassMachinery`` and the various ``register*`` methods of
``BaseModulePassMachinery`` called from the derived class.

BaseModule::initializePassMachineryForFrontend
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``BaseModule::initializePassMachineryForFrontend`` sets up a ``PassMachinery``
for use in the pipelines run by ``BaseModule::compileOpenCLC`` and
``BaseModule::compileSPIRV``. A default implementation is provided, though
targets may override this method to register custom analyses or tune the
pipeline.

BaseModule::initializePassMachineryForFinalize
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``BaseModule::initializePassMachineryForFinalize`` sets up a ``PassMachinery``
for use in the pipeline run by ``BaseModule::finalize`` (and by extension
``BaseModule::getLateTargetPasses``). A default implementation is provided,
though targets may override this method to register
custom analyses or tune the pipeline.

BaseModule::createBinary
~~~~~~~~~~~~~~~~~~~~~~~~

``BaseModule::createBinary`` creates a compiled binary which can be loaded by
the corresponding Mux implementation using ``muxCreateExecutable``.

.. code:: cpp

    compiler::Result createBinary(cargo::array_view<std::uint8_t> &buffer);

-  ``buffer`` - an array view over the binary buffer. This array view is valid
   until the next call to ``createBinary``.

.. rubric:: Return Value

-  If there was an allocation failure, ``compiler::Result::OUT_OF_MEMORY``
   **must** be returned.
-  Otherwise ``compiler::Result::SUCCESS`` **should** be returned.

BaseModule::createKernel
~~~~~~~~~~~~~~~~~~~~~~~~

``BaseModule::createKernel`` creates a deferred kernel, an object which
represents a specific kernel function within the Module which can have its
compilation deferred. Note that this function should not create a new kernel
function in the module, but instead creates a new ``compiler::Kernel`` object
that represents an existing kernel in the module.

This method **must** return ``nullptr`` if the ``compiler::Module`` does not
support deferred compilation of kernels and
``compiler::Info::supports_deferred_compilation`` is ``false``.

``BaseModule::getKernel`` will either look up ``compiler::Kernel`` objects by
kernel name, or call ``BaseModule::createKernel`` to create ``compiler::Kernel``
objects lazily.

.. code:: cpp

    compiler::Kernel *createKernel(
        const std::string &name);

-  ``name`` - the name of the kernel function to select from the module.

.. rubric:: Return Value

-  If there was an allocation failure, ``nullptr`` **must** be returned.
-  If this module does not support deferred compilation, ``nullptr`` **must** be
   returned.
-  Otherwise an instance of ``compiler::Kernel`` **should** be returned.

Kernel
------

A ``Kernel`` object represents a single kernel function inside a Module whose
compilation into a ``mux_kernel_t`` can be deferred at any point up to the point
we enqueue the kernel into a command buffer. The ``Kernel`` class is not
required to be implemented if the compiler implementation does not support
deferred compilation.

``Kernel`` **may** be used to perform further optimizations to specific kernels
once additional information is provided, such as local or global work-group
sizes, and/or descriptors.

.. code:: cpp

    namespace compiler {
    class BaseKernel {
     public:
      BaseKernel(size_t preferred_local_size_x,
                 size_t preferred_local_size_y,
                 size_t preferred_local_size_z,
                 size_t local_memory_size);

      virtual Result precacheLocalSize(size_t local_size_x,
                                       size_t local_size_y,
                                       size_t local_size_z) = 0;

      virtual cargo::expected<uint32_t, Result> getDynamicWorkWidth(
          size_t local_size_x,
          size_t local_size_y,
          size_t local_size_z) = 0;

      virtual cargo::expected<cargo::dynamic_array<uint8_t>, Result> createSpecializedKernel(
          const mux_ndrange_options_t &specialization_options) = 0;

      virtual cargo::expected<uint32_t, Result> getSubGroupSize() = 0;

      virtual cargo::expected<uint32_t, Result> querySubGroupSizeForLocalSize(
          size_t local_size_x, size_t local_size_y, size_t local_size_z) = 0;

      virtual cargo::expected<std::array<size_t, 3>, Result>
      queryLocalSizeForSubGroupCount(size_t sub_group_count) = 0;

      virtual cargo::expected<size_t, Result> queryMaxSubGroupCount() = 0;
    };
    }

Constructor
~~~~~~~~~~~

A ``Kernel`` object which extends ``BaseKernel`` **must** have a constructor
which calls ``BaseKernel``'s constructor with the following arguments:

.. code:: cpp

    BaseKernel(
        size_t preferred_local_size_x,
        size_t preferred_local_size_y,
        size_t preferred_local_size_z,
        size_t local_memory_size);

-  ``preferred_local_size_x`` - the preferred local size in the x dimension for
   this kernel object.
-  ``preferred_local_size_y`` - the preferred local size in the y dimension for
   this kernel object.
-  ``preferred_local_size_z`` - the preferred local size in the z dimension for
   this kernel object.
-  ``local_memory_size`` - the amount of local memory used by this kernel
   object.

BaseKernel::precacheLocalSize
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``BaseKernel::precacheLocalSize`` signals to the compiler to *optionally*
pre-cache a specific local work-group size configuration that **may** be required
later by ``BaseKernel::createSpecializedKernel``.

.. code:: cpp

    compiler::Result precacheLocalSize(
        size_t local_size_x,
        size_t local_size_y,
        size_t local_size_z);

-  ``local_size_x`` - the size of the *x* dimension of the local work-group.
-  ``local_size_y`` - the size of the *y* dimension of the local work-group.
-  ``local_size_z`` - the size of the *z* dimension of the local work-group.

.. rubric:: Return Value

-  If there was an allocation failure, ``compiler::Result::OUT_OF_MEMORY``
   **must** be returned.
-  If ``local_size_x`` is 0, ``compiler::Result::INVALID_VALUE`` **must** be returned.
-  If ``local_size_y`` is 0, ``compiler::Result::INVALID_VALUE`` **must** be returned.
-  If ``local_size_z`` is 0, ``compiler::Result::INVALID_VALUE`` **must** be returned.
-  Otherwise ``compiler::Result::SUCCESS`` **should** be returned.

BaseKernel::getDynamicWorkWidth
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``BaseKernel::getDynamicWorkWidth`` obtains the dynamic work width of this
kernel for a given local work-group size.

The work width indicates the number of work-items in a work-group that will
execute together. Note that the work width may be less than the size of the
work-group but never greater than, and may be 1.

Commonly the work width will relate to the hardware vector/wave-front/warp
width (likely the device's ``max_work_width``), but may be lowered if a
particular kernel cannot fully exploit the hardware. The work width may be less
than or greater than the hardware width, depending on factors such as what data
types are used in the kernel.

.. code:: cpp

    cargo::expected<uint32_t, compiler::Result> getDynamicWorkWidth(
        size_t local_size_x,
        size_t local_size_y,
        size_t local_size_z);

-  ``local_size_x`` - the size of the *x* dimension of the local work-group.
-  ``local_size_y`` - the size of the *y* dimension of the local work-group.
-  ``local_size_z`` - the size of the *z* dimension of the local work-group.

.. rubric:: Return Value

-  If there was an allocation failure,
   ``cargo::make_unexpected(compiler::Result::OUT_OF_MEMORY)`` **must** be
   returned.
-  If ``local_size_x`` is 0, ``compiler::Result::INVALID_VALUE`` **must** be
   returned.
-  If ``local_size_y`` is 0, ``compiler::Result::INVALID_VALUE`` **must** be
   returned.
-  If ``local_size_z`` is 0, ``compiler::Result::INVALID_VALUE`` **must** be
   returned.
-  Otherwise, a work width **should** be returned. The work width **must** be
   greater than 0.

BaseKernel::createSpecializedKernel
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``BaseKernel::createSpecializedKernel`` creates a compiled binary containing (at least)
the kernel represented by this ``compiler::Kernel`` object, which **may** have
been cloned and optimized further from the original module given all the
information required to execute. This binary should be loadable by the
corresponding Mux implementation using ``muxCreateExecutable``

Parameter information consists of descriptions of each parameter passed to the
kernel function. Execution information consists of information on the number of
work-groups to execute, and a work-group offset.

The ``compiler::Kernel`` object used to create this binary is guaranteed to
be destroyed **after** the ``mux_executable_t`` created from this binary is
destroyed.

.. code:: cpp

    cargo::expected<cargo::dynamic_array<uint8_t>, Result> createSpecializedKernel(
        const mux_ndrange_options_t &options);

-  ``options`` - the execution options that will be used when the
   kernel is executed by ``muxCommandNDRange``.

.. rubric:: Return Value

-  If there was an allocation failure,
   ``cargo::make_unexpected(compiler::Result::OUT_OF_MEMORY)`` **must** be
   returned.
-  If ``options.descriptors`` is not NULL and ``descriptors_length`` is 0,
   ``cargo::make_unexpected(compiler::Result::INVALID_VALUE)`` **must** be
   returned.
-  If ``options.descriptors`` is NULL and ``descriptors_length`` is not 0,
   ``cargo::make_unexpected(compiler::Result::INVALID_VALUE)`` **must** be
   returned.
-  If any element in ``options.local_size`` is 0,
   ``cargo::make_unexpected(compiler::Result::INVALID_VALUE)`` **must** be
   returned.
-  If ``options.global_offset`` is NULL,
   ``cargo::make_unexpected(compiler::Result::INVALID_VALUE)`` **must** be
   returned.
-  If ``options.global_size`` is NULL,
   ``cargo::make_unexpected(compiler::Result::INVALID_VALUE)`` **must** be
   returned.
-  If ``options.length`` is 0 or greater than 3,
   ``cargo::make_unexpected(compiler::Result::INVALID_VALUE)`` **must** be
   returned.
-  If ``options.descriptors`` contains an element where the ``type`` data member
   is ``mux_descriptor_info_type_custom_buffer`` and
   ``device->info->custom_buffer_capabilities`` is ``0``,
   ``cargo::make_unexpected(compiler::Result::INVALID_VALUE)`` **must** be
   returned.
-  If there was a failure during any code generation,
   ``cargo::make_unexpected(compiler::Result::FINALIZE_PROGRAM_FAILURE)``
   **must** be returned.
-  Otherwise an instance of ``cargo::dynamic_array<uint8_t>`` containing a valid
   binary **should** be returned.

BaseKernel::querySubGroupSizeForLocalSize
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``BaseKernel::querySubGroupSizeForLocalSize`` calculates the maximum sub-group
size that would result from enqueing the kernel with the given local size.
Enqueuing the kernel with the specified local size **shall** result in at least
one sub-group of the size returned in ``out_sub_group_size`` and **may**
additionally result in exactly one sub-group of size less than that returned
when the local size is not evenly divisible by the sub-group size.

.. code:: cpp

    virtual cargo::expected<uint32_t, Result> querySubGroupSizeForLocalSize(
        size_t local_size_x, size_t local_size_y, size_t local_size_z);

-  ``local_size_x`` - the size of the *x* dimension of the local work-group.
-  ``local_size_y`` - the size of the *y* dimension of the local work-group.
-  ``local_size_z`` - the size of the *z* dimension of the local work-group.

.. rubric:: Return Value

-  If there was an allocation failure, ``compiler::Result::OUT_OF_MEMORY``
   **must** be returned.
-  If any of ``local_size_x``, ``local_size_y`` or ``local_size_z`` are zero,
   ``compiler::Result::INVALID_VALUE`` **must** be returned.
-  If the device targeted by this kernel does not support sub-groups,
   ``compiler::Result::FEATURE_UNSUPPORTED`` **must** be returned.  ``
-  Otherwise, a sub-group size **should** be returned. The sub-group size
   **must** be greater than 0.

BaseKernel::queryLocalSizeForSubGroupCount
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``BaseKernel::queryLocalSizeForSubGroupCount`` calculates the local size that
when enqueued with the kernel would result in the specified number of
sub-groups.

.. code:: cpp

    virtual cargo::expected<std::array<size_t, 3>, Result>
    queryLocalSizeForSubGroupCount(size_t sub_group_count);

-  ``sub_group_count`` - the requested number of sub-groups.

.. rubric:: Return Value

-  If there was an allocation failure, ``compiler::Result::OUT_OF_MEMORY``
   **must** be returned.
-  If the device targeted by this kernel does not support sub-groups,
   ``compiler::Result::FEATURE_UNSUPPORTED`` **must** be returned.
-  Otherwise, a local size **should** be returned. The local size **must** be 1
   dimensional, that is, at least two of the elements in the array must be 1.
   The local size **must** be evenly divisible by the sub-group size in the
   kernel. If no local size would result in the requested number of sub-groups
   this function may return a local size of zero.

BaseKernel::queryMaxSubGroupCount
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``BaseKernel::queryMaxSubGroupCount`` calculates the maximum number of
sub-groups that can be supported by the kernel for any local size.

.. code:: cpp

    virtual cargo::expected<size_t, Result> queryMaxSubGroupCount();

.. rubric:: Return Value

-  If there was an allocation failure, ``compiler::Result::OUT_OF_MEMORY``
   **must** be returned.
-  If the device targeted by this kernel does not support sub-groups,
   ``compiler::Result::FEATURE_UNSUPPORTED`` **must** be returned.  ``
-  Otherwise, a sub-group count **should** be returned. The sub-group count
   **must** be greater than zero.


LLVM intermediate representation
--------------------------------

Mangling
--------

Mangling is used by the vectorizer to declare, define and use internal
overloaded builtin functions. In general, the mangling scheme follows
`Appendix A of the SPIR 1.2 specification <https://www.khronos.org/registry/SPIR/specs/spir_spec-1.2.pdf>`_\ ,
itself an extension of the Itanium C++ mangling scheme.

Vector Types
~~~~~~~~~~~~

The Itanium specification under-specifies vector types in general, so vendors
are left to establish their own system. In the vectorizer, fixed-length vector
types follow the convention that LLVM, GCC, ICC and others use. The first
component is ``Dv`` followed by the number of elements in the vector, followed by
an underscore (\ ``_``\ ) and then the mangled element type:

.. code-block::

   <2 x i32> -> Dv2_i
   <32 x double> -> Dv32_d

Scalable-vector IR types do not have an established convention. Certain vendors
such as ARM SVE2 provide scalable vector types at the C/C++ language level, but
those are mangled in a vendor-specific way.

The vectorizer chooses its own mangling scheme using the Itanium
vendor-extended type syntax, which is ``u``\ , followed by the length of the
mangled type, then the mangled type itself.

Scalable-vectors are first mangled with ``nx`` to indicate the scalable
component. The next part is an integer describing the known multiple of the
scalable component. Lastly, the element type is mangled according to the
established vectorizer mangling scheme (i.e. Itanium).

Example:

.. code-block::

   <vscale x 1 x i32>               -> u5nxv1j
   <vscale x 2 x float>             -> u5nxv2f
   <vscale x 16 x double>            -> u6nxv16d
   <vscale x 4 x i32 addrspace(1)*> -> u11nxv4PU3AS1j

   define void @__vecz_b_interleaved_storeV_Dv16_dPU3AS1d(<16 x double> %0, double addrspace(1)* %1, i64 %2) {
   define void @__vecz_b_interleaved_storeV_u6nxv16dPU3AS1d(<vscale x 16 x double> %0, double addrspace(1)* %1, i64 %2) {

Builtins
--------

The ComputeMux specification defines the following builtin functions. Any of
these functions **may** be declared and/or called in the LLVM intermediate
representation stored in ``compiler::BaseModule::finalized_llvm_module``.

A Mux implementation **shall** provide definitions for these builtin functions.

.. note::
   In the list of functions below:

   * ``size_t`` represents either ``i32`` or ``i64``, depending on the pointer
     size in bytes of the target.
   * ``__mux_dma_event_t`` represents an event object and may be defined as
     *any* type chosen by the Mux implementation, as long as it is consistently
     used across the module at any given time. For example, it may be a
     structure type, an a target extension type, an integer type, a pointer
     type, etc. This type **may** change throughout the compilation process.

* ``i1 __mux_isftz()`` - Returns whether the device flushes
  floating-point values to 0.
* ``i1 __mux_usefast()`` - Returns whether we should use faster, but
  less accurate, algorithms for maths builtins used in the LLVM module.
* ``i1 __mux_isembeddedprofile()`` - Returns whether the device
  implements OpenCL 1.2 Embedded Profile.
* ``size_t __mux_get_global_size(i32 %i)`` - Returns the number of global
  invocations for the ``%i``'th dimension.
* ``size_t __mux_get_global_id(i32 %i)`` - Returns the unique global
  invocation identifier for the ``%i``'th dimension.
* ``size_t __mux_get_global_offset(i32 %i)`` - Returns the global offset (in
  invocations) for the ``%i``'th dimension.
* ``size_t __mux_get_local_size(i32 %i)`` - Returns the number of local
  invocations within a work-group for the ``%i``'th dimension.
* ``size_t __mux_get_local_id(i32 %i)`` - Returns the unique local invocation
  identifier for the ``%i``'th dimension.
* ``i32 __mux_get_sub_group_id()`` - Returns the sub-group ID.
* ``size_t __mux_get_num_groups(i32 %i)`` - Returns the number of work-groups
  for the ``%i``'th dimension.
* ``i32 __mux_get_num_sub_groups()`` - Returns the number of sub-groups for
  the current work-group.
* ``i32 __mux_get_max_sub_group_size()`` - Returns the maximum sub-group size
  in the current kernel.
* ``i32 __mux_get_sub_group_size()`` - Returns the number of invocations in the
  sub-group.
* ``i32 __mux_get_sub_group_local_id()`` - Returns the unique invocation ID
  within the current sub-group.
* ``size_t __mux_get_group_id(i32 %i)`` - Returns the unique work-group
  identifier for the ``%i``'th dimension.
* ``i32 __mux_get_work_dim()`` - Returns the number of dimensions in
  use.
* ``__mux_dma_event_t __mux_dma_read_1D(ptr address_space(3) %dst,``
  ``ptr address_space(1) %src, size_t %width, __mux_dma_event_t %event)`` - DMA
  1D read from ``%src`` to ``%dst`` of ``%width`` bytes. May use ``%event``
  from previous DMA call. Returns event used.
* ``__mux_dma_event_t __mux_dma_read_2D(ptr address_space(3) %dst,``
  ``ptr address_space(1) %src, size_t %width, size_t %dst_stride,``
  ``size_t %src_stride, size_t %height __mux_dma_event_t %event)`` - DMA 2D
  read from ``%src`` to ``%dst`` of ``%width`` bytes and ``%height`` rows, with
  ``%dst_stride`` bytes between dst rows and ``%src_stride`` bytes between src
  rows. May use ``%event`` from previous DMA call. Returns event used.
* ``__mux_dma_event_t __mux_dma_read_3D(ptr address_space(3) %dst,``
  ``ptr address_space(1) %src, size_t %width, size_t %dst_line_stride,``
  ``size_t %src_line_stride, size_t %height, size_t %dst_plane_stride,``
  ``size_t %src_plane_stride, size_t %depth, __mux_dma_event_t %event)`` - DMA
  3D read from ``%src`` to ``%dst`` of ``%width`` bytes, ``%height`` rows, and
  ``%depth`` planes, with ``%dst_line_stride`` bytes between dst rows,
  ``%src_line_stride`` bytes between src rows, ``%dst_plane_stride`` bytes
  between dst planes, and ``%src_plane_stride`` between src planes. May use
  ``%event`` from previous DMA call. Returns event used.
* ``__mux_dma_event_t __mux_dma_write_1D(ptr address_space(1) ptr %dst,``
  ``ptr address_space(3) %src, size_t %width, __mux_dma_event_t %event)`` - DMA
  1D write from ``%src`` to ``%dst`` of ``%width`` bytes. May use ``%event``
  from previous DMA call. Returns event used.
* ``__mux_dma_event_t __mux_dma_write_2D(ptr address_space(1) %dst,``
  ``ptr address_space(1) %src, size_t %width, size_t %dst_stride,``
  ``size_t %src_stride, size_t %height __mux_dma_event_t %event)`` - DMA 2D
  write from ``%src`` to ``%dst`` of ``%width`` bytes and ``%height`` rows,
  with ``%dst_stride`` bytes between dst rows and ``%src_stride`` bytes between
  src rows. May use ``%event`` from previous DMA call. Returns event used.
* ``__mux_dma_event_t __mux_dma_write_3D(ptr address_space(3) %dst,``
  ``ptr address_space(1) %src, size_t %width, size_t %dst_line_stride,``
  ``size_t %src_line_stride, size_t %height, size_t %dst_plane_stride,``
  ``size_t %src_plane_stride, size_t %depth,
  ``__mux_dma_event_t %event)`` - DMA 3D write from ``%src`` to ``%dst`` of
  ``%width`` bytes, ``%height`` rows, and ``%depth`` planes, with
  ``%dst_line_stride`` bytes between dst rows, ``%src_line_stride`` bytes
  between src rows, ``%dst_plane_stride`` bytes between dst planes, and
  ``src_plane_stride`` between src planes. May use ``%event`` from previous DMA
  call. Returns event used.
* ``void __mux_dma_wait(i32 %num_events, __mux_dma_event_t*)`` - Wait on
  events initiated by a DMA read or write.
* ``size_t __mux_get_global_linear_id()`` - Returns a linear ID equivalent
  to ``(__mux_get_global_id(2) - __mux_get_global_offset(2)) *``
  ``__mux_get_global_size(1) * __mux_get_global_size(0) +``
  ``(__mux_get_global_id(1) - __mux_get_global_offset(1)) *``
  ``__mux_get_global_size(0) + (__mux_get_global_id(0) -``
  ``__mux_get_global_offset(0))``.
* ``size_t __mux_get_local_linear_id(void)`` - Returns a linear ID equivalent
  to ``__mux_get_local_id(2) * __mux_get_local_size(1) *``
  ``__mux_get_local_size(0) + __mux_get_local_id(1) * __mux_get_local_size(0)``
  ``+ __mux_get_local_id(0)``.
* ``size_t __mux_get_enqueued_local_size(i32 i)`` - Returns the enqueued
  work-group size in the ``i``'th dimension, for uniform work-groups this is
  equivalent to ``size_t __mux_get_local_size(i32 %i)``.
* ``void __mux_mem_barrier(i32 %scope, i32 %semantics)`` - Controls the order
  that memory accesses are observed (serves as a fence instruction). This
  control is only ensured for memory accesses issued by the invocation calling
  the barrier and observed by another invocation executing within the memory
  ``%scope``. Additional control over the kind of memory controlled and what
  kind of control to apply is provided by ``%semantics``. See `below
  <#memory-and-control-barriers>`__ for more information.
* ``void __mux_work_group_barrier(i32 %id, i32 %scope, i32 %semantics)`` and
  ``void __mux_sub_group_barrier(i32 %id, i32 %scope, i32 %semantics)`` - Wait
  for other invocations of the work-group/sub-group to reach the current point
  of execution (serves as a control barrier). A barrier identifier is provided
  by ``%id`` (note that implementations **must** ensure uniqueness themselves,
  e.g., by running the ``compiler::utils::PrepareBarriersPass``). These
  builtins may also atomically provide a memory barrier with the same semantics
  as ``__mux_mem_barrier(i32 %scope, i32 %semantics)``. See `below
  <#memory-and-control-barriers>`__ for more information.

Group operation builtins
~~~~~~~~~~~~~~~~~~~~~~~~

ComputeMux defines a variety of builtins to handle operations across a
sub-group, work-group, or *vector group*.

The builtin functions are overloadable and are mangled according to the type of
operand they operate on.

Each *work-group* operation takes as its first parameter a 32-bit integer
barrier identifier (``i32 %id``). Note that if barriers are used to implement
these operations, implementations **must** ensure uniqueness of these IDs
themselves, e.g., by running the ``compiler::utils::PrepareBarriersPass``. The
barrier identifier parameter is not mangled.

.. note::

   The sub-group and work-group builtins are all **uniform**, that is, the
   behaviour is undefined unless all invocations in the group reach this point
   of execution.

   Future versions of ComputeMux **may** add **non-uniform** versions of these
   builtins.

The groups are defined as:

* ``work-group`` - a group of invocations running together as part of an ND
  range. These builtins **must** only take scalar values.
* ``sub-group`` - a subset of invocations in a work-group which can synchronize
  and share data efficiently. ComputeMux leaves the choice of sub-group size
  and implementation to the target; ComputeMux only defines these builtins with
  a "trivial" sub-group size of 1. These builtins **must** only take scalar
  values.
* ``vec-group`` - a software level group of invocations processing data in
  parallel *on a single invocation*. This allows the compiler to simulate a
  sub-group without any hardware sub-group support (e.g., through
  vectorization). These builtins **may** take scalar *or vector* values. The
  scalar versions of these builtins are essentially identical to the
  corresponding ``sub-group`` builtins with a sub-group size of 1.


``any``/``all`` builtins
++++++++++++++++++++++++

The ``any`` and ``all`` builtins return ``true`` if any/all of their operands
are ``true`` and ``false`` otherwise.

.. code:: llvm

   i1 @__mux_sub_group_any_i1(i1 %x)
   i1 @__mux_work_group_any_i1(i32 %id, i1 %x)
   i1 @__mux_vec_group_any_v4i1(<4 x i1> %x)

``broadcast`` builtins
++++++++++++++++++++++

The ``broadcast`` builtins broadcast the value corresponding to the local ID to
the result of all invocations in the group. The sub-group version of this
builtin takes an ``i32`` sub-group linear ID to identify the invocation to
broadcast, and the work-group version take three ``size_t`` indices to locate
the value to broadcast. Unused indices (e.g., in lower-dimension kernels)
**must** be set to zero - this is the same value returned by
``__mux_get_global_id`` for out-of-range dimensions.

.. code:: llvm

   i64 @__mux_sub_group_broadcast_i64(i64 %val, i32 %sg_lid)
   i32 @__mux_work_group_broadcast_i32(i32 %id, i32 %val, i64 %lidx, i64 %lidy, i64 %lidz)
   i64 @__mux_vec_group_broadcast_v2i64(<2 x i64> %val, i32 %vec_id)

``reduce`` and ``scan`` builtins
++++++++++++++++++++++++++++++++

The ``reduce`` and ``scan`` builtins return the result of the group operation
for all values of their parameters specified by invocations in the group.

Scans may be either ``inclusive`` or ``exclusive``. Inclusive scans perform the
operation over all invocations in the group. Exclusive scans perform the
operation over the operation's identity value and all but the final invocation
in the group.

The group operation may be specified as one of:

* ``add``/``fadd`` - integer/floating-point addition.
* ``mul``/``fmul`` - integer/floating-point multiplication.
* ``smin``/``umin``/``fmin`` - signed integer/unsigned integer/floating-point minimum.
* ``smax``/``umax``/``fmax`` - signed integer/unsigned integer/floating-point maximum.
* ``and``/``or``/``xor`` - bitwise ``and``/``or``/``xor``.
* ``logical_and``/``logical_or``/``logical_xor`` - logical ``and``/``or``/``xor``.

Examples:

.. code:: llvm

   i32 @__mux_sub_group_reduce_add_i32(i32 %val)
   i32 @__mux_work_group_reduce_add_i32(i32 %id, i32 %val)
   float @__mux_work_group_reduce_fadd_f32(i32 %id, float %val)

   i32 @__mux_sub_group_scan_inclusive_mul_i32(i32 %val)
   i32 @__mux_work_group_scan_inclusive_mul_i32(i32 %id, i32 %val)
   float @__mux_work_group_scan_inclusive_fmul_f32(i32 %id, float %val)

   i64 @__mux_sub_group_scan_exclusive_mul_i64(i64 %val)
   i64 @__mux_work_group_scan_exclusive_mul_i64(i32 %id, i64 %val)
   double @__mux_work_group_scan_exclusive_fmul_f64(i32 %id, double %val)

   i64 @__mux_vec_group_scan_exclusive_mul_nxv1i64(<vscale x 1 x i64> %val)


Sub-group ``shuffle`` builtin
+++++++++++++++++++++++++++++

The ``sub_group_shuffle`` builtin allows data to be arbitrarily transferred
between invocations in a sub-group. The data that is returned for this
invocation is the value of ``%val`` for the invocation identified by ``%lid``.

``%lid`` need not be the same value for all invocations in the sub-group.

.. code:: llvm

   i32 @__mux_sub_group_shuffle_i32(i32 %val, i32 %lid)

Sub-group ``shuffle_up`` builtin
++++++++++++++++++++++++++++++++

The ``sub_group_shuffle_up`` builtin allows data to be transferred from an
invocation in the sub-group with a lower sub-group local invocation ID up to an
invocation in the sub-group with a higher sub-group local invocation ID.

The builtin has two operands: ``%prev`` and ``%curr``. To determine the result
of this builtin, first let ``SubgroupLocalInvocationId`` be equal to
``__mux_get_sub_group_local_id()``, let the signed shuffle index be equivalent
to this invocation’s ``SubgroupLocalInvocationId`` minus the specified
``%delta``, and ``MaxSubgroupSize`` be equal to
``__mux_get_max_sub_group_size()`` for the current kernel.

* If the shuffle index is greater than or equal to zero and less than the
  ``MaxSubgroupSize``, the result of this builtin is the value of the ``%curr``
  operand for the invocation with ``SubgroupLocalInvocationId`` equal to the
  shuffle index.

* If the shuffle index is less than zero but greater than or equal to the
  negative ``MaxSubgroupSize``, the result of this builtin is the value of the
  ``%prev`` operand for the invocation with ``SubgroupLocalInvocationId`` equal
  to the shuffle index plus the ``MaxSubgroupSize``.

All other values of the shuffle index are considered to be out-of-range.

``%delta`` need not be the same value for all invocations in the sub-group.

.. code:: llvm

   i8 @__mux_sub_group_shuffle_up_i8(i8 %prev, i8 %curr, i32 %delta)

Sub-group ``shuffle_down`` builtin
++++++++++++++++++++++++++++++++++

The ``sub_group_shuffle_down`` builtin allows data to be transferred from an
invocation in the sub-group with a higher sub-group local invocation ID down to
a invocation in the sub-group with a lower sub-group local invocation ID.

The builtin has two operands: ``%curr`` and ``%next``. To determine the result
of this builtin , first let ``SubgroupLocalInvocationId`` be equal to
``__mux_get_sub_group_local_id()``, the unsigned shuffle index be equivalent to
the sum of this invocation’s ``SubgroupLocalInvocationId`` plus the specified
``%delta``, and ``MaxSubgroupSize`` be equal to
``__mux_get_max_sub_group_size()`` for the current kernel.

* If the shuffle index is less than the ``MaxSubgroupSize``, the result of this
  builtin is the value of the ``%curr`` operand for the invocation with
  ``SubgroupLocalInvocationId`` equal to the shuffle index.

* If the shuffle index is greater than or equal to the ``MaxSubgroupSize`` but
  less than twice the ``MaxSubgroupSize``, the result of this builtin is the
  value of the ``%next`` operand for the invocation with
  ``SubgroupLocalInvocationId`` equal to the shuffle index minus the
  ``MaxSubgroupSize``. All other values of the shuffle index are considered to
  be out-of-range.

All other values of the shuffle index are considered to be out-of-range.

``%delta`` need not be the same value for all invocations in the sub-group.

.. code:: llvm

   float @__mux_sub_group_shuffle_down_f32(float %curr, float %next, i32 %delta)

Sub-group ``shuffle_xor`` builtin
+++++++++++++++++++++++++++++++++

These ``sub_group_shuffle_xor`` builtin allows for efficient sharing of data
between items within a sub-group.

The data that is returned for this invocation is the value of ``%val`` for the
invocation with sub-group local ID equal to this invocation’s sub-group local
ID XOR’d with the specified ``%xor_val``. If the result of the XOR is greater
than the current kernel's maximum sub-group size, then it is considered
out-of-range.

.. code:: llvm

   double @__mux_sub_group_shuffle_xor_f64(double %val, i32 %xor_val)

Memory and Control Barriers
---------------------------

The mux barrier builtins synchronize both memory and execution flow.

The specific semantics with which they synchronize are defined using the
following enums.

The ``%scope`` parameter defines which other invocations observe the memory
ordering provided by the barrier. Only one of the values may be chosen
simultaneously.

.. code:: cpp

  enum MemScope : uint32_t {
    MemScopeCrossDevice = 0,
    MemScopeDevice = 1,
    MemScopeWorkGroup = 2,
    MemScopeSubGroup = 3,
    MemScopeWorkItem = 4,
  };

The ``%semantics`` parameter defines the kind of memory affected by the
barrier, as well as the ordering constraints. Only one of the possible
**ordering**\s may be chosen simultaneously. The **memory** field is a
bitfield.

.. code:: cpp

  enum MemSemantics : uint32_t {
    // The 'ordering' to apply to a barrier. A barrier may only set one of the
    // following at a time:
    MemSemanticsRelaxed = 0x0,
    MemSemanticsAcquire = 0x2,
    MemSemanticsRelease = 0x4,
    MemSemanticsAcquireRelease = 0x8,
    MemSemanticsSequentiallyConsistent = 0x10,
    MemSemanticsMask = 0x1F,
    // What kind of 'memory' is controlled by a barrier. Acts as a bitfield, so
    // a barrier may, e.g., synchronize both sub-group, work-group and cross
    // work-group memory simultaneously.
    MemSemanticsSubGroupMemory = 0x80,
    MemSemanticsWorkGroupMemory = 0x100,
    MemSemanticsCrossWorkGroupMemory = 0x200,
  };

Atomics and Fences
------------------

The LLVM intermediate representation stored in
``compiler::BaseModule::finalized_llvm_module`` **may** contain any of the
following atomic instructions:

* `cmpxchg`_ for the `monotonic ordering`_ with *strong* semantics only
* `atomicrmw`_ for the following opcodes: ``add``, ``and``, ``sub``, ``min``,
  ``max``, ``umin``, ``umax``, ``or``, ``xchg``, ``xor`` for the `monotonic
  ordering`_ only

.. _cmpxchg: https://llvm.org/docs/LangRef.html#cmpxchg-instruction
.. _atomicrmw: https://llvm.org/docs/LangRef.html#atomicrmw-instruction

A compiler **shall** correctly legalize or select these instructions to ISA
specific operations.

The LLVM intermediate representation stored in
``compiler::BaseModule::finalized_llvm_module`` **may** also contain any of the
following atomic instructions:

* `cmpxchg`_ for the `monotonic ordering`_ with *weak* semantics
* `load`_ with the instruction marked as *atomic* for the `monotonic ordering`_
  only
* `store`_ with the instruction marked as *atomic* for the `monotonic ordering`_
  only
* `fence`_ for the `acquire ordering`_, `release ordering`_ and `acq_rel ordering`_
  only

.. _load: https://llvm.org/docs/LangRef.html#load-instruction
.. _store: https://llvm.org/docs/LangRef.html#store-instruction
.. _fence: https://llvm.org/docs/LangRef.html#fence-instruction

A compiler **may** choose not to support these instructions depending on which
open standards it wishes to enable through the oneAPI Construction Kit. For example;
support for the OpenCL C 3.0 standard requires support for these instructions.

The atomic instructions listed above **shall not** have a `syncscope`_
argument.

No lock free requirements are made on the above atomic instructions. A target
**may** choose to provide a software implementation of the atomic instructions
via some other mechanism such as a hardware mutex.

.. _monotonic ordering: https://llvm.org/docs/LangRef.html#ordering
.. _acquire ordering: https://llvm.org/docs/LangRef.html#ordering
.. _release ordering: https://llvm.org/docs/LangRef.html#ordering
.. _acq_rel ordering: https://llvm.org/docs/LangRef.html#ordering
.. _syncscope: https://llvm.org/docs/LangRef.html#syncscope

Metadata
--------

The following table describes metadata which can be introduced at different stages of the
pipeline:

.. list-table:: Function Metadata
   :widths: 25 25 50
   :header-rows: 1

   * - Name
     - Fields
     - Description
   * - ``!reqd_work_group_size``
     - i32, i32, i32
     - Required work-group size encoded as *X*, *Y*, *Z*. If not present, no
       required size is assumed.
   * - ``!max_work_dim``
     - i32
     - Maximum dimension used for work-items. If not present, ``3`` is assumed.
   * - ``!codeplay_ca_wrapper``
     - various (incl. *vectorization options*)
     - Information about a *kernel entry point* regarding its work-item
       iteration over *sub-kernels* as stitched together by the
       ``WorkItemLoopsPass`` pass in the ``compiler::utils`` module. Typically
       this involves the loop structure, the vectorization width and options of
       each loop.
   * - ``!codeplay_ca_vecz.base``
     - *vectorization options*, ``Function*``
     - Links one function to another, indicating that the function acts as the
       *base* - or *source* - of vectorization with the given vectorization
       options, and the linked function is the result of a *successful*
       vectorization. A function may have *many* such pieces of metadata, if it
       was vectorized multiple times.
   * - ``!codeplay_ca_vecz.derived``
     - *vectorization options*, ``Function*``
     - Links one function to another, indicating that the function is the
       result of a *successful* vectorization with the given vectorization
       options, using the linked function as the *base* - or *source* - of
       vectorization. A function may only have **one** such piece of metadata.
   * - ``!codeplay_ca_vecz.base.fail``
     - *vectorization options*
     - Metadata indicating a *failure* to vectorize with the provided
       vectorization options.
   * - ``!mux_scheduled_fn``
     - i32, i32(, i32, i32)?
     - Metadata indicating the function parameter indices of the pointers to
       MuxWorkItemInfo and MuxWorkGroupInfo structures, respectively. A
       negative value (canonicalized as -1) indicates the function has no such
       parameter. Up to two additional custom parameter indices can be used by
       targets.
   * - ``!intel_reqd_sub_group_size``
     - i32
     - Required sub-group size encoded as a 32-bit integer. If not present, no
       required sub-group size is assumed.

Users **should not** rely on the name, format, or operands of these metadata.
Instead, utility functions are provided by the ``utils`` module to work with
accessing, setting, or updating each piece of metadata.

.. note::
  The metadata above which refer to *vectorization options* have no concise
  metadata form as defined by the specification and **are not** guaranteed to
  be backwards compatible. See the C++ utility APIs in the ``utils`` module as
  described above for the specific information encoded/decoded by
  vectorization.

.. list-table:: Module Metadata
   :widths: 25 25 50
   :header-rows: 1

   * - Name
     - Fields
     - Description
   * - ``!opencl.ocl.version``
     - A single operand, itself containing !{i32, i32}
     - The major/minor OpenCL C version that this module is compatible with. If
       unset the compiler assumes 1.2. The compiler will infer different
       semantics and supported builtin functions depending on this metadata.
   * - ``!mux-scheduling-params``
     - string, string, ...
     - A list of scheduling parameter names used by this target. Emitted into
       the module at the time scheduling parameters are added to functions that
       requires them (see ``AddSchedulingParametersPass``). The indices found
       in ``!mux_scheduled_fn`` function metadata are indices into this list.

Function Attributes
-------------------

The following table describes function attributes which can be introduced at
different stages of the pipeline:

.. list-table:: Function Attributes
   :widths: 25 50
   :header-rows: 1

   * - Attribute
     - Description
   * - ``"mux-kernel"/"mux-kernel"="x"``
     - Denotes a *"kernel"* function. Additionally denotes a
       *"kernel entry point"* if the value is ``"entry-point"``. `See below
       <#mux-kernel-attribute>`__ for more details.
   * - ``"mux-orig-fn"="val"``
     - Denotes the name of the *"original function"* of a function. This
       original function may or may not exist in the module. The original
       function name is propagated through the compiler pipeline each time
       ComputeMux creates a new function to wrap or replace a function.
   * - ``"mux-base-fn-name"="val"``
     - Denotes the *"base name component"* of a function. Used by several
       passes when creating new versions of a kernel, rather than appending
       suffix upon suffix.

       For example, a pass that suffixes newly-created functions with
       ``".pass2"`` will generate ``@foo.pass1.pass2`` when given function
       ``@foo.pass1``, but will generate simply ``@foo.pass2`` if the same
       function has ``"mux-base-name"="foo"``.
   * - ``"mux-local-mem-usage"="val"``
     - Estimated local-memory usage for the function. Value must be a positive
       integer.
   * - ``"mux-work-item-order"="val"``
     - Work-item order (the dimensions over which work-items are executed from
       innermost to outermost) as defined by the ``utils_work_item_order_e``
       enum. If not present, ``"xyz"`` may be assumed.
   * - ``"mux-barrier-schedule"="val"``
     - Typically found on call sites. Determines the ordering of work-item
       execution after a berrier. See the `BarrierSchedule` enum.

``mux-kernel`` attribute
~~~~~~~~~~~~~~~~~~~~~~~~

ComputeMux programs generally consist of a number of *kernel functions*, which
have a certain programming model and may be a subset of all functions in the
*module*.

ComputeMux compiler passes often need to identity kernel functions amongst
other functions in the module. Further to this, a ComputeMux implementation may
know that an even smaller subset of kernels are in fact considered *kernels
under compilation*. In the interests of compile-time it is not desirable to
optimize kernels that are known to never run.

Under this scheme, it is further possible to distinguish between kernels that
are *entry points* and those that aren't. Entry points are kernels which may be
invoked from the runtime. Other kernels in the module may only be run when
invoked indirectly: called from kernel entry points.

The ``mux-kernel`` function attribute is used to communicate *kernels under
compilation* and *kernel entry points* (a subset of those) between passes. This
approach has a myriad of advantages. It provides a stable, consistent, kernel
identification method which other data do not: names cannot easily account for
new kernels introduced by optimizations like vectorization; calling conventions
are often made target-specific at some point in the pipeline; pointers to
functions are unstable when kernels are replaced/removed.

Passes provided by ComputeMux ensure this attribute is updated when adding,
removing, or replacing kernel functions. Each ComputeMux pass in its
documentation lists whether it operates on *kernels* or *kernel entry points*,
if applicable.
