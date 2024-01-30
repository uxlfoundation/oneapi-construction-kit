# ComputeAorta Changes

## Version 4.0.0

Upgrade guidance:

* Support for SPIR 1.2 programs (`cl_khr_spir`) has been dropped.
* The mux spec has been bumped:
  * 0.77.0: to loosen the requirements on the mux `event` type used by
    DMA builtins.
  * 0.78.0: to introduce mux builtins for sub-group, work-group, and
    vector-group operations.
  * 0.79.0: to introduce mux builtins for sub-group shuffle operations.
  * 0.80.0: to introduce support for 64-bit atomic operations.
* The `compiler::ImageArgumentSubstitutionPass` now replaces sampler typed
  parameters in kernel functions with i32 parameters via a wrapper function.
  The `host` target as a consequence now passes samplers to kernels as 32-bit
  integer arguments, not as integer arguments disguised as pointer values.
* The `compiler::utils::ReplaceBarriersPass` has been replaced with the
  `compiler::utils::LowerToMuxBuiltinsPass`.
* The `compiler::utils::HandleBarriersPass` has been renamed to the
  `compiler::utils::WorkItemLoopsPass`.
* The `compiler::utils::createLoop` API has moved its list of `IVs` parameter
  into its `compiler::utils::CreateLoopOpts` parameter. It can now also set the
  IV names via a second `CreateLoopOpts` field.
* Support for LLVM versions is now limited to LLVM 16 and LLVM 17. Support for
  earlier LLVM versions has been removed.
* Support for FMA (fused multiply-add) is required for the device. For the host
  device for x86-64, this means only x86-64-v3 and newer are supported. This
  roughly translates to 2015 or newer, both for Intel and for AMD.
  * Although hardware support for FMA is available on all platforms we currently
    test, if you are using OCK on a platform we do not test and encounter
    issues, please let us know by opening an issue!
* `compiler-utils` library has been split into `compiler-pipeline` and
  `compiler-binary-metadata` to  allow use of compiler pipeline utilities without
   the binary metadata requirements. Both will be needed for `mux` targets.
## Version 3.0.0

Upgrade guidance:

* The mux spec has been bumped:
  * 0.74.0: to account for changes to `BaseModule`.
  * 0.75.0: to remove code supporting `cl_codeplay_program_snapshot`.
  * 0.76.0: to add code supporting `cl_intel_required_subgroup_size`.
* The `cl_codeplay_program_snapshot` extension has been removed.
* `cl::binary::ProgramInfo` and `cl::binary::KernelInfo` have been removed and
  replaced with equivalent structures in the `compiler` namespace.
* `compiler::Module::finalize` no longer takes a callback to populate program
  info - it now takes `compiler::ProgramInfo` by pointer and populates it
  itself if passed a non-null address.

Feature additions:

* The `cl_intel_required_subgroup_size` OpenCL extension is now supported. Note
  that no in-tree targets currently report any supported sub-group sizes so no
  kernels compiled with a required sub-group size will successfully compile.
* The SPIR-V `SubgroupSize` execution mode is now supported via the
  `SubgroupDispatch` capability. Note that other aspects of this capability are
  *not* supported.
* The SPIR-V `OptNoneINTEL` capability is now supported.
* `compiler::BaseModulePassMachinery` can now be given compiler options to
  guide its decisions.
* A new method - `compiler::BaseModulePassMachinery::handlePipelineElement` -
  has been added to allow more customizable target-specific pipeline component
  parsing.

## Version 2.0.0 - 2023-05-23

Upgrade guidance:

* The project license has been changed to the Apache License version 2.0 with
  LLVM Exceptions. See LICENSE.txt for the full license text.
* Support for LLVM 14 was dropped.
* Snapshots are no longer supported for `riscv`/`refsi` targets.
* Support for DXIL has been removed. This includes:
  * DXIL builtin info;
  * DXIL binary support in `veczc`;
  * The `dx2llvm` tool.
* The `-cl-wi-order` command-line option has been removed. Work item ordering
  can no longer be user-specified.
* The project is now universally built in C++17 mode.
* An individual contributor license agreement has been added in the
  CONTRIBUTING file.
* A code of conduct has been added for how developers and users should interact
  with the oneAPI Construction Kit project once it has been open-sourced.
* Offline OpenCL kernels compiled with `clc` are now serialized with the metadata API 
  and stored using a different binary format. Thus, kernels compiled with a previous 
  version of `clc` will no longer work. You will need to recompile all your offline 
  kernels for them to be accepted by ComputeAorta.
* The compiler is now always built in what was previously 3.0 mode.
    * ComputeAorta will now build when enabling host image support and OpenCL
      3.0, but will warn that the image support is known non-conformant to 3.0.
* Compiler:
    * Each `compiler::BaseTarget` now owns its own `LLVMContext`, rather than
      the (shared) `compiler::BaseContext` owning it.
    * The `compiler::utils` module has been updated to move all members under a
      single namespace `compiler::utils`. Replace references to `core::utils`,
      `::utils`, and `utils` in the context of this module, with
      `compiler::utils`.
    * The `CoreWorkItemInfo`, `CoreWorkGroupInfo`, and `Core_schedule_info_s`
      structure types have been renamed, switching "Core" to "Mux".
    * The `CorePackedArgs` structure type has been renamed. It is now named as
      `MuxPackedArgs.` followed by the name of the kernel whose parameters it
      wraps. This guarantees a unique naming more often, as previously two kernels
      with different signatures would both compete for the same structure name,
      leading to `CorePackedArgs` and `CorePackedArgs.0`.
    * The kernel sub-group size is now communicated from the compiler to the mux
      target through `handler::GenericMetadata` and
      `handler::VectorizeInfoMetadata`.
    * The `compiler::utils::SimpleCallbackLegacyPass` has been removed.
    * The `compiler::utils::AddKernelWraperPass` now takes a set of options on
      construction, wrapping up the previous `bool` parameter. The pass can now opt
      in to packing kernel pointer parameters in the `local`/`Workgroup` address
      space as pointers, rather than by `size_t` (representing the size).
    * `compiler::utils::createLoop` has been refactored to take options by a
      structure. The `isVP` parameter has also been removed.
    * `multi_llvm/element_count_helper.h` has been removed. Please use the
      equivalent LLVM functionality directly.
    * The `ReplaceMuxDmaPass` has been renamed to `DefineMuxDmaPass`. It now uses
      `BuiltinInfo` and calls `defineMuxBuiltin` on all ComputeMux DMA functions.
      Targets should override `BIMuxInfoConcept::defineMuxBuiltin` to customize the
      lowering of mux DMA builtins.
    * ComputeMux runtimes now handle multiple compiler-generated variants of a
      given kernel, selecting between them at runtime.
    * The helper function `compiler::utils::IsBarrierName` has been removed. Use
      `compiler::utils::BuiltinInfo::isMuxBarrierID` instead.
    * `compiler::utils::BarrierRegions` now queries the `BuiltinInfo` for what were
      known as _movable work-item calls_. The
      `BuiltinInfo::isRematerializableBuiltinID` API now handles this, allowing
      targets to customize behaviour.
    * The class names for the `riscv` target and generated targets are now
      prefixed with the target name capitalized e.g. RiscvTarget.
    * The entry point to the `LLD` linker has been moved into a new
      `compiler-linker-utils` library. Targets must opt-in to link against this
      library.
    * `BuiltinID` enums specific to OpenCL have been hidden from the API,
      and most functions accepting or returning BuiltinIDs now use the new `Builtin`
      and `BuiltinCall` structs to hold all the required information. Changes:
        * `BuiltinInfo::analyzeBuiltin()` now returns a `Builtin` struct
          containing both the builtin properties and the builtin ID. Use of
          language-specific IDs is discouraged outside of the implementations
          themselves. Mux builtin IDs are still exposed.
        * `BuiltinInfo::identifyBuiltin()` has been removed from the API. Use
          `BuiltinInfo::analyzeBuiltin()` instead and obtain the ID from the
          `Builtin` struct returned.
        * `BuiltinInfo::isBuiltinUniform()` has been removed from the
          BuiltinInfo API (although it is still present on the
          `BILangInfoConcept`). Use `BuiltinInfo::analyzeBuiltinCall()` to get
          the uniformity instead.
        * The following property queries have been removed and replaced by
          flags in the `BuiltinProperties` enum:
            * `BuiltinInfo::isBuiltinAtomic()` replaced by `eBuiltinPropertyAtomic`
            * `BuiltinInfo::isRematerializableBuiltinID()` replaced by
              `eBuiltinPropertyRematerializable`
            * Functions formerly accepting a `BuiltinID` now accept a `Builtin
              const &`.
            * The static functions `BuiltinInfo::getInvalidBuiltin()` and
              `BuiltinInfo::getUnknownBuiltin()` have been removed.
              Implementations can use the corresponding enums directly. In
              addition, the `Builtin` struct has `isValid()` and `isUnknown()`
              member functions, for convenience.
    * The unused `BuiltinInfo::emitInstanceID` function was removed.
    * The vectorizer now calls `__mux_get_local_id` and `__mux_get_local_size`
      when establishing the vector length in vector predication mode. This is
      instead of deferring to the BuiltinInfo to create a local id or local
      size.
    * The unused `BuiltinInfo::emitLocalID` and `BuiltinInfo::emitLocalSize`
      functions were removed.
    * The `BuiltinInfo::getGlobalIdBuiltin` and
      `BuiltinInfo::getLocalIdBuiltin` functions were removed as they were too
      language-specific and the existance of one and only one corresponding
      builtin.
    * Targets may need to provide their own version of the
      `compiler::utils::ReplaceAddressSpaceQualifierFunctionsPass` if different
      address spaces are supported in hardware, if they wish to support the
      feature.
    * The `HandleBarriersPass` no longer sets `combined_vecz_scalar`,
      `vecz_scalar_loop_induction`, or `vecz_wrapper_loop` metadata.
    * The unused `compiler::utils::mutateScalarPeelInductionStart` function has
      been removed.
    * The vectorizer no longer sets `vecz_vector_predication` metadata on
      vector-predicated kernels. This information is available through the
      orig<->vecz link metadata.
    * All `compiler::utils::Pass*` compiler passes have been renamed to
      `compiler::utils::*Pass`.
    * The `compiler::utils` module header paths have been moved to
      `compiler/utils`. Replace header includes of `<utils/*>` with
      `<compiler/utils/*`, except for `<include/utils/system.h>`, which is not
      a part of the `compiler::utils` module.
    * More data is now reported from the compiler to the runtime via
      `handler::VectorizeInfoMetadata`:
        * `min_work_item_factor`
        * `pref_work_item_factor`
    * `check-passes-host-lit` has been renamed to `check-host-compiler-lit`.
    * The `compiler::utils::SetBarrierConvergentPass` is now run when compiling
      OpenCL C kernels and not just on SPIR/SPIRV modules.
    * The `compiler::utils::SetBarrierConvergentPass` has been renamed
      `SetConvergentAttrPass`.
        * The `compiler::utils::SetConvergentAttrPass` now sets the
          `convergent` attribute to any function not known to the `BuiltinInfo`
          to be non-convergent. This property is bubbled up throuh the
          call-graph so that callers of convergent functions are themselves
          marked convergent.
    * The `host` target now configures its scheduling parameters through a
      derived `compiler::utils::BIMuxInfoConcept`. It configures three
      scheduling parameters (found in the host target documentation). The
      `host::AddEntryHookPass` no longer alters the kernel ABI and purely
      performs work-group scheduling.
    * The declarations of `sub_group_*` OpenCL builtins are now marked
      convergent.
    * The compiler no longer necessarily generates kernels which retain their
      original names. The actual generated kernel name is conveyed through ELF
      metadata along with the original name, so the runtime can map between
      kernels and their generated forms.
    * The `mux-orig-fn` function attribute now truly conveys the original
      function name and should not change throughout the compiler. Its previous
      use as the base name component which is used when generating new kernel
      wrappers has been renamed as `mux-base-fn-name`.
    * The `utils::AddWorkItemInfoStructPass`,
      `utils::AddWorkGroupInfoStructPass`,
      `utils::AddWorkItemFunctionsIfRequiredPass`,
      `utils::ReplaceLocalWorkItemIdFunctionsPass` and
      `utils::ReplaceNonLocalWorkItemFunctionsPass` have been removed. Use the
      `utils::AddSchedulingParametersPass` and `utils::DefineMuxBuiltinsPass`
      in their place.
    * Many `multi_llvm` classes and methods which had no difference between
      officially-supported LLVM versions have now been removed.
    * Changes to BuiltinLoader class hierarchy:
        * `compiler::utils::BuiltinLoader` is renamed to
          `compiler::utils::CLBuiltinLoader`, and is now declared in
          `modules/compiler/utils/cl_builtin_info.h`.
        * `compiler::utils::SimpleLazyBuiltinLoader` is replaced with
          `compiler::utils::SimpleCLBuiltinLoader`, and is now declared in
          `modules/compiler/utils/cl_builtin_info.h`.
        * `compiler::utils::createSimpleLazyCLBuiltinInfo` is renamed to
          `compiler::utils::createCLBuiltinInfo`, and is now declared in
          `modules/compiler/utils/cl_builtin_info.h`.
        * `compiler::utils::LazyBuiltinLoader` has been removed. Use either
          `compiler::utils::SimpleCLBuiltinLoader` or subclass
          `compiler::utils::CLBuiltinLoader` directly instead.
    * The compiler now checks the OpenCL version compatibility of a module at
      runtime, as opposed to at build time. This is encoded as
      `!opencl.ocl.version` metadata.
    * `compiler::utils::CLBuiltinInfo` now only identifies builtins when the
      module's OpenCL version is recent enough to support those builtins.
    * `utils::populateSimpleWorkItemInfoLookupFunction`,
      `utils::populateSimpleWorkGroupInfoLookupFunction` and
      `utils::populateSimpleWorkItemInfoSetterFunction` have been removed and
      replaced with the more generic `utils::populateStructSetterFunction` and
      `utils::populateStructGetterFunction`. This is so targets can easily reuse
      these functions to define builtins that may not necessarily use the
      `MuxWorkItemInfoStruct` or `MuxWorkGroupInfoStruct` types.
    * The dependency on `utils::AddWorkItemFunctionsIfRequiredPass` from
      `utils::HandleBarriersPass` has been severed. The `HandleBarriersPass` now
      unconditionally calls the helper builtins `__mux_set_local_id` and
      `__mux_set_sub_group_id`. They will be optimized out by later passes if
      unused.
    * The undocumented, untested command-line option
      `-vecz-inject-debug-printfs` was removed as it crashed the compiler when
      used.
* Mux:
    * `mux_kernel_s::sub_group_size` has been removed.
    * A new mux entry point has been introduced: `muxQueryMaxNumSubGroups`.
    * Added the ``__mux_mem_barrier``, ``__mux_work_group_barrier``, and
      ``__mux_sub_group_barrier`` builtins. They replace the older
      ``__mux_global_barrier``, ``__mux_shared_local_barrier``, and
      ``__mux_full_barrier`` builtins, which have been removed.
    * The `__mux_get_kernel_width` and `__mux_set_kernel_width` builtins have
      been removed, along with the `kernel_width` entry in the default
      work-item scheduling structure.
* Tutorials, examples, and new targets:
    * The `create_target.py` script and associated `json` files have been updated to support
      the "feature" element as a ';' separated list to show multiple end points for
      tutorials. This has been updated to show end points for `clmul` tutorial and to add
      lit test support for `refsi-wrapper-pass`. Also the need to escape some quotes in the
      `json` file is no longer needed.
    * The `clmul` extension documention and new target creation has been moved
      to the compiler module as it is a compiler extension. This should require
      no changes by the user except to follow the updated instructions.
    * The `creating a new mux target` tutorial and associated scripts have been
      updated to allow a hal to be placed externally to the target source code.
      This is supported via an additional `cmake` variable
      `CA_EXTERNAL_REFSI_TUTORIAL_HAL_DIR`.
    * The refsi M1 compiler target has been split off from the main refsi
      target. The risc-v compiler target remains, but is only compatible with
      the 'G' target, which is now the default. The 'M' target now exists under
      examples/refsi/refsi_m1. Both compiler targets are now derived from a new
      compiler riscv utils library. The M1 target can still be built by using
      -DCA_EXTERNAL_MUX_COMPILER_DIRS=<ONEAPI_CON_KIT>/examples/refsi/refsi_m1/compiler/refsi_m1
      and -DCA_MUX_COMPILERS_TO_ENABLE="refsi_m1"
* hal_refsi:
    * `hal_refsi` now rounds up local buffer arguments to 128 bytes.
    * `hal_refsi` now aligns pass-by-value kernel arguments to the next power
      of in `WI` mode: not just in `WG` mode.
* SPIR-V:
    * The external `SPIRV-Headers` has been bumped to `sdk-1.3.239.0`.
    * The support for "experimental" features (via
      `CA_ENABLE_SPIRV_LL_EXPERIMENTAL` was removed)
* The submodule `source/cl/external/OpenCL-ICD-Loader` has been removed and
  replaced by cmake fetchcontent logic to fetch it from github. It can built as
  before, except that the registry setting for the ICD needs to be set on
  windows. The script icd-register.ps1 can be used for this, although
  administrator privileges will be required.
* Compiler Debug Support:
    * The `CA_DEBUG_SUPPORT_ENABLED` pre-processor define has been renamed
      `CA_ENABLE_DEBUG_SUPPORT` to match the cmake variable.
    * `CA_DEBUG_PASSES_ENABLED` pre-processor define has been renamed
      `CA_ENABLE_DEBUG_SUPPORT` to match the cmake variable.
    * A new cmake option - `CA_ENABLE_LLVM_OPTIONS_IN_RELEASE` - has been added to
      provide support for parsing `CA_LLVM_OPTIONS` in release-mode builds.
    * All debug instrumentations have been removed. Use `CA_LLVM_OPTIONS` instead:
        * `CA_PASS_PRINT=1` -> `CA_LLVM_OPTIONS=-debug-pass-manager`
        * `CA_PASS_VERIFY=1` -> `CA_LLVM_OPTIONS=-verify-each`
        * `CA_PASS_VERIFY_DUMP=1` -> `CA_LLVM_OPTIONS=-verify-each`
        * `CA_PASS_PRE_DUMP=1` ->
          `CA_LLVM_OPTIONS=-print-before/-print-before-all`
        * `CA_PASS_POST_DUMP=1` ->
          `CA_LLVM_OPTIONS=-print-after/-print-after-all`
        * `CA_PASS_SCEV_PRE_DUMP=1` -> re-compile with
          `llvm::ScalarEvolutionPrinterPass` added to the pipeline.
        * `CA_PASS_SCEV_POST_DUMP=1` -> re-compile with
          `llvm::ScalarEvolutionPrinterPass` added to the pipeline.
* The submodule `source/cl/external/OpenCL-Intercept-Layer` has been removed and
  replaced by cmake FetchContent logic to fetch it from github. It can built
  as before.
* Unified Runtime:
    * UR has had a spec bump which includes many ABI breaking changes.
    * The unified runtime submodule has been removed and is now fetched if
      CA_ENABLE_API includes `ur`. The default for CA_ENABLE_API is now `cl;vk`
      rather than all APIs as unified runtime does not fully work across all targets.
* The `metadata` library's `VectorizeInfoMetadata` now represents the 'work
  width' as a single named structure -- `FixedOrScalableQuantity<uint64_t>`,
  rather than two disjoint values for the known and scalable parts.

Feature additions:

* The `host` target now uses LLVM's `ORC`-based JIT to execute online kernels,
  rather than the older `MCJIT`.
* Added basic support for the generic address space in OpenCL 3.0.
* The `CA_HOST_TARGET_CPU` environment variable may be set to `"native"` at
  runtime to compiler and execute JIT kernels for the host system. The
  behaviour should be identical to that when the project is built with
  `-DCA_HOST_TARGET_CPU=native`.
* Introduce the ability to specialize builtin functions for specific targets.
  This is done using the `add_target_builtins()` CMake command and providing a
  list of source files which implement optimized versions of builtins for a
  target architecture. Targets can then access the embedded LLVM bitcode at
  runtime to make use of the optimized builtin functions.
* A target-specific `host-utils` library has been added to serve both `mux` and
  `compiler` modules. Any other target may add their own equivalent library in
  `modules/utils/target/<target-name>`.
    * The `host-common` library has been removed; its contents have been moved
      to `host-utils`.
* Add prototype support for the oneAPI Unified Runtime API. This builds upon
  ComputeMux to enable another path of integration into the DPC++ SYCL runtime
  for ComputeAorta.
    * Implement the new compiler API for UR.
    * `urEnqueue*` functions no longer block unless explicitly told to.
    * `urEnqueue*` functions can now wait for an artbitrary list of events to
      complete before starting execution.
    * Implement urEnqueueUSMMemcpy.
    * Implement `urQueueFinish` and `urQueueFlush` entry points.
* Compiler changes:
    * Created a mechanism for implementing degenerate subgroups selectively on
      some kernels and not others. It will also clone kernels to produce two
      versions (using degenerate and non-degenerate subgroups) when the local
      size is not known at compile time.
    * More optimized Abacus sqrt(float) builtin function.
    * A new overload of `compiler::utils::createKernelWrapperFunction` has been
      added to simplify the process of creating a wrapper with the same
      function prototype as the old function.
    * A new utils pass `compiler::utils::ReplaceMemcpySetIntrinsicsPass` has
      been created which will replace calls to intrinsics `llvm.memcpy.*`,
      `llvm.memset.*` and `llvm.memmove.*` with calls to a generated equivalent
      loop. This pass has now been added to the `riscv` target and
      conditionally to the cookie cutter generated target, based off the `json`
      "feature" element `replace_mem`.
    * Two new passes - `utils::AddSchedulingParametersPass` and
      `utils::DefineMuxBuiltinsPass` - have been added to replace the old pass
      framework of adding scheduling paramaters and replacing work-item
      builtins:
        * `core::utils::AddWorkItemFunctionsIfRequiredPass`
        * `core::utils::AddWorkItemInfoStructPass`
        * `core::utils::ReplaceLocalWorkItemIdFuncsPass`
        * `core::utils::AddWorkGroupInfoStructPass`
        * `core::utils::ReplaceNonLocalWorkItemFuncsPass`
      The two replacement passes utilize `utils::BuiltinInfo` APIs concerning
      scheduling parameters (see `utils::BuiltinInfo::SchedParamInfo` and
      associated methods) and for defining mux work-item builtins (see
      `utils::BuiltinInfo::defineMuxBuiltin`)
    * LLVM Intrinsics are now handled directly by the main BuiltinInfo object,
      instead of the language/target-dependent implementations, ensuring
      consistent scalarization and vectorization/widening.
    * `lldLinkToBinary` now returns the linked object file (or error) by value,
      rather than being passed to the function. Error messages have been added
      for when the function fails.
    * The `riscv` and cookie-cutter targets now register the LLVM assembly
      parsers, meaning they can use inline assembly if they wish.
    * If the `HandleBarriersPass` finds a vector kernel and a vector-predicated
      kernel with the same source, they are combined into one wrapper. The
      vector-predicated kernel will not generate its own entry point, even if
      it is marked as an entry point.
    * `utils::BuiltinInfo` can be used to get-or-declare mux work-item builtin
      functions.
* `SPIR-V` changes:
    * `spirv-ll-tool` now outputs to stdout when given `-o -`.
    * `spirv-ll` now generates calls to ComputeMux synchronization builtins for
      `OpControlBarrier` (`__mux_work_group_barrier` and `__mux_sub_group_barrier`)
      and `OpMemoryBarrier` (`__mux_mem_barrier`).
    * `spirv-ll` now supports the `SPV_KHR_expect_assume` extension.
    * `spirv-ll` now supports the `GenericPointer` storage class, targeting address
      space 4.
    * `spirv-ll` now supports the `SPV_KHR_linkonce_odr` extension.
    * `spirv-ll` now supports the `SPV_KHR_uniform_group_instructions` extension.
    * `spirv-ll` now supports the `SPV_INTEL_arbitrary_precision_integers` extension.
    * Implement SPIR-V extensions:
        * SPV_EXT_shader_atomic_float_add
        * SPV_EXT_shader_atomic_float_min_max
* New LIT check targets have been introduced:
    * `check-host-lit` runs all mux/compiler lit tests for the `host` target
    * `check-riscv-lit` runs all mux/compiler lit tests for the `riscv` target
    * `check-mux-lit` runs all mux lit tests for all targets.
    * `check-compiler-lit` runs all compiler tests for all targets.
* `vecz` changes:
    * Vecz can now handle kernels featuring irreducible control flow, by
      running the new Fix Irreducible pass.
    * Vecz Stride Analysis can now determine stride linearity on simple
      loop-carried PHI nodes regardless of which incoming value is the initial
      value and which is the increment.
    * Vecz is now able to vectorize shuffle vector instructions (without
      scalarizing them first) when a scalable vectorization factor is
      requested.
    * Improved efficiency of reductions over subvector packets by reducing to a
      single vector first and then reducing that to scalar, rather than the
      other way round.
    * Optimized Insert Element operations for scalable vectorization factors on
      RISC-V.
* Tutorials, examples, and new targets:
    * The copyright on files generated from create_target.py has been updated,
      allowing a customer copyright to be added via the json field
      `copyright_name`.
    * The `clmul` add custom builtin extensions tutorial has been updated to
      use a derived version of `CLBuiltinInfo` as this is a more correct
      method.
    * The oneAPI Construction Kit has been extended to also include `clik` and
      examples HALs such as `hal_refsi_tutorial`. The `hal` submodule has been
      moved to the top level as a subtree so it can be shared with `clik`.
    * Added support to create_target to assume llvm_cpu, llvm_features and
      llvm_triple are expressions by default and add support for putting ' at
      beginning or end of the json field to signify ".

Non-functional changes:

* Vecz: Removed handling of builtin functions from the Vectorization Unit. The
  Vectorization Context is now directly responsible for returning everything
  required to vectorize a builtin.
* Updated HAL documentation to show requirements for use with `clik`.
* Code structure:
    * The `modules/builtins` directory has been moved to
      `modules/compiler/builtins`.
    * The `modules/spirv-ll` module has been moved to
      `modules/compiler/spirv-ll`.
    * The `modules/vecz` module has been moved to `modules/compiler/vecz`.
* riscv-isa-sim now fetches from Codeplay's github fork
* Store degenerate subgroup information as function attribute.
* `BuiltinInfo` interface changes:
    * `BuiltinInfo::identifyBuiltin()` now takes a `llvm:Function` const
    reference instead of a StringRef of the name.
    * `BuiltinInfo::analyzeBuiltin()` now takes a `llvm::Function` const
      reference instead of a const pointer.
    * `BuiltinInfo::isArgumentVectorized()` has been removed.
    * The BuiltinInfo interface no longer has `emitBuiltinInline` that accepts
      a BuiltinID. Use the version that accepts a Function pointer instead.
    * The Builtininfo interface no longer has `materializeBuiltin` function,
      since this is only used internally by specific implementations.
    * Refactor of BuiltinInfo API to remove exposure of internal BuiltinIDs and
      create a universal way to pass builtin information between BuiltinInfo
      functions instead of having to use IDs in some places and function
      pointers in others.
    * Simplified BuiltinLoader hierarchy and made it specific to CLBuiltinInfo.
      CLBuiltinInfo can now be constructed directly from a Builtins module.
    * CLBuiltinInfo now assigns Builtin IDs for `VLoad`/`VStore` (including
      `_half` variants), and also `select` and `as_` builtins, simplifying code
      and easing maintenance.

Bug fixes:

* Fixed issue with non-user event dependencies across queues for OpenCL. This
  is a fix for the following case:
    Queue 1 : clEnqueueNDRange -> event_kernel
    Queue 2 : clEnqueueReadBuffer <- wait event_kernel, -> event_read
    clWaitForEvents(event_read)
   This should all be internal and require no user changes.
* Fix for clReleaseCommandQueue leaving event functions in a bad state. The fix
  involves retaining the command queue in the event and also adding an
  effective finish when clReleaseCommandQueue is called, when we have events
  waiting. Additionally wait events have to be retained until they are cleaned
  up once they are removed from the dispatch list. This fixes a number of
  issues including CTS fail `test_api/queue_flush_on_release`,
  and vexcl/events test.
* The 32-bit arm host target now uses the hard-float ABI, which was incorrectly
  being lost and set to the soft-float ABI.
* A couple of bugs were fixed in the `CL_KERNEL_LOCAL_SIZE_FOR_SUB_GROUP_COUNT`
  sub-group APIs, which could return uninitialized data.
* A couple of data races on LLVM variables have been fixed.
* Host Kernel compilation will now return an error state when finalization
  fails.
* Fixed the use of `CA_MUX_COMPILERS_TO_ENABLE` which could be used to choose
  which compiler targets were enabled. `CA_MUX_COMPILERS_TO_ENABLE` can be used
  to decide which compiler directories to be added. If
  `CA_MUX_COMPILERS_TO_ENABLE` was passed in, the name of the library to be added
  was taken from this variable rather than the actual name of the library which
  would probably differ from the directory name. The logic is changed so that
  the library name to be added is taken from `MUX_COMPILER_LIBRARIES`, which was
  added to when the compiler target was added using `add_mux_compiler_target`.
  This allows `CA_MUX_COMPILERS_TO_ENABLE` to be used effectively to limit to one
  compiler library or another. `CA_MUX_TARGETS_TO_ENABLE` has been changed to use
  the same logic i.e. it only enables the directory inclusion.
* The `riscv` and generated targets have been updated to store the
  `VectorizeInfoMetadata` directly, removing the dependency on the compiler,
  allowing `CA_RUNTIME_COMPILER_ENABLED` to be used on risc-v and other targets.
* Fixed an issue where a `cl_command_buffer_khr` could not be used by to two
  different `cl_command_queue`s simultaneously, this now behaves correctly.
* `vecz`:
    * Fix crash in Control Flow Conversion caused by presence of Switch
      instructions, which are not handled.
    * The command line tool `veczc` now has a command line option that allows
      vectorization to fail without returning an error state, for testing
      failures.
    * When scalarizing a vector splat which is used by a non-scalarizable
      instruction, the scalarizer no longer inserts multiple insert element
      instructions to reconstruct the value, but restores the vector splat
      instead.
    * Vector-predicated kernels are now named with "vp" to avoid clashes with
      identically-vectorized kernels without vector predication.
    * `sub_group_scan_exclusive_*` builtins are now correctly
      scalably-vectorized for `half` and `double` types and no longer crash the
      compiler backend.
    * Linear work-item schedules, produced by certain work-group collective
      builtins, no longer generate invalid IR when vectorized.
* Compiler passes:
    * The `compiler::utils::AlignModuleStructsPass` has been fixed so that it
      pads struct types more closely matching the user-facing SPIR ABI. This
      also means it no longer pads in cases where it previously unnecessarily
      did.
    * The `compiler::utils::ReduceToFunctionPass` was fixed in the presence of
      multiple vectorized forms of a kernel. It would previously crash if a
      base function was linked to more than one vectorized form.
    * The `compiler::utils::EncodeBuiltinRangeMetadataPass` no longer sets the
      maximum global size based on the device's `max_concurrent_work_items`
      value, which refers to the *local* size. The pass now accepts a 3D values
      of global sizes, though those are not currently set by the compiler
      itself.
    * The `compiler::utils::ReplaceLocalModuleScopeVariablesPass` can handle
      global variables used by comparison instructions.
    * Local-address-space accumulator global variables are now generated with
      `internal` linkage to fix potential unresolved symbols.
    * Use of generic address space in legacy atomics from SpirV-ll will no
      longer result in a crash or linker error.
* `SPIR-V`:
    * A bug was fixed in handling function calls to function forward
      references, which was previously generating invalid LLVM IR.
    * `spirv-ll` will now correctly mangle boolean types.
    * A bug was fixed where the incorrect group reduction operation would be
      called if the sign of the input integer type did not match the sign of
      the group operation.
    * Signed and unsigned integer OpenCL-Ext builtins now correctly mangle
      their argument types based on the operation, not based on the integer
      type.
* Calling `printf` on half floating-point types now works correctly in all
  cases.
* Work-group collective functions of `half` types no longer crash the compiler.
* Builtins:
    * CL Builtins `length` and `fast_length` are now emitted as `fabs` for
      scalar types; `normalize` and `fast_normalize` are emitted as `sign`.
    * Fixed undefined behaviour in the implementation of trigonometric builtins
      that only manifested in rare edge cases (when invoked with a constant
      argument).
* Fix a couple of minor memory issues in UnitUR.

## Version 1.75.0 - 2022-10-31

Upgrade guidance:

* The `!ca_barrier_schedule` metadata is now a (call-site) function attribute:
  `mux-barrier-schedule`.
* `core::utils::copyParameterAttrs` has been renamed
  `core::utils::copyFunctionAttrs` to better reflect its role.
* `!dma_reqd_size` metadata is now encoded as `mux-dma-reqd-size` attributes.
* `!work_item_order` metadata is now encoded as `mux-work-item-order` attributes.
* `!codepay_ca_kernel` metadata is now encoded as `mux-kernel` attributes.
  Kernels are functions with any `mux-kernel` attribute, and entry points are
  those with the specific attribute value `"entry-point"`.
* The `riscv::AddElfMetadataPass()` was removed. Use
  `core::utils::AddMetadata<AnalysisTy, HandlerTy>()` to encode binary metadata
  instead.
* `BaseModule::runBackendPasses` and `BaseModule::addLatePasses` have been
  removed and fused into the new `BaseModule::getLateTargetPasses`. This method
  returns a pass pipeline that is implicitly run by `BaseModule::finalize`.
* The vector of printf descriptors provided to the
  `compiler::PassPrintfReplacement` pass is now passed by pointer rather than
  by reference, and may be `nullptr` if the descriptors populated by the pass
  are unwanted by the user.
* `compiler::utils::DiagnosticHandler` has been removed. Its only use in
  `compiler::BaseTarget` has been replaced with an LLVM diagnostic handler
  callback function, keeping LLVM's default-constructed diagnostic handler in
  place.
* `compiler::DiagHandler` has been removed. Its only uses in
  `compiler::BaseModule` now use `compiler::ScopedDiagnosticHandler`.
* `compiler::PassCheckForDoubles` no longer bubbles failure up to
  `compiler::BaseModule::finalize` via a `bool` passed on construction. It now
  raises a custom `compiler::DiagnosticInfoDoubleNoDouble` diagnostic which is
  intercepted by the compiler.
* A new pass - `compiler::PassCheckForExtFuncs` - has been introduced to raise
  a `compiler::DiagnosticInfoExternalFunc` diagnostic if the module contains an
  unresolved external function. It replaces an old extra-pipeline check in
  `BaseModule::finalize`. Its pass ID used for debugging is `check-ext-funcs`.
* The pass `core::utils::AutoDmaPass` has been removed. The `OpenCL` command
  options has had `-cl-dma` removed. This has also been removed from `clc`.
* `utils::PassMachinery`'s `bool debugLog` parameter has been expanded to an
  enum of different debugging levels. See `utils::DebugLogging`.
* Kernel functions now better preserve their original names when wrapped by
  functions created internally by the compiler. They are given a suffix, and a
  function string attribute `"mux-orig-fn"` whose value retains the original
  name. This original name is propagated through successive wrapper functions.
  The passes this affects are:
  * `core::utils::AddKernelWrapperPass`
  * `core::utils::HandleBarriersPass`
  * `core::utils::ReplaceLocalModuleScopeVariablesPass`
  * `core::utils::ReplaceLocalWorkItemIdFunctionsPass`
  * `host::AddFloatingPointControlPass`
  * `host::AddEntryHookPass`
  * `riscv::RefSiWrapperPass`
  For targets where the final wrapped kernels must be given the original kernel
  names, a helper pass -- `core::utils::RenameKernelEntryPointsPass` -- has
  been introduced to rename kernel entry points according to their
  `mux-orig-fn` original names. Running this pass late in the pipeline after
  all wrappers have been created will preserve the previous behaviour.
* The `host` target's compiler pipelines have been further streamlined and
  consolidated.

Feature additions:

* A new mux-level `BuiltinInfo` implementation is available. Targets may
  inherit from `utils::BIMuxInfoConcept` and override methods such as
  `defineMuxBuiltin` to customize how Mux builtins are defined.
* Both the `riscv` and generated target now support the `-cl-wi-order` program
  build options.
* Sub group scan operations are now implemented using `log2(width)` vector
  shuffles instead of a linear loop. Two new functions are provided in
  `vecz::TargetInfo` for generating vector shuffles, one for general
  single-source shuffles that works with dynamic shuffle masks, and another
  to rotate vector elements up by one place and inserting a given new element
  at the beginning.
* A new pass has been added to aid in the computation of local memory usage in
  kernels: `core::utils::ComputeLocalMemoryUsagePass`. This pass aims to
  replace the `core::utils::computeLocalMemoryUsage` helper function, which
  required the module to first be reduced to just the kernel functions in
  question. The `ComputeLocalMemoryUsagePass` can be run on an entire module,
  encoding more accurate memory usage for each kernel in function attributes.
* A new `core::utils::AddMetadata<AnalysisTY, HandlerTy>()` was added to unify
  encoding binary metadata into an offline compiled kernel for host and RISC-V.
  Custom targets with target-specific metadata can use this pass to store
  metadata in their offline kernels by implementing the required analysis which
  extracts the metadata from the IR.
* A `BaseModule::llvmFatalErrorHandler` is provided for targets to conveniently
  redirect fatal errors to the build log.
* Support for two LLVM options - `--time-passes` and `--debug-pass-manager` -
  has been added to provide additional information about compiler pass
  pipelines, mimicking LLVM's `opt` tool.
  * `--time-passes`  will individually time compiler passes and print a timing
    summary report for each individual compiler pipeline.
  * `--debug-pass-manager`:
    * (no value) Enables the tracing and printing of compiler pass pipeline
      structure.
    * `=quiet` As above but don't print information for analyses
    * `=verbose` As above but also print adaptors and pass managers
* Support for the LLVM option `--verify-each` has been added to optionally
  verify the IR before/after each pass
* Added UnitCL tests for the optional work-group collective feature. All
  supported integer types are tested. At present floating point types are not
  tested.
* Support for Vector Length Predication and special RISC-V implementations of
  the following `vecz::TargetInfo` functions:
  * `createVectorShuffle()`
  * `createVectorSlideUp()`

Bug fixes:

* The `host` target's `host::AddFloatingPointControlPass` now preserves
  function metadata and function attributes when creating a wrapper function.
  It also now correctly transfers function attributes to the call site. It no
  longer applies `alwaysinline` attributes to functions with `noinline`
  attributes.
* `-cl-opt-disable` now generates `optnone` function attributes in LLVM,
  disabling more optimizations.
* Replace calls to `get_sub_group_size` directly in the packetizer, so that the
  predicated Vector Length can be substituted directly, instead of going via
  the work items info struct, which is broken when there is a barrier.
* Clang diagnostics are forwarded on to the target's notify callback function,
  providing users with a more complete set of diagnostics for the whole
  compilation process.
* OpenCL will now check that write mappings resulting from `clEnqueueMapBuffer`
  don't overlap and will return the correct error code (`CL_INVALID_OPERATION`)
  if they do.
* Crashes, aborts, and assertion failures in compiler targets are now safely
  handled and no longer bring down the ComputeMux process.
* In OpenCL C, calls to `printf` containing '%v16' vector specifiers are no
  longer removed from the program, and other two-digit vector lengths between
  `%v10` and `%v19` are now correctly identified as erroneous as according to
  the specification.
* Several function passes have been fixed so that they run on `optnone`
  functions, where previously they'd been skipped. This affects:
  * `compiler::PassBitShiftFixup`
  * `compiler::PassCheckForDoubles`
  * `compiler::PassCombineFPExtFPTrunc`
  * `compiler::PassSoftwareDivision`
  * `compiler::StripFastMathAttrs`
  * `core::utils::MakeFunctionNameUniquePass`
* A new pass - `RemoveByValAttributesPass` - has been introduced to work around
  a LLVM X86 code generation bug when compiling for x86-64. See
  https://github.com/llvm/llvm-project/issues/34300.
* Kernel functions now better preserve function attributes, parameter
  attributes, return attributes, parameter names, and function metadata when
  being wrapped by functions created internally by the compiler. The wrapped
  call to the original function also preserves all attributes. The passes this
  affects are:
  * `core::utils::AddKernelWrapperPass`
  * `core::utils::HandleBarriersPass`
  * `core::utils::ReplaceLocalModuleScopeVariablesPass`
  * `refsi::RefSiWrapperPass`
  * `host::AddEntryHookPass`
  * `host::AddFloatingPointControlPass`

## Version 1.74.0 - 2022-10-05

Upgrade guidance:

* LLVM command-line options are no longer used to create a `TargetMachine` in
  the `riscv` compiler target. The `llvm_command_line_options` field has also
  been removed from the new-target configuration; it is no longer used when
  creating a `TargetMachine`.
* Vecz itself no longer contains any defaults for SIMD width. Vecz can no longer
  be invoked with a vectorization factor of zero. Use `wfv=auto` to activate the
  SIMD Width Analysis and the given factor will be used as a maximum.
* The `VeczLowerSwitchWrapperPass` has been removed. Targets should use LLVM's
  `LowerSwitchPass` directly instead.
* The `VeczUnifyFunctionExitNodesWrapperPass` has been removed. Targets should
  use LLVM's `UnifyFunctionExitNodesPass` directly instead.
* The `NewFlattenCFGPass` has been removed. Targets should use LLVM's
  `FlattenCFGPass` directly instead.
* The default OpenCL version is now OpenCL 3.0. If you wish to continue using
  OpenCL 1.2, but your build scripts don't specify the version with
  `-DCA_CL_STANDARD=1.2`, ensure that you add this where appropriate.
* Support for LLVM versions is now limited to LLVM 14 and LLVM 15. Support for
  LLVM versions 8 through 13 has been removed.
* Many `multi_llvm` helper functions are no longer needed and have been removed,
  they include functions in:
  * Type size helpers
  * `ElementCount` helpers
  * Pointer opacity queries
  * Target registry wrappers
  * `Attribute` helper wrappers
  * The `ShuffleVectorIndex` typedef
  * A `CodegenOptimization` typedef
* `cl_codeplay_command_buffer_mutable_dispatch` is no longer a reported
  extension, and instead `cl_khr_command_buffer_mutable_dispatch` is reported to
  users now that it has been provisionally ratified.
* The vectorizer now retrieves `vecz::TargetInfo` via a new analysis pass -
  `vecz::TargetInfoAnalysis`. A default implementation of this analysis is
  provided for targets using a `BaseModulePassMachinery`. All targets may also
  register their own version of this analysis.
* The `debug` module was trimmed down, with `debug/support.h` removed entirely.
* Mux targets are now required to clone the `mux_command_buffer_t` during
  `muxCloneCommandBuffer` such that ND range kernel commands are deep copied.
* The `create_new_target.py` script has been updated to only support out of tree
  creation. It now expects an additional `CMakelists.txt` template in
  `ComputeAorta/external/cookie/{{cookiecutter.external_dir}}/CMakeLists.txt`.
  An additional argument has been added, `--external-dir`. Additionally the
  default templates now call the target `refsi_tutorial`.
* `GenericModule` and `GenericTarget` which were previously part of `riscv`
  target and any new target creation no longer exists and new targets will
  recreate these in their entirety.

Feature additions:

* Created the Linear Barrier Schedule, which executes all work items in a
  barrier region in Linear Local ID order. This can be selected by adding the
  relevant metadata to the associated barrier call. Default behaviour is still
  *Vector then Scalar*, which can be explicitly selected if required to guard
  against the default schedule changing in future.
* Compiler device information can be retrieved from passes via the new
  `utils::DeviceInfoAnalysis`. This has removed several pass constructor
  parameters from utility passes.
* A new mux binary metadata API was implemented, which unifies previous kernel
  metadata techniques used within ComputeAorta.
* A header file to help support linking with `lld` has been provided under
  `utils` and can be accessed using `core::utils::lldLinkToBinary()`. The
  `riscv` target and the generated target have been updated to use this.
* Compiler targets deriving from `BaseModule` can now initialize their custom
  pass machineries and register custom analyses for the `BaseModule::finalize`
  compilation pipeline without having to override `finalize` itself. See
  `BaseModule::initializePassMachineryForFinalize`.
* Compiler targets deriving from `BaseModule` can now initialize their custom
  pass machineries and register custom analyses for the various `BaseModule`
  front-end compilation pipelines without having to override them themselves.
  See `BaseModule::initializePassMachineryForFrontend`.
* Targets may preserve command-line options when calling
  `core::utils::lldLinkToBinary` with the helper function
  `core::utils::appendMLLVMOptions`.
* Work-group collective scan operations are now implemented in terms of
  sub-group scans, which greatly simplifies the IR and allows Vecz to produce
  optimized vector code.
* The maximum SIMD width for the `host` target is now 64, although a width of 16
  will still be used when using `wfv=always` to limit impact on performance in
  this configuration.
* Code generation of work-group collective scan operations in terms of sub-group
  scan operations was improved by removing the unnecessary sub group reduction
  operation and extracting the last value from the sub group scan instead.
* Barriers can now be given metadata to indicate different execution orderings
  of the work items in the barrier region. Currently supports only "Unordered"
  (i.e. any ordering is allowed; the default) and "Once" (for regions that only
  need to execute a single arbitrary work item). Work Group Collective
  Reductions now use a "Once" barrier region to initialize their accumulators.
* A new series of compiler pass-pipeline builder functions has been introduced
  to share common code between *base* compiler targets.

Non-functional changes:

* All of the `Generic*` classes have been removed from `riscv` target and also
  from the generated target using `create_target.py` script. Functionally the
  generated target should be the same as before, but with values spread around a
  bit more, mostly in the `Target` class.

Bug fixes:

* Fix a null pointer dereference in OpenCL 3.0 code which uses
  `sub_group_barrier`.
* Pushing a duplicate key to a metadata hash-table is now correctly checked and
  the correct error code is returned.
* Improperly defined feature test macros for optional features were fixed. Now
  the following feature test macros are only defined if the device actually
  supports them:
  * `__opencl_c_program_scope_global_variables`
  * `__opencl_c_atomic_order_acq_rel`
  * `__opencl_c_atomic_order_seq_cst`
  * `__opencl_c_device_enqueue`
  * `__opencl_c_generic_address_space`
  * `__opencl_c_pipes`
  * `__opencl_c_read_write_images`
  * `__opencl_c_images`
  * `__opencl_c_3d_image_writes`
* `__opencl_c_subgroups` is now defined when subgroup support is enabled on the
  device.
* Fixed a bug in the Vecz Common GEP Elimination Pass where two GEPs could be
  identified as equivalent when opaque pointers are used and the source element
  types are different.
* Fix alignment issue arising because an opaque pointer prevents an array
  `alloca` from being replaced with a vector `alloca`, which results in memory
  accesses having a more strict alignment than the `alloca`.
* `utils::PassMachinery::initializeStart` no longer registers any LLVM passes or
  analyses. This is left to `PassMachinery::initializeFinish`. This allows
  targets to register their own versions of LLVM passes like
  `llvm::TargetLibraryInfoAnalysis` before calling `initializeFinish`.
* A couple of crashes were fixed in `compiler::BaseModule::deserialize` when
  provided an empty buffer and when `llvm::Module` deserialization failed.
* A bug has been fixed in the AutoDma pass where DMA accesses were incorrectly
  analyzed.
* Add the `SPV_INTEL_kernel_attributes` SPIR-V extension to the list of
  supported extensions in OpenCL. This fixes a bug where modules containing
  `OpExtension "SPV_INTEL_kernel_attributes"` would be rejected erroneously.
* `spirv-ll-tool` no longer crashes when encountering `OpTypePointer` types that
  point to `OpTypeForwardPointer` pointers.
* When `clUpdateMutableCommandsKHR()` is used on a command-buffer pending
  execution it will no-longer prematurely update it, and instead perform the
  correct behaviour of only updating the command in subsequent command-buffer
  enqueues.

## Version 1.73.0 - 2022-09-07

Upgrade guidance:

* The `core::utils::HandleBarriersPass` now infers the size in bytes of the
  `size_t` type from the module rather than being passed by the user. The pass's
  constructor now takes a `core::utils::HandleBarriersOptions` structure which
  wraps the remaining previous arguments.
* Bump `cl_khr_command_buffer` to version 0.9.1, which defines an error if
  `clFinalizeCommandBufferKHR` is called on an already finalized command-buffer.
* The `PassPipeline` and `GenericPassPipeline` utility classes were removed and
  its functionality was moved into utility functions that help the construction
  of a compilation pipeline. See the new methods in
  `modules/compiler/targets/riscv/include/riscv/common/generic_pass_pipeline.h`
* The unused legacy version of the `ReduceToFunctionPass` was removed.
* Update OpenCL-Headers to `68d98613113259dbcd4c05b2039bb758766bf2ac` to include
  definitions of extension `cl_khr_command_buffer_mutable_dispatch`.
  Applications must now use the installed `CL/cl_ext.h` header for these
  definitions rather than `cl_command_buffer_mutable_dispatch.h` which has been
  removed.
* Target-specific snapshots for targets deriving from `compiler::BaseTarget` can
  now supplied by filling out
  `compiler::BaseModule::supported_target_snapshots`.
  `compiler::BaseTarget::listSnapshotStages` handles this data structure
  automatically, meaning derived targets should need to override this method
  less often.
* Modules deriving from `compiler::BaseModule` now support multiple active
  snapshots in `compiler::BaseModule::snapshots`, a vector of active snapshots.
  This replaces the singular optional `compiler::BaseModule::snapshot_details`.
* `compiler::BaseModule` now provides several static snaphot query and taker
  methods for convenience. See `compiler::BaseModule::shouldTakeSnapshot`,
  `compiler::BaseModule::takeSnapshot`, etc.
* `compiler::BaseModule` now provides a static method to query for an enabled
  snapshot and, if enabled, schedule a callback pass to take that snapshot. See
  `compiler::BaseModule:::addSnapshotPassIfEnabled`.
* The `muxTryWait` API now takes a timeout parameter. See the ComputeMux Runtime
  spec for details on the semantics of this argument.
* The `mux_error_command_buffer_not_ready` error code has been replaced with
  `mux_fence_not_ready`.
* The `mux_error_command_buffer_failure` error code has been replaced with
  `mux_error_fence_failure`.
* The ``mux_error_command_buffer_wait_semaphore_failure`` error code has been
  removed.
* `__core` builtin functions are renamed to `__mux` and so old bitcode will no
  longer work. Targets must either update their compiler or run the
  `RenameBuiltinsPass`.
* The revision of the `cl_khr_command_buffer_mutable_dispatch` extension
  implemented is updated to 22.
* Both in-tree targets now take lists of `std::string`s for kernel names, rather
  than `llvm::StringRef`s.
* Three metadata-related functions have been moved from `utils/pass_functions.h`
  to `utils/metadata.h`. In doing so, `cargo` types were converted to `llvm`
  types and function names were changed to start with a lower-case letter. See
  `parseRequiredWGSMetadata` (+ overload) and `parseMaxWorkDimMetadata`.
* The generic target function `riscv::LinkerFn` no longer takes a name for the
  entry point.
* The `lldLinkToBinary` now sets the dummy entry point at the zero address,
  rather than a provided kernel function.
* A new `multi_llvm` header -- `optional_helper.h` -- has been introduced to
  allow seamless use of `llvm::Optional::getValueOr` which is deprecated in LLVM
  15+ in favour of `llvm::Optional::value_or`.
* `compiler::GenericTarget` now creates its `compiler::PassPipeline` on demand,
  rather than having one created for it and passed in on construction.
* `compiler::GenericModule` now creates its `llvm::TargetMachine` (rather than
  `compiler::GenericPassPipeline`) and passes that to the
  `compiler::PassPipeline` on creation.
* `compiler::GenericTarget` now conceptually owns its list of valid snapshot
  stages, rather than the `compiler::PassPipeline`.
* `compiler::GenericTarget` now creates its `utils::PassMachinery` when
  compiling a module and passes that to the `compiler::PassPipeline`, rather
  than the `compiler::PassPipeline` creating its own `utils::PassMachinery`.
* `utils::BuiltinInfo` is now provided to passes via an analysis:
  `utils::BuiltinInfoAnalysis`. This affects the construction of both
  `vecz::TargetInfo` and `utils::OptimalBuiltinReplacementPass`. The analysis
  pass is only run once and never invalidated.
* Targets should ensure that the `utils::BuiltinInfoAnalysis` analysis is run
  before the `utils::OptimalBuiltinReplacementPass` is run, as this pass may
  only access *cached* analyses. Targets can run the
  `llvm::RequireAnalysisPass<utils::BuiltinInfoAnalysis>` pass to ensure this.
* The `utils::BuiltinInfo` base class now handles `mux` builtins, rather than
  `utils::CLBuiltinInfo`. This allows all targets to benefit from shared
  analysis and handling of `mux` builtins.
* The `core::utils::LinkBuiltins` pass now receives the builtins module to link
  via the `utils::BuiltinInfoAnalysis`. Its `Module *` constructor parameter has
  been removed.
* `core::utils::ReplaceCoreDmaPass` was renamed
  `core::utils::ReplaceMuxDmaPass`. Its pass ID has consequently changed to
  `replace-mux-dma`.
* `riscv::RefSiReplaceCoreDmaPass` has been renamed
  `riscv::RefSiReplaceMuxDmaPass`. Its pass ID has consequently changed to
  `refsi-replace-mux-dma`.
* `core::utils::ReplaceCoreMathDeclsPass` was renamed
  `core::utils::ReplaceMuxMathDeclsPass`. Its pass ID has consequently changed
  to `replace-mux-math-decls`.

Feature additions:

* Enable support for `fp16` with the vector extension on `RISC-V` if selected in
  the `HAL_DESCRIPTION`.
* A new method `loadBuiltinsPCH` has been added to `compiler::BaseModule`.
  Calling this function will load the pre-compiled OpenCL builtins library
  header into a clang compiler instance. This function must be called after
  having already prepared and begun a `clang::CodeGenAction`.
* A new method `populateCodeGenOpts` has been added to `compiler::BaseModule`.
  Calling this function will populate a `clang::CodeGenOptions` object with
  default option values required for compiling OpenCL.
* The compiler `riscv` target now supports the possibility of SPIR-V producing
  the builtins for `get_global_linear_id()`, `get_local_linear_id()` or
  `get_enqueued_local_size()`.
* A new optimization pass - `utils::EncodeBuiltinRangeMetadataPass` - has been
  added to inform the compiler about the range of values that certain builtins
  may return. This pass is scheduled by default early in the compilation process
  to make the most of the optimization. Builtin ranges have been implemented for
  OpenCL (for users of `utils::CLBuiltinInfo`) builtins `get_work_dim`,
  `get_(global|local)_size` and `get_(global|local)_id`.
* Make use of the `timeout` parameter of `muxTryWait` in the `vkWaitForFences`
  entry point so that waiting on fences is no longer handled in software at the
  VK level and can be implemented in hardware by a Mux target where supported.
* A custom C++ allocator -`callback_allocator` was added, which wraps a
  user-supplied `realloc()` callback. 
* A new testing target `UnitMD`, was added as a unit testing tool for all
  metadata related code.
* `core::utils::RenameBuiltinsPass` is added for backwards compatibility with
  targets that do not yet implement the new `__mux` builtin functions.
* A new helper function has been added to provide a default way for derived
  targets to fill in the initial vector of kernels and their required work-group
  sizes for the purposes of compilation. See `core::utils::populateKernelList`.
* A new pass -- `utils::TransferKernelMetadataPass` -- has been added to perform
  the common setup of kernel metadata for compilation. Its pass ID for debugging
  is `transfer-kernel-metadata`.
* The `utils::EncodeKernelMetadataPass` now takes only one kernel's data to
  encode into metadata. Users wishing to encode multiple kernels' metadata may
  use the `utils::TransferKernelMetadataPass` pass.
* A new method `setOpenCLInstanceDefaults` has been added to
  `compiler::BaseModule`. Calling this function will set up a clang instance
  with the correct SPIR triple and language options for compiling OpenCL.
* The compiler `riscv` target has now split the `lld` linker part, which is now
  passed through from `riscv::RiscvInfo`.
* Serialization and De-Serialization utilities were added to support mux binary
  metadata format.
* A new method `prepareOpenCLInputFile` has been added to
  `compiler::BaseModule`. Calling this function will prepare and return a clang
  front-end input file to be used during compilation of OpenCL kernel source
  code.
* A new metadata API was added for managing kernel metadata in mux.
* A new `basic_map` has been introduced to replace `std::map` in metadata.
* Work-group collective reduction operations are now implemented in terms of
  sub-group reductions, which greatly simplifies the IR and allows Vecz to
  produce optimized vector code.

Non-functional changes:

* Add lit tests to the pass `refsi-replace-mux-dma`.
* The `refsi` target has been more optimal by improving when the wrapper pass
  was run and by not writing to the input.
* UnitCL floating point data now generates floats distributed uniformly within
  the field of floating point values, rather than uniformly as real values. This
  gives a much better range of possible values, since a uniform distribution
  between float min and float max will tend to result in everything having a
  very large exponent. Therefore we now generate random floats based on
  uniformly random bit representations.
* UnitCL Floating point Subgroup reduction and scan tests now exclude infinities
  and NaNs from their data set. Including NaN in the data set at any point
  results in reduce_add necessarily returning NaN, regardless of any other
  values present, so it is not helpful to include it.
* The code that handle the splitting of kernels into barrier regions (`class
  core::utils::barrier`) has been split out from `HandleBarriersPass.cpp` into
  separate files (`BarrierRegions.cpp` and `include/barrier_regions.h`). Handle
  Barriers Pass now only contains the code to create the wrapper function from
  the barrier regions/sub-kernels.

Bug fixes:

* The `riscv` target now obeys the `-codeplay-soft-math` option when optimizing
  kernels.
* The vectored version of the `abacus` builtin provided for `atan2pi` has been
  fixed.
* `spirv-ll` now correctly mangles calls to the 2D/3D `work_group_broadcast`
  overloads.
* Fixed a dangling pointer dereference and subsequent `cl_event` segfault when
  the contained queue was destroyed before the containing event.
* The `oclc` tool now waits until the binary is finalized before querying
  whether a snapshot was hit or not. This prevents the tool from mistakenly
  outputting the raw object binary in addition to the queried snapshot.
* For geometric UnitCL tests, a correct type has been given to `ULP` when
  comparing the expected and actual value, so `half_length`, `half_distance` and
  `half_normalize` will not fail. The UnitCL test `Geometric_04_Half_Normalize`
  has been disabled due to overflow on its use of `dot` builtin.
* Fixed the old OpenCL-CTS to correctly cast a pointer for the `printf` test,
  which breaks when building with LLVM 15.
* The `CL/cl_khr_command_buffer_mutable_dispatch.h` header is now included as
  part of the install includes, so that external customers can use the
  extension.
* On `RISC-V` previously if no `HAL` was found it would try to build without it
  and then showed an error at runtime. It now checks for any `HAL` directories
  and also if the expected `HAL` has a `CMakeLists.txt` and errors out if this
  is not so.

## Version 1.72.0 - 2022-08-01

Upgrade guidance:

* Vecz is now on by default, using auto-vectorization. Anyone wishing to run
  without vecz must now explicitly set `-cl-wfv=never` from the command line,
  or set `options.vectorization_mode = compiler::VectorizationMode::NEVER` on
  the `compiler::Options` object.
* Targets **must** implement the new `mux_fence_s` object along with the
  `muxCreateFence`, `muxDestroyFence` and `muxResetFence` entry points. See the
  ComputeMux Runtime spec for the semantics of these entry points.
* `muxDispatch` has been updated to take an optional `mux_fence_t` argument,
  `muxTryWait` and `muxWait` have been updated to wait on `mux_fence_t` objects
  rather than `mux_command_buffer_t` objects. See the ComputeMux runtime spec
  for details of the updated semantics of these entry points.
* The option to remove local fences in `utils::HandleBarriersPass` has been
  removed. The default behaviour - set to `false` - is now the only option.
* `utils::PassMachinery` initialization has been changed:
  * `initialize` has been split into `initializeStart` and `initializeFinish`.
  * `initializeStart` calls `registerPasses`. Users *may* override
    `registerPasses` *or* register passes inline in between calling
    `initializeStart` and `initializeFinish`.
  * `utils::PassMachinery::buildDefaultAAPipeline` and
    `utils::PassMachinery::registerLLVMAnalyses` are provided to make common
    registries simpler.
* Targets no longer need to implement the `-cl-vec` option.
* `MemOp`s now no-longer model any call instruction with pointer operands,
  only known memory builtins are considered `MemOp`s. This change removes the
  `Pointer`-kind `MemOp`s entirely.
* `BaseTarget` now owns the `builtins` module it creates. Both the `HostTarget`
  and `GenericTarget` derived targets no longer own their own builtins modules.
* `GenericTarget` now passes the builtins module around by pointer, in case it
  is uninitialized by `BaseTarget`.

Feature additions:

* Add `cargo::unique_lock` which a wrapper around `std::unique_lock` that
  exposes the subset of operations it is possible to represent using the Clang
  thread-safety analysis attributes.
* Implement `host:fence_s` and use it make the implicit fence in
  `host::command_buffer_s` explicit.
* Add the `muxCreateFence`, `muxDestroyFence` and `muxResetFence` entry points
  to the Mux spec.
* The SIMD Width Analysis will now fail (i.e. return `0`) if live values don't
  fit into vector registers at any width (including a width of 1), and
  packetization will bail out of vectorization in this case.
* Add new `mux_sync_point_s` type to the ComputeMux Runtime for intra
  command-buffer synchronization. Update all command recording entry-points
  to take a list of sync-points to wait on before executing a command, as well
  as returning a sync-point for other commands to wait on.
* Map `cl_sync_point_khr` OpenCL command-buffer sync-points down to Mux
  `mux_sync_point_s` sync-points.
* Enable work-group collectives on host using the soft implementation in
  `ReplaceWGCPass`.
* Vecz Squash Small Vectors Pass can now convert an extract element followed
  by a zero-extend or sign-extend into shifts and masks, if the result is the
  same bitwidth as the source vector.
* The `riscv` target now loads the RefSi HAL by default rather than the legacy
  Spike HAL. The Spike HAL is no longer built nor supported with ComputeAorta.
  The RefSi HAL targets the RefSi M1 virtual SoC by default, which does not
  behave the same as the target simulated by the Spike HAL. To get behaviour
  similar to the Spike HAL, RefSi G1 can be targeted instead. This can be done
  by setting `HAL_REFSI_SOC` to `G1` and `CA_RISCV_DEVICE` to `RefSi G1 RV64`
  during CMake configuration.
* Add the `DegenerateSubGroupPass` which implements sub-group builtins in
  terms of work-group builtins so that sub-group == work-group i.e. the
  degenerate sub-group case.
* Enable sub-groups on the host target and make use of the
  `DegenerateSubGroupPass` on that target.
* When the local size is known, Vecz can better analyze index expressions based
  on Local ID to guarantee load/store contiguity across work items.
* Vecz Ternary Transform Pass: restrict the conditions under which the transform
  will be applied, based on Stride Analysis.
* A new method `executeOpenCLAction` has been added to `compiler::BaseModule`.
  Calling this function will execute the supplied clang codegen action and
  return a `Result` indicating success or failure.
* Pre-vecz SLP and Loop Vectorization options are now handled in
  `compiler::BaseModule` instead of in `host`, once during the standard
  front-end pass pipeline and additionally during finalization.
* A new method `runOpenCLFrontendPipeline` has been added to
  `compiler::BaseModule`. Calling this function will run the current module
  through the OpenCL frontend LLVM pipeline, optionally running additional
  early and/or late passes specified in the call.
* A new method `getEarlyOpenCLCPasses` has been added to `compiler::BaseModule`.
  This function is intended to be used in conjuction with
  `runOpenCLFrontendPipeline` to pass sensible default early passes when
  compiling OpenCL C source.
* A new method `getEarlySPIRPasses` has been added to `compiler::BaseModule`.
  This function is intended to be used in conjuction with
  `runOpenCLFrontendPipeline` to pass sensible default early passes when
  compiling SPIR or SPIR-V source.
* Vecz Stride Analysis now uses LLVM's Known Bits analysis on uniform values, so
  that overflow can be ruled out in more cases when index expressions involve
  unsigned integers.
* A new method `getBuiltinCapabilities` has been added to `compiler::Info`.
  Calling this function will return a bitfield of the builtin capabilities of
  the device, based on the mux device info.
* The `core::utils::OptimalBuiltinReplacement` pass is now runnable with
  `muxc`. Use `optimal-builtin-replace` as its pass string.
* `GenericPassPipelineConfig`'s `llvm_command_line_options` is now a vector of
  strings. This enables more than one command-line option to be specified.
* `riscv` kernels are now built with the lp64d/ilp32d floating-point ABIs.
* The patch version of LLVM 14 supported is bumped from 14.0.0 to 14.0.3.
* Support the ability to update USM pointer arguments to kernels through the
  `cl_khr_command_buffer_mutable_dispatch` extension.
* A new query `muxQueryWFVInfoForLocalSize` has been added to ComputeMux
  Runtime for querying whole function vectorization information from the Mux
  target. Any target supporting whole function vectorization **must** implement
  this new API. Any target not supporting whole function vectorization **must**
  return the appropriate unsupported error code. See the Mux changelogs and the
  Mux spec for the relevant entry point.
* Vecz Stride Analysis: NUW property of binary operators is now respected.
* Added compiler `riscv target` lit test pass suport under
 `modules/compiler/targets/riscv/test/lit/passes`.

Non-functional changes:

* `UnitMux`, `CL`, and `VK` now make use of the updated `muxDispatch`,
  `muxTryWait` and `muxWait` entry points that synchronize with fences rather
  than command buffers.
* The HAL submodule now has a default implementation for `mem_copy` and 
  `mem_fill`.

Bug fixes:

* The `riscv` target with the vector extension enabled now preserves
  `CA_LLVM_OPTIONS` across all compiles in LLVM 15 and onwards.
* `spirv-ll` now correctly generates calls to the correct 2D/3D
  `work_group_broadcast` overloads when `OpGroupBroadcast` is passed a vector
  of local IDs rather than a scalar argument.
* Allow the OpenCL work-group collective builtins to be vectorized by handling
  all `__core_*` work-item builtins in `CLBuiltinInfo`.
* Fix incorrect work item ID comparison code.
* Insert fences before barriers in work item collective functions.
* The `vecz` `BasicMem2RegPass` now correctly promotes memory where the value
  stored to memory and the value loaded from memory differ in type and where
  there is no explicit bitcast between the types in the instruction stream. This
  can occur only in opaque-pointer mode address computation underflow when a
  large memory allocation should fail. ``hal::hal_nullptr`` is now correctly
  returned in these cases.
* A bug was fixed in the `compiler::PassPrintfReplacement` pass where when
  combining opaque pointers and `-cl-opt-disable` the format string would not
  be found and the printf would be incorrectly scrubbed from the module.
* A bug was fixed in `compiler::utils::ReplaceLocalModuleScopeVariables` pass
  where it would crash if a global was listed in the debug metadata but itself
  had no debug location.
* `vecz` now correctly packetizes calls with pointer operands with opaque
  pointers enabled.
* A vector alignment bug in `vstore_half` builtin function definitions has been
  fixed. This bug occurs if compiling with fp16 enabled, it causes wrong
  alignment when saving vectors. The correct alignment of the stores should be
  scalar, but the stores were incorrectly given vector-sized alignments. The
  script generating this builtin library is also patched. 
* Initialize variables in the UnitCL test fixture
  `muxQueryLocalSizeForSubGroupCount`.
* Fixed issue in `clSetKernelArg()` where SPIR-V kernel arguments decorated by
  `MaxByteOffset` were not having the specified value checked, and the
  `CL_MAX_SIZE_RESTRICTION_EXCEEDED` error code was not being returned in
  invalid cases.

## Version 1.71.0 - 2022-07-05

Upgrade guidance:

* `multi_llvm::Align` has been removed: use `llvm::Align` instead.
* `multi_llvm::MaybeAlign` has been removed: use `llvm::MaybeAlign` instead.
* The format of metadata emitted by `vecz` to link base and derived functions
  together has changed. The vectorization key is now its own metdata node
  linked from the two-operand key/value pair, rather than being inlined into
  the "key".
* Metadata methods concerning vectorization now consistently take/return
  `core::utils::VectorizationInfo`.
* Vector predication is now uniformly encoded in vectorization metadata links.
* The `core::utils::cloneFunctionsAddArg` and
  `core::utils::addParamToAllFunctions` helper functions now take an optional
  *callback* function to update metadata in a customizable way. This replaces
  the former `bool` which would only update `opencl.kernels` metadata.
  Furthermore, these functions now no longer take `codeplay_ca_kernel` kernel
  metadata over to the new function from the old function by default. Users who
  wish for this behaviour should pass a callback to do this themselves (e.g.,
  using `core::utils::takeKernelMetadata(newFn, oldFn)`).
* The `PartialVectorization` `vecz` choice has been removed.  This behaviour is
  the default behaviour - the vectorizer always preserves the scalar kernel. It
  is up to targets to delete/rename kernels if they so require.
* The `HandleBarriersPass` assumes partial vectorization as the default if it
  is passed a vector kernel with a scalar kernel tied to it. If the local
  work-group size in the vectorized dimension is known to be a multiple of the
  vectorization factor, the scalar kernel is omitted. There is also a global
  boolean setting for the `HandleBarriersPass` which forces the omission of
  scalar tail kernels.
* `vecz` now auto-adjusts its vectorization factor to the largest power of two
  less than or equal to the known local work-group size. This is a change from
  the previous behaviour which would find the largest *divisor* of the local
  work-group size. This means the vectorizer should use larger vectorization
  factors in general (and vectorize more often), though at the cost of
  requiring scalar iterations to execute any remaining work-items.
* The `vecz::VeczReplaceScalarFunctionsPass` pass has been removed as no
  in-tree targets require it. Targets wishing to delete/renames kernels should
  do so with a custom pass.
* All Passes are now new-style passes. All new passes must be so too.
* `core::utils::addPass` has been removed. Use PassInstrumentation features from
  the modern PassMachinery helpers.
* `multi_llvm::RunCodegenLLVMPasses` has been made an implementation detail of
  BaseModule
* `utils::BuiltinProperties::eBuiltinPropertyPointerReturn` has been split into
  `utils::BuiltinProperties::eBuiltinPropertyPointerReturnEqualRetTy` and
  `utils::BuiltinProperties::eBuiltinPropertyPointerReturnEqualIntRetTy`. This
  denotes the only kind of pointer-return properties supported: those which
  return either through a type equal to the function return type, or one which
  is an int type of equal width to the function return type.
* `utils/passes.h` has been deleted. The only remaining pass contained within -
  `core::utils::UniqueOpaqueStructsPass` - has been moved to
  `utils/unique_opaque_structs_pass.h`.
* Support for `CA_PASS_IR_PRINT`, `CA_PASS_IR_PRINT_DEVICE` and
  `CA_PASS_IR_PRINT_NO_GROUP_ID` were removed with no replacement.
* `vecz` internal masked load/store builtins now require the pointer types to
  match the data types (loaded type or stored value type). Builtins are
  stricter at asserting that this is the case at the point of calling. This is
  for consistency but also helps future-proof them for opaque pointers where
  the pointee type may not be known.
* `vecz::MemOp::createMaskedMemOp` has been refactored into two separate free
  functions `vecz::createMaskedLoad` and `vecz::createMaskedStore`. This avoids
  the implicit state held by `MemOp` from influencing how builtins are defined
  and requires the user to know what they want.
* `vecz::MemOp::createMaskedMemOp` has been turned into a free function and
  renamed as `vecz::getOrCreateMaskedMemOpFn`.
* `vecz::MemOp::createInterleavedMemOp` and
  `vecz::MemOp::createMaskedInterleavedmemOp` have been refactored into two
  separate but unified free functions `vecz::createInterleavedLoad` and
  `vecz::createInterleavedStore`. This avoids the implicit state held by
  `MemOp` from influencing how builtins are defined and requires the user to
  know what they want. Masked accesses set the `Mask` parameter, else the
  operations are unmasked.
* `vecz::MemOp::createInterleavedMemOp` and
  `vecz::MemOp::createMaskedInterleavedMemOp` have been turned into a free
  function and unified under `vecz::getOrCreateInterleavedMemOpFn`.
* `vecz::MemOp::createScatterGatherMemOp` and
  `vecz::MemOp::createMaskedScatterGatherMemOp` have been refactored into two
  separate but unified free functions `vecz::createGather` and
  `vecz::createScatter`. This avoids the implicit state held by `MemOp` from
  influencing how builtins are defined and requires the user to know what they
  want. Masked accesses set the `Mask` parameter, else the operations are
  unmasked.
* `vecz::MemOp::createScatterGatherMemOp` and
  `vecz::MemOp::createMaskedScatterGatherMemOp` have been turned into a free
  function and unified under `vecz::getOrCreateScatterGatherMemOpFn`.
* `vecz::MemOp`s and `vecz::MemOpDesc`s now behave more like read-only data
  structures. The various `MemOpDesc::analyzeXXX` functions are now static and
  return a `llvm::Optional<MemOpDesc>`, rather than populating an existing
  structure. All `MemOpDesc:setXXX` have been removed to better reflect this.
* The legacy `debug::DebugPass` was removed, since the last of the legacy
  passes were transitioned to run on the new pass manager. Those are covered by
  `debug::DebugInstrumentation`.
* When packing kernel arguments, pointers to structure types are no longer
  assumed to be passed by-value. The `byval` parameter attribute now dicates
  this behaviour.
* A new `utils` header - `utils/scheduling.h` - has been added to hold all of
  the work-item and work-group scheduling utilities. The enumerations from
  `utils/passes.h` have been moved there. This utility header also contains
  helper functions to populate lookup work-item and work-group functions and
  acts as a centralized location for the definition of the scheduling structure
  types used by the compiler.
* Scheduling structure parameter indices are now encoded in `!mux_scheduled_fn`
  function metadata by the `utils::AddWorkItemInfoStruct` and
  `utils::AddWorkGroupInfoStruct` passes. This is intended to remove the
  implicit assumptions on parameter indices found in passes such as
  the `utils::ReplaceLocalWorkItemIdFunctions`,
  `utils::ReplaceNonLocalWorkItemIdFunctions` and `host::AddEntryHook` passes
  and permit a more flexible pass order for users.
* Up to two custom parameter indices can be encoded in `!mux_scheduled_fn` for
  target-specific purposes. The `utils::AddKernelWrapper` pass uses this when
  building kernel wrappers and packing up arguments.
* `spirv_ll::Builder`'s `applyMangledLength` now returns a new string (rather
  than mutating a supplied one) to make it more easily usable with known
  constant strings.
* `spirv_ll::getMangledTypeName` now requires a `OpType` when mangling a
  pointer type. This is so it can correctly mangle pointee types when opaque
  pointers are enabled.
* Partial Scalarization is now enabled by default. The `PartialScalarization`
  Vecz Choice has been removed and `FullScalarization` has been added instead.
  Use `FullScalarization` to turn off Partial Scalarization and restore the
  previous default behaviour.
* LLVM's opaque pointers mode is now unconditionally enabled for all LLVM
  versions starting with 15.

Feature additions:

* Add the `ReplaceWGCPass` which provides a software implementation of the
  OpenCL C work-group collective builtins.
* The `PassMachinery` class takes an `llvm TargetMachine` pointer in the
  constructor. By default this can only be known in the derived class, and so to
  support the `TargetMachine` being known throughout the compilation pipeline,
  it is advised to override the `BaseModule::createPassMachinery`, even if only
  to create the `BaseModulePassMachinery` with a known `TargetMachine`.
* Adds the NULL macro to OpenCL 3.0 as required by the standard.
* `vecz` can now remove `ptrtoint` instructions used by binary operators which
  operate on types other than `i8`. A bitcast is issued to the equivalent `i8`
  pointer type before issuing the `i8` GEP.
* Added compiler `host` target lit test suport under
 `modules/compiler/targets/host/test`. Currently only tests one pass
 `AddFPControlPass`. Tests can be run with `check-passes-host-lit` target.
* A new helper function `core::utils::getOrCreateCoreDMAEventType` was added to
  help ensure consistent handling of the `__core_dma_event_t` structure across
  compiler passes.
* Vecz SIMD Width: "Tolerant Impl" is gone and replaced by a simple baseline
  factor of 4. "Avoid Spill Impl" has been improved to mitigate the difference:
  * `vecz::TargetInfo::estimateSimdWidth()` now only considers the total width
    in bits of all the packetizable live values, assuming that the back-end can
    pack several values into one register to avoid spilling.
  * The live values are only considered relevant if they are live within the
    current loop, on the principle that spilling inside a loop would be bad, but
    spilling before a loop may be acceptable.
  * Uniform values used by packetizable values are no longer counted as to be
    packetized themselves, on the reasoning that their broadcast values can be
    sunk towards their uses after packetization.
  * Work Item builtins are no longer considered, since just like broadcasts,
    the vector addition of their subgroup IDs can be sunk after packetization.
* Opaque pointers can now be mangled/demangled. They are mangled as `u3ptr`
  followed by the optional qualifiers and address space. If users wish to
  mangle pointer element types using opaque pointers they must do so in a
  custom manner, though they can use the provided mangling APIs to do so.
* The internal `utils::NameMangler` can now demangle pointer types mangled with
  the address space qualifiers before the const/volatile/restrict qualifiers.
* Support for fp16 has been added to RISC-V targets (with the Zfh extension 
  enabled).
* Added the two argument version of `work_group_barrier()`.
* A new tool `muxc` has been added which allows the running of pass pipelines.
  This can be used for testing for generic utility passes and target specific
  ones. `--print-passes` can be used to show passes available.
* The "Squash Int Vectors Pass" can now squash loads and stores of any type that
  fits into a legal integer, such as `<2 x float>` into an `i64`.
  Correspondingly, it has been renamed to "Squash Small Vectors Pass". It also
  now uses Stride Analysis to check that this transform is really necessary, and
  checks the alignment against the ABI alignment, not the preferred alignment.
* Add the `mux_fence_s` type to the Mux spec. APIs to create, wait on, query,
  reset, and destroy these objects will be added in a future spec revision.
  Implement `hal::fence` and `riscv::fence_s` and use it to make the implicit
  fence in `riscv::command_buffer_s` explicit.
* A helper function `core::utils::copyParameterAttrs` has been added to copy
  parameter attributes between functions.

Non-functional changes:

* A unified lit configuration file - `modules/lit/lit.common.cfg.in` - has been
  added to simplify the addition of a new lit compiler test suite.  This will
  be configured to the build directory as `<build>/modules/lit/lit.common.cfg`
  and can then be included by test suites. It sets up common features and
  substitutions used by ComputeAorta compiler test suites.
* The `riscv::queue_s` implementation is now single-threaded which greatly
  simplifies the implementation.
* The `mux::hal::semaphore` implementation is now a single 32-bit unsigned
  integer atomic and no longer keeps track of command-buffers to signal, this is
  now handled by the `riscv::queue_s`.

Bug fixes:

* Fix a bug in the `cl_command_queue` cleanup of completed
  `mux_command_buffer_t` dispatches where `mux_semaphore_t` objects were being
  prematurely destroyed while still being waited upon by currently running
  dependant `mux_command_buffer_t` dispatches.
* The `alwaysinline` attribute is now more consistently applied to kernels only
  if the `noinline` attribute is not present, as these attributes are
  incompatible.
* `RemoveFences` pass is disabled when compiling with `-cl-std=CL3.0`.
* The fact that an `llvm::Function` is or isn't an intrinsic is now correctly
  preserved when remapping functions in the
  `core::utils::AlignModuleStructsPass`.
* Parameter attributes are now better preserved by middle-end passes and are
  copied over to various wrapper functions.
* Perform internal retain of command-queue on command-buffer construction to
  avoid the command-queue being freed while the command-buffer is still alive.
  This can result in a use-after-free as the command-buffer expects to be able
  to access the queue.
* Move error for passing a mutable-handle to the `clCommandNDRangeKenrnelKHR`
  entry-point earlier. This avoids the handle being created and never returned,
  resulting in a memory leak.
* The `CL_KERNEL_PRIVATE_MEM_SIZE` is now calculated correctly when queried via
  `clGetKernelWorkGroupInfo()`.
* Parameter attributes are now better preserved when cloning functions for
  wrappers.
* Prevent combinatorial explosion during `shouldVectorize()` heuristic, by
  caching intermediate results that might otherwise be recalculated an
  indefinite number of times for extremely complicated expressions.
* The `__OPENCL_VERSION__` macro is now correctly defined when using
  `-cl-std=CL3.0`.

## Version 1.70.0 - 2022-06-07

Upgrade guidance:
* Headers in the `mux/utils` directory have been moved out of the `mux` target
  and into a separate `mux-utils` target, if using these headers update CMake to
  link against `mux-utils`.
* Headers in the `utils` directory have been moved out of the `utils` target
  and into a separate `compiler-utils` target, if using these headers update CMake to
  link against `compiler-utils`.
* `utils::LazyBuiltinLoader` no longer takes or holds an `LLVMContext`. It was
  previously unused.
* `utils::SimpleLazyBuiltinLoader` no longer takes a `Module` on construction.
  It was previously only used to pass a `LLVMContext` to
  `utils::LazyBuiltinLoader`.
* The `multi_llvm` `newLoadInst` APIs now take the load result type separately to the
  pointer type and no longer infers the result type from the pointer type.
* Removed the untested and broken kernel coverage feature. If kernel coverage is
  required, please open a feature request to reinstate it.
*  `vecz::TargetInfo::createLoad`, `vecz::TargetInfo::createMaskedLoad`,
   `vecz::TargetInfo::createInterleavedLoad`,
   `vecz::TargetInfo::createMaskedInterleavedLoad`,
   `vecz::TargetInfo::createGatherLoad` and
   `vecz::TargetInfo::createMaskedGatherLoad` now take the type to load as a
   parameter, rather than inferring it from the pointer type and the
   vectorization factor. Furthermore, `vecz::TargetInfo::createLoad`,
   `vecz::TargetInfo::createMaskedLoad`,
   `vecz::TargetInfo::createInterleavedLoad`,
   `vecz::TargetInfo::createMaskedInterleavedLoad`,
   `vecz::TargetInfo::createGatherLoad`,
   `vecz::TargetInfo::createMaskedGatherLoad`, `vecz::TargetInfo::createStore`,
   `vecz::TargetInfo::createMaskedStore`,
   `vecz::TargetInfo::createInterleavedStore`,
   `vecz::TargetInfo::createScatterStore`,
   `vecz::TargetInfo::createMaskedScatterStore` and
   `vecz::TargetInfo::createMaskedInterleavedStore` no longer take
   vectorization factors as parameters. This information is implicit in the
   types they load/store.
* The `LocalMemoryCalculatePass` has been replaced with a simple query function
  that computes local memory usage of the given module. It no longer requires
  surrounding PassManager infrastructure and can be freely run on an
  `llvm::Module`:
  ```cpp
  #include "utils/pass_functions.h"
  uint64_t local_mem_usage = core::utils::computeLocalMemoryUsage(module);
  ```

Feature additions:

* Targets supporting work-group collectives will now define the feature macro
  `__opencl_c_work_group_collective_functions` in `.cl` files.
* Initial support for using the RefSi HAL alongside the `riscv` Mux target. This
  is done by checking out the `hal_refsi` repo in
  `$CA_SRC_ROOT/modules/mux/targets/riscv/external` and passing the following
  options to CMake: `-DHAL_ENABLE_REFSI=TRUE -DCA_HAL_NAME=refsi`. The device
  name to use for OpenCL testing is `RefSi M1`. Note that `get_group_id()`
  currently always returns zero when the RefSi HAL is used. This is a known
  issue which breaks most kernels.
* Added a tutorial to the overview documentation for creating a new HAL which
  targets Codeplay Reference Silicon. It describes how to implement the various
  HAL operations for RefSi and how to test the new HAL by running clik examples.
* Added toolchain file that allows building static libCL with the host mux
  target to run on aarch64 based eMCOS. This also entails a few non-functional
  changes to the host target iteself (mostly adding another #else to some
  #ifdefs).
* Added the `bool device_s::supports_work_group_collectives` field, see
  `mux-runtime-spec.md` for usage.
* A new set of `multi_llvm` helper APIs has been introduced to deal with opaque
  pointers. See `multi_llvm/opaque_pointers.h`.
* On RISC-V, the features enable by `CA_RISCV_VF` and
  `CA_RISCV_EARLY_LINK_BUILTINS` are now enabled in Release mode by default.
* Added a tutorial to the overview documentation for creating a new Mux target.
  It describes using the script to create the new target, and the how to write
  a pass to change the compilation.
* On RISC-V, vector-predicated code produced by `vecz` is now more optimal
  through the use of `vsetvli` intrinsics to compute the active vector length
  (kernel width).
* The `riscv` target now supports the generation of kernel debug info.
* Introduce `cargo::thread` which wraps `std::thread` and extends it to be able
  to set the thread name, this is helpful while debugging to quickly determine
  the ownership of certain threads by name.
* Update `host::thread_pool_s` to use `cargo::thread` and set the thread names
  to `host:pool:<thread-index>`.
* The `compileOpenCLC` default implementation has been updated to separate out
  the debug printing of OpenCL kernel sources to a separate function,
  `debugDumpKernelSource`. This is intended to make it easier for
  target-specific subclasses of `BaseModule` to override `compileOpenCLC`
  without excessive code duplication.
* The `compileOpenCLC` default implementation has also been updated to move the
  code that prints OpenCL kernel sources to disk while also setting clang's
  codegen options to point to that new file, into a separate function. This is
  similarly intended to make it easier for target subclasses of `BaseModule` to
  override the function without duplicating the routine.
* `BaseModule` has an additional virtual method `createPassMachinery()`. This
  will provide a `PassMachinery` which can be used throughout the pipeline to
  handle state needed for the new pass manager interface. 
* The Vecz Module Pass Manager has been separated out from the Vectorization
  Context. Instead of the Vectorization Context owning a Pass Manager, we now
  construct a Pass Manager directly inside the RunVeczPass and use a builder
  function to populate it.

Non-functional changes:

* Document things that a target backend needs to do or consider when using Vecz.
* Delete some effectively dead code from Vecz Divergence Analysis.
* Performance improvement to the Abacus double precision square root function.

Bug fixes:

* Fix undefined behaviour issue in Vecz Divergence Analysis from iterator use
  after invalidation.
* Fixes RISC-V spike simulator slowdown. An upstream change to the Spike simulator
  was responsible for a large slowdown. This resolves this by taking a later commit
  of upstream Spike.
* The `riscv` target now properly reports the number of elapsed cycles. This
  previously could differ from the number of retired instructions.
* `CA_RISCV_DUMP_ASM` no longer mutates the module it was previewing, meaning
  it should no longer affect the "real" compilation following it.
* Using the `--log-file` (or `-l`) option with CityRunner no longer results in
  duplicate output in the terminal nor an empty log file.
* ROSCC branches are no longer created for a branch that targets multiple
  return paths. DCE is run after Simplify CFG to clean up dangling branch
  condition instructions.
* The `riscv` target now properly reports that performance counters are not
  supported when the loaded HAL device reports no performance counters. It also
  gracefully fails when the user queries for non-existent counters through the
  Mux API.
* Fix the `riscv` target for modes where we know we can vectorize. This happens
  when environment variable `CA_RISCV_VF` is set to `V` which means assume
  vectorization only with no tail loop.

## Version 1.69.0 - 2022-05-04

Upgrade guidance:

* Clarification that `muxUpdateDescriptor` implementations don't need to verify
  the size of POD arguments.
* The `utils::CostModel` interface has been removed. This simplifies the way
  vecz is controlled. In the old implementation there were two interfaces for
  controlling vecz's per-function options, but since the newer
  `VeczPassOptionsAnalysis` allows the user to specify their own hook which can
  directly control more of the vecz optimization parameters (and decide to
  multiply vectorize where appropriate), the old option was removed. If you use
  a custom cost model, you will need to upgrade your pass pipeline to ensure a
  `VeczPassOptionsAnalsis` is constructed with a user function which handles
  your existing cost-modelling. See #Cost Model Interface in
  doc/modules/utils.md for details and an example.
* The `vecz::RunVeczLegacyDestructivePass` has been removed. Legacy behaviour
  must now be implemented by running `vecz::RunVeczPass` followed by
  `vecz::VeczReplaceScalarFunctionsPass`.
 * The legacy version of `core::utils::PrepareBarriersPass` --
   `core::utils::createPrepareBarriersLegacyPass` -- was removed.
* When specifing custom pass pipelines on the `veczc` command line, LLVM
  function passes must be explicitly passed as `function(passname)`.
* The `riscv` target now uses v1.1.0 of the Spike RISC-V ISA simulator.
* The OpenCL sub-group queries are now implemented through ComputeMux Runtime
  and ComputeMux Compiler queries. Any target supporting sub-groups **must**
  implement these new APIs. Any target not supporting sub-groups **must** return
  the appropriate unsupported error code. See the Mux changelogs and the Mux
  spec for the relevant entry points and fields.
* The legacy version of `core::utils::ReplaceCoreDmaPass` --
  `core::utils::createReplaceCoreDmaLegacyPass` -- was removed.
* The `vecz` lit wrapper has been removed and replaced with the generic `ca-lit`
  script.
* The legacy version of `core::utils::OptimalBuiltinReplacementPass` --
  `core::utils::createOptimalBuiltinReplacementLegacyPass` -- was removed.
* Support for LLVM 8 and LLVM 9 has been removed.
* C++14 is now the default C++ mode, upgraded from C++11.
* `GenericPassPipeline::addSnapshotPass` now takes a `llvm::ModulePassManager &`
  rather than a `llvm::legacy::PassManager &`.
* `GenericPassPipeline` only adds the default `scheduled` snapshot if passed
  `1`. This means a single invalid stage no longer appears as a valid one.
* The old `__core_dma` builtins have been extended to include 3D, and the 2D
  builtins have a new function signature to handle a destination and a source
  stride. Targets which provided their own implementations for the `__core_dma`
  builtins should now be updated to enable DMA.
* The legacy version of `core::utils::LinkBuiltinsPass` --
  `core::utils::createLinkBuiltinsLegacyPass` -- was removed.

Feature additions:

* A new function `setDefaultOpenCLLangOpts` is added to `compiler::BaseModule`.
  This function can be called to populate a `clang::LangOptions` object with
  defaults for OpenCL, based on the options set on the module. This function
  should be called after any call to
  `clang::CompilerInvocation::setLangDefaults` so as to avoid any of our
  defaults being overridden.
* A new function `setClangOpenCLStandard` is added to `compiler::BaseModule`.
  This function can be called to simultaneously set the OpenCL version on a
  `clang::LangOptions` object while returning the appropriate matching
  `clang::LangStandard::Kind` to be passed to any calls to
  `clang::CompilerInvocation::setLangDefaults`.
* The `riscv` target now supports the `cl_khr_command_buffer` extension by
  implementing the `muxCloneCommandBuffer` Mux entry-point.
* `vecz` and `veczc` now support vectorizing the same kernel multiple times in
  the same invocation. The `-k` argument has been extended to support naming not
  just a single kernel to vectorize with global parameters, but multiple
  vectorization factor, scalability specs, dimensions, and SIMD widths. e.g.,
  `-k foo:16 -kbar:2,4,8.2@16` vectorizes the `foo` kernel by 16, and the `bar`
  kernel 3 times; once by two, once by four and once by eight on the second
  dimension assuming a local size (SIMD width) of 16.
* A const-qualified accessor has been added to `utils::TypeQualifiers` to allow
  the inspection of qualifiers without modifying the list's state.
* The Vectorization Context now builds a Module Pass Manager instead of a
  Function Pass Manager, so module passes can now be inserted into the pass
  pipeline for the vectorizer.
* A new function `addDefaultOpenCLPreprocessorOpts` is added to
  `compiler::BaseModule`. This function can be called to add default OpenCL
  preprocessor definitions and compile options to the module. When overriding
  `compileOpenCLC`, the clang instance must be populated with these by calling
  `populatePPOpts`, as well as `populateOpenCLOpts` once the clang instance
  target has been set.
* The `riscv` target has been extended to include a number of `std::function`
  entry points which overrides or adds passes in the `GenericPassPipelineConfig`
  class. The RISC-V specific `IRToBuiltinReplacementPass()` is added from
  `info.cpp` now. There should now be no RISC-V specific aspects to the
  `GenericPassPipeline` now. Additional pass/debug names that are desired to be
  used in debugging the pipleline can also be added through the
  `GenericPassPipelineConfig` class. This also removes the `SpikePassPipeline`
  class.
* `vecz` is now able to vectorize the `llvm.fma` intrinsic.
* A LIT convenience wrapper script, `ca-lit`, was added to which can run
  individual tests or test sub-directories from mapped source paths. This is
  more convenient than having to run entire suites via their build-directory
  paths. The `ca-lit` script wraps `lit` and passes all arguments through, so
  all `lit` arguments are supported. It is built and copied to `<build
  dir>/modules/lit/ca-lit`.
* The `OptimalBuiltinReplacementPass` now replaces 64-bit `__abacus_clz` with
  `llvm.ctlz` on all targets except 32-bit ARM.
* Added a tutorial describing how to add a `clmul` extension to a base target.
* The `core::utils::OptimalBuiltinReplacement` pass can now run a list of
  replacement functions customized by users. Users can define
  `OptimalBuiltinReplacement::adjustReplacements` when constructing the pass in
  order to add new replacements or remove any of the default set of
  replacements.
* `spirv-ll` now handles dynamic scopes for the various `OpGroup.*` operations
  required to implement sub-group builtins and work-group collectives in OpenCL.
* An initial version of create_target script to simplify creating new targets.
  This uses cookie cutter templates to create mux and compiler aspects that are
  suitable for a starting point for a new target. Cookie templates are provided
  under `modules/mux/cookie` and `modules/compiler/cookie`.
* The `riscv` target now automatically passes the LLVM backend feature strings
  if running on a RISC-V platform for which the `Zba`, `Zbb`, `Zbc`, `Zbs` or
  `Zfh` extensions are enabled. This enables code generation for any of these
  extensions.
* The old `__core_dma` builtins have been extended to include 3D, and the 2D
  builtins now include a destination and a source stride. Details of the new
  builtins are in the compiler spec.
* The `ReplaceCoreDmaPass` now provides default implementations for 1D, 2D and
  3D copies.
* Support for the `cl_khr_extended_async_copies` extension has been added. When
  enabled, it introduces new `async_work_group_copy_2D2D` and
  `async_work_group_copy_3D3D` builtins. The `ReplaceAsyncCopiesPass` provides
  definitions for these functions in terms of the `__core_dma` builtins.
* A new helper function has been added
  `core::utils::createKernelWrapperFunction` which can be used by customers who
  want to wrap the function created by `AddKernelWrapper`. It is also used by
  this pass.
* The `riscv` target now implements `muxUpdateDescriptors` and supports the
  `cl_khr_command_buffer_mutable_dispatch` OpenCL extension.
* The `clUpdateMutableCommandsKHR` entry-point from
  `cl_khr_command_buffer_mutable_dispatch` now verifies its struct argument.

Non-functional changes:

* Update the `riscv::memory_s` object to be implemented in terms of the new
  `mux::hal::memory` object.
* Update the `riscv::semaphore_s` object to be implemented in terms of the new
  `mux::hal::semaphore` object.
* Host now uses the RunVecz module pass instead of the legacy function pass.
* Metadata for Partial Vectorization and Vector Predication is now set on the
  vectorized kernel insteead of the original scalar kernel.
* Vecz now transfers the kernel entry point metadata from the scalar kernel to
  the vectorized kernel even when using Partial Vectorization. The Barrier Pass
  therefore looks up the scalar kernel from the vector kernel, rather than the
  other way about. This reduces the number of different cases to consider and
  will allow simpler implementation of multiple vectorizations.
* Use an analysis function provided by LLVM instead of a home-grown solution for
  detecting the presence of irreducible control flow.
* Introduce the `mux-hal` library to improve our ability to share code between
  targets utilising the HAL interface.
* Update the `riscv::memory_s` object to be implemented in terms of the new
  `mux::hal::memory` object.

Bug fixes:

* We now check for device `mux_device_info_s::can_clone_command_buffers` support
  before reporting the simultaneous-use capability in the command-buffer
  extension.
* Fix a data race in `compiler::BaseModule::finalize()` present since the
  introduction of `utils::PassMachinery` and related refactoring.
* Vecz: run Break Critical Edges Pass before Control Flow Conversion Pass to
  eliminate certain edge cases during linearization. Restore full functionality
  of the Branch Splitting process during Uniform Reassociation Pass. Allow the
  ROSCC optimization to see through blocks containing only an unconditional
  branch.
* The `core::utils::HandleBarriersPass` was fixed so that it picks vectorized
  functions more often. This case arises when `vecz` is used to vectorize a
  function and partial vectorization is disabled and the original scalar
  function is not replaced. The `HandleBarriersPass` now tries to find a
  vectorized function from the original (via metadata) and if one is found, it
  is used to form the work-item loops. Note that it is assumed that if such a
  vectorized function exists it must be safe to execute, otherwise partial
  vectorization should be enabled or the vectorized function should be deleted.
* Reverting back `riscv` vectorization default to scalar. There has been recent
  fails when we moved to LLVM 14. We believe these are a result of longstanding
  issues with backend vector alignment.
* The `utils::TypeQualifiers` class has been given a stable 64-bit storage
  instead of relying on `size_t`. Its maximum size was also fixed so that
  attempting to push more qualifiers than `MaxSize` into `TypeQualifiers` now
  correctly returns a `false` value from `push_back`.
* Move setting pitch defaults out of validation helper back into the main body
  of the EnqueueCopyBufferRect functions.
* Missing `lld` linker output in LLVM versions 14+ has been restored in the
  `riscv` target.
* Fix numerous data races in LLVM's global state for command-line option
  storage, this state can be dynamically updated at various points in the
  compilation pipeline. To combat this the `utils::getLLVMGlobalMutex()` utility
  should be locked while global state may be modified. Data races were found
  using the ThreadSanitizer.
* The `OptimalBuiltinReplacementPass` has been given a pass ID which was
  erroneously missing. Its pass ID is `replace-builtins`.
* Vecz: fix bug in CFG Conversion that was introduced in the previous release
  where Basic Block masks were inadvertently changed during refactoring.
* The `cl_khr_command_buffer_mutable_dispatch` extension now checks if the Mux
  device supports `mux_device_info_s::descriptors_updatable` before enabling
  extension and using `muxUpdateDescriptors` entry-point.

## Version 1.68.0 - 2022-04-06

Upgrade guidance:

* The `host` target's `host::AddFloatingPointControl` pass (formerly retrieved
  by `host::createAddFloatingPointControl`) was made a non-legacy pass. Its pass
  ID used for debugging is `add-fp-control`.
* The `core::utils::MakeFunctionNameUniquePass` pass (formerly retrieved by
  `core::utils::createMakeFunctionNameUniquePass`) was made a non-legacy
  `Function` pass. Its pass ID used for debugging is `make-unique-func`.
* The `core::utils::RemoveExceptionsPass` pass (formerly retrieved by
  `core::utils::createRemoveExceptionsPass`) was made a non-legacy `Function`
  pass. Its pass ID used for debugging is `remove-exceptions`.
* The `core::utils::PrepareBarriersPass` pass (formerly retrieved by
  `core::utils::createPrepareBarriersPass`) was made a non-legacy pass. Its pass
  ID used for debugging is `prepare-barriers`. A legacy version --
  `core::utils::createPrepareBarriersLegacyPass` -- was introduced to manage the
  transition.
* The `cl_khr_mutable_dispatch` extension has been renamed
  `cl_khr_command_buffer_mutable_dispatch` in line with the most recent revision
  of the draft specification.
* The officially supported version of LLVM for use with the `riscv` target was
  increased from LLVM 13 to LLVM 14.
* The `vecz::Vectorizer` interface and implementation classes have been removed.
  Targets must now use RunVeczPass to perform vectorization. The "vecz/vecz.h"
  header has also been removed, so any indirectly included headers must now be
  included directly.
* The `core::utils::LinkBuiltinsPass` is now a pass that runs in LLVM's new pass
  manager. A legacy version -- `core::utils::LinkBuiltinsLegacyPass` was added
  to manage the transition between pass managers.
  `core::utils::creatLinkBuiltinsPass` was renamed to
  `core::utils::creatLinkBuiltinsLegacyPass`. The new pass's ID used for
  debugging is `link-builtins`.
* `vecz` lit tests are now run from the source directory, rather than from the
  build directory. The entire test suite can be run as before by invoking `lit`
  in the build directory. To run individual tests, a convenience script --
  `ca-vecz-lit` -- was added which can run individual tests or test
  sub-directories from source. It wraps `lit` and passes all arguments through,
  so all `lit` arguments are supported.
* `riscv` now uses a `GenericPassPipeline` which is defined using a
  `GenericPassPipelineConfig`. This reduces references to the `HAL` in the
  `RiscvInfo` class. `GenericInfo` has also been created which provides some of
  the common code.
* The `AutoDmaPass` pass was converted to a pass that runs on LLVM's new
  `PassManager`. Its pass ID used in debugging was changed from `host_sched_dma`
  to `auto-dma`.
* A new `multi_llvm` wrapper -- `multi_llvm/target_registry.h` -- was added to
  account for `llvm/Support/TargetRegistry.h` moving to `llvm/MC` in LLVM 14 and
  beyond. Developers should use this `multi_llvm` header rather than including
  either header explicitly.
* Some leftover parts of the `compiler` interface in ComputeMux used to
  implement the compatibility layer has been removed:
  * `cargo::optional<mux_device_t> device` and `mux_allocator_info_t
    allocator_info` has been removed from `compiler::Info::createTarget`.
  * `compiler::BaseKernel::createSpecializedKernel` has been moved to
    `compiler::Kernel::createSpecializedKernel`.
    `compiler::Kernel::createMuxSpecializedKernel` was an implementation detail
    of `compiler::BaseKernel` which has now been removed.
  * `compiler::SpecializedKernel` has been removed.
* `compiler::BaseTarget` now loads the builtins module for the given builtin
  capabilities as part of `compiler::BaseTarget::init`. Compiler targets should
  implement `compiler::BaseTarget::initWithBuiltins` instead. Unlike `init`,
  `initWithBuiltins` does not need to delegate to `compiler::BaseTarget` first,
  as it's a pure virtual function.
* The notification callback passed to `compiler::Target::init` is now passed to
  `compiler::Info::createTarget` and is now of type
  `compiler::NotifyCallbackFn`. This should be passed along to
  `compiler::BaseTarget`'s constructor.
* The HAL submodule has been moved from `riscv` target and now resides under
  module/mux/external.
* The `core::utils::BarriersPass` (formerly retrieved by
  `core::utils::createHandleBarriersPass`) was made non-legacy and renamed
  `core::utils::HandleBarriersPass`. Its pass ID used for debugging is
  `barriers-pass`.
* The `core::utils::AlignModuleStructsPass` pass was moved from `utils/passes.h`
  to `utils/align_module_structs_pass.h`.
* The `core::utils::OptimalBuiltinReplacementPass` pass and its legacy version
  `core::utils::createOptimalBuiltinReplacementLegacyPass` were moved from
  `utils/passes.h` to `utils/optimal_builtin_replacement_pass.h`.
* The Vectorization Choice `NoDoubleSupport` has been removed. The
  `RunVeczLegacyDestructivePass` now takes a boolean on construction to support
  this feature. The `new RunVeczPass` for the new pass manager now takes a
  `vecz::TargetInfo` on construction instead of a `utils::BuiltinInfo` and
  `llvm::TargetMachine`.
* Some `vecz` command-line options have been removed:
  `-vecz-control-flow-conversion`, `-vecz-no-early-cse`,
  `-vecz-no-pre-lineraization`, `-vecz-no-middle-optimizations`,
  `-vecz-no-post-optimizations`, `-vecz-no-cleanup`,
  `-vecz-no-preparation-pass`. Users wanting more control over the pipeline
  should use `-vecz-passes` instead.
* The `-vecz-target-independent-packetization` command-line option was removed
  and was made a 'choice' instead: use
  `-vecz-choices=TargetIndependentPacketization`.
* The `core::utils::MaterializeAbsentWorkItemBuiltinsPass` was made a non-legacy
  pass. Its pass ID used for debugging is `missing-builtins`.
* The `core::utils::ReplaceCoreMathDeclarationsPass` (formerly retrieved by
  `core::utils::createReplaceCoreMathDeclarationsPass`) was made non-legacy. Its
  pass ID used for debugging is `replace-core-math-decls`.
* The `core::utils::ReduceToFunctionPass` was made a non-legacy pass, and the
  previous legacy pass was renamed to `core::utils::ReduceToFunctionLegacyPass`.
  The legacy pass's creation method was similarly renamed to
  `core::utils::createReduceToFunctionLegacyPass`. The non-legacy version's pass
  ID used for debugging is `reduce-to-func`.
* The `core::utils::AddWorkItemFunctionsIfRequiredPass` (formerly retrieved by
  `core::utils::createAddWorkItemFunctionsIfRequiredPass`) was made a non-legacy
  pass. Its pass ID used for debugging is `add-reqd-work-item-funcs`.
* The `core::utils::FixupCallingConventionPass` (formerly retrieved by
  `core::utils::createFixupCallingConventionPass`) was made a non-legacy pass.
  Its pass ID used for debugging is `fixup-calling-conv`.
* The RISC-V `TakeSnapshotPass` was converted to a shared `utils` pass that runs
  on LLVM's new `PassManager`. It was additionally made into a generic callback
  pass: `utils::SimpleCallbackPass`. A legacy version --
  `utils::SimpleCallbackLegacyPass` -- which can continue to run on LLVM's
  legacy `PassManager` was introduced to manage the transition between pass
  managers. The pass is given a generic callback function which is invoked with
  the module when the pass is run. The `riscv` and `host` targets use this pass
  to take snapshots.
* CMake now errors if a user enables the `cl_khr_mutable_dispatch` extension but
  not `cl_khr_command_buffer` too, as this is a dependency.
* `riscv` snapshots now use `compiler::BaseModule::SnapshtDetails` to share as
  much code as possible. The old `riscv::Snapshot` has been removed.
* Removed the following Vectorization Choices, which were previously enabled by
  default:
  * AllScatterGatherAsMasked
  * AllInterleavedAsMasked
  * MaskedInterleavedAsMaskedScatterGather Override creator functions in
    Vecz::TargetInfo if optimal implementations are available for the target.
* `riscv::PassPipeline` now *adds* early passes to a pass manager and runs them
  later. `riscv::PassPipeline::runEarlyPasses` has been renamed to
  `riscv::PassPipeline::addEarlyPasses` to reflect this.
* The `riscv` target now contains `GenericModule` and `GenericTarget` classes,
  with the intention that these will be moved to a common place in the future.
  These use an abstract class PassPipeline which can be used in conjunction with
  these classes.
* Vecz Snapshots have been removed. Use standard LLVM instrumentation to obtain
  debug prints of IR between vecz passes, such as `-print-after-all`.
* The `core::utils::ReplaceCoreDmaPass` pass was changed to one which runs on
  LLVM's new pass manager. A legacy version --
  `core::utils::ReplaceCoreDmaLegacyPass` -- was introduced to manage the
  transition between pass managers. The pass ID of the new pass, used for
  debugging, is `replace-core-dma`.
* The `riscv` target's `SpikeReplaceCoreDmaPass` pass was moved to one which
  runs on LLVM's new pass manager. No legacy replacement was provided. Its pass
  ID used for debugging was changed from `riscv_replace_dma`  to
  `spike-replace-core-dma`.
* All but one of the vecz command-line options used to 'enable' passes have been
  removed. All uses can be substituted with the `-vecz-passes` option. The only
  remaining option -- `-vecz-control-flow-conversion` -- is deprecated. Users
  should use `-vecz-passes` for more specific and robust testing.
* The `core::utils::AddWorkItemInfoStructPass` (formerly retrieved by
  `core::utils::createAddWorkItemInfoStructPass`) was made a non-legacy pass.
  Its pass ID used for debugging is `add-work-item-info`.
* The `core::utils::ReplaceLocalWorkItemIdFunctionsPass` (formerly retrieved by
  `core::utils::createReplaceLocalWorkItemIdFuncsPass`) was made a non-legacy
  pass and renamed to `core::utils::ReplaceLocalWorkItemIdFuncsPass`. Its pass
  ID used for debugging is `replace-work-item-id-funcs`.
* The `core::utils::AddWorkGroupInfoStructPass` (formerly retrieved by
  `core::utils::createAddWorkGroupInfoStructPass`) was made a non-legacy pass.
  Its pass ID used for debugging is `add-work-group-info`.
* The `core::utils::CreateNonLocalWorkItemFunctionsPass` (formerly retrieved by
  `core::utils::createReplaceNonLocalWorkItemFunctionsPass`) was made a
  non-legacy pass and renamed to `core::utils::ReplaceNonLocalWorkItemFuncPass`.
  Its pass ID used for debugging is `replace-non-local-work-item-funcs`.
* The `host` target's `host::AddEntryHookPass` (formerly retrieved by
  `host::createAddEntryHookPass`) was made a non-legacy pass. Its pass ID used
  for debugging is `add-entry-hook`.
* The `core::utils::AddKernelWrapperPass` (formerly retrieved by
  `core::utils::createAddKernelWrapperPass`) was made a non-legacy pass. Its
  pass ID used for debugging is `add-kernel-wrapper`.
* The `core::utils::ReplaceLocalModuleScopeVariablesPass` (formerly retrieved by
  `core::utils::createReplaceNonLocalModuleScopeVariablesPass`) was made a
  non-legacy pass. Its pass ID used for debugging is
  `replace-module-scope-vars`.
* The `riscv` target's `riscv::AddElfMetadataPass` (formerly retrieved by
  `riscv::createAddElfMetadataPass`) was made a non-legacy pass. Its pass ID
  used for debugging is `add-elf-metadata`.
* The `riscv` target uses the `vecz::RunVeczPass` pass to vectorize, rather than
  the `vecz::RunVeczLegacyDestructivePass` version.
* The `compiler::PassSoftwareDivision` pass was made a non-legacy pass. Its pass
  ID used for debugging is `software-div`. A legacy version --
  `compiler::LegacyPassSoftwareDivision` -- was introduced for legacy users.
* The `compiler::PassImageArgumentSubstitution` pass was made a non-legacy pass.
  Its pass ID used for debugging is `image-arg-subst`.
* The `core::utils::ReplaceAsyncCopiesPass` (formerly retrieved by
  `core::utils::createReplaceAsyncCopiesPass`) was made non-legacy. Its pass ID
  used for debugging is `replace-async-copies`.
* The `core::utils::ReplaceAtomicFunctionPass` (formerly retrieved by
  `core::utils::createReplaceAtomicFunctionPass`) was made non-legacy and
  renamed `core::utils::ReplaceAtomicFuncsPass`. Its pass ID used for debugging
  is `replace-atomic-funcs`.
* The `core::utils::ReplaceC11AtomicFunctionPass` (formerly retrieved by
  `core::utils::createReplaceC11AtomicFunctionPass`) was made non-legacy and
  renamed `core::utils::ReplaceC11AtomicFuncsPass`. Its pass ID used for
  debugging is `replace-c11-atomic-funcs`.
* The `core::utils::ReplaceBarriersPass` (formerly retrieves by
  `core::utils::createReplaceBarriersPass`) was made non-legacy. Its pass ID
  used for debugging is `replace-barriers`.
* The `compiler::PassMemToReg` pass was made a non-legacy *function* pass. Its
  pass ID used for debugging is `ca-mem2reg`.
* The `compiler::PassBuiltinSimplification` pass was made non-legacy. Its pass
  ID used for debugging is `builtin-simplify`.
* The `compiler::PassPrintfReplacement` pass was made non-legacy. Its pass ID
  used for debugging is `replace-printf`.
* The `compiler::PassCombineFPExtFPTrunc` pass was made a non-legacy *function*
  pass. Its pass ID used for debugging is `combine-fpext-fptrunc`.
* The `compiler::PassCheckForDoubles` pass was made a non-legacy *function*
  pass. Its pass ID used for debugging is `check-doubles`.
* The `host` target's `DisableNeonAttributePass` pass (formerly retrieved by
  `host::createDisableNeonAttributePass`) was made a non-legacy pass. Its pass
  ID used for debugging is `disable-neon-attr`.
* The `core::utils::RemoveLifetimeIntrinsicsPass` pass (formerly retrieved by
  `core::utils::createRemoveLifetimeIntrinsicsPass`) was made a non-legacy pass.
  Its pass ID used for debugging is `remove-lifetimes`.
* The `core::utils::RemoveFencesPass` pass (formerly retrieved by
  `core::utils::createRemoveFencesPass`) was made a *function* non-legacy pass.
  Its pass ID used for debugging is `remove-fences`.
* The `riscv` and `host` targets internally use `llvm::PassBuilder` to build
  their standard per-module middle-end optimization pipeline, rather than the
  legacy `llvm::PassManagerBuilder`.
* A helper class to introduced to manage the lifetime and initialization of
  LLVM's new pass machinery: `utils::PassMachinery`. It can be extended to add
  custom callbacks and register custom passes.
* A new set of debug instrumentations are available for use with passes running
  on LLVM's new `PassManager`:
  * See various instrumentations in `debug/passes.h`, e.g.,
    `debug::PrintPassNameInstrumentation`.
  * Debug support for `CA_DUMP_IR_PASS` is not available on any passes running
    on LLVM's new `PassManager`.
  * Debug support for `,<iteration>` arguments is not available on any passes
    running on LLVM's new `PassManager`.
* The `riscv` target now sets the RISC-V target-specific triple and `DataLayout`
  via the `utils::SimpleCallbackPass`, executing it as a pass, rather than
  in-line after flushing the pass manager.
* The `riscv` `IRToBuiltinsReplacementPass` was converted to a pass that runs in
  the new pass manager. Its pass ID used for debugging changed from
  `riscv_ir_to_builtins` to `ir-to-builtins`.
* The `vecz` `VerificationPass` has been removed; the standard LLVM
  `VerifierPass` is used instead.
* `vecz`: the `InlinePostVectorization` no longer takes `utils::BuiltinInfo` on
  constructon, instead retrieving it from the common
  `VectorizationContextAnalysis`.

Feature additions:

* If the user does not specify a kernel to vectorize (via `-k`), `veczc` will
  vectorize all kernel functions in the module.
* If the user specifies a function to vectorize that doesn't exist in the
  module, `veczc` will report an error. Previously it would report that it
  "failed to vectorize" the function.
* The `core::utils::HandleBarriersPass` adds metadata to scalar work-item loops
  to prevent LLVM's inner-loop vectorizer from performing further vectorization.
* The RunVeczPass now only constructs a single `VectorizationContext`, to be
  used for all vectorization during the Module pass. Vecz Choices are now owned
  by the Vectorization Unit, to facilitate the use of different Choices per
  function in future.
* `spirv-ll` now correctly emits calls to the `sub_group_barrier` builtin when
  the execution scope of the `OpControlBarrier` builtin is `Subgroup`.
* The `riscv` target now enabled vectorization by default, unless
  `-cl-wfv=never` is supplied. Its default vectorization factor is equivalent to
  passing `CA_RISCV_VF=1,S`, or the minimum-possible scalable factor.
* Introduce a SPIR-V extended instruction set, *Codeplay.GroupAsyncCopies*, to
  represent the builtins defined in the Khronos OpenCL extension
  `cl_khr_extended_async_copies`. This SPIR-V extension is not production ready,
  and should be considered a prototype pending an official solution from the
  Khronos OpenCL working group.
* Vecz: `VectorizationContext::canVectorize()` is now implemented in an analysis
  pass.
* All entry-points to the `cl_khr_command_buffer` extension now verify the
  arguments passed by the user according to the specification, and return the
  appropriate error codes.
* All MemOp creation functions in Vecz::TargetInfo have been regularized and now
  support Vector Predication.
* All passes that run on LLVM's new `PassManager` are registered with
  `-print-after=` and `-print-before=` on LLVM versions above 12.

Non-functional changes:

* The VectorizationContext is now responsible for the cloning of functions to be
  vectorized, and deleting them on failure, not the VectorizationUnit.
* Add a section on control processors to the overview documentation.
* The `SpikeReplaceCoreDmaPass` pass now uses an updated version of `hal_spike`
  and the Spike DMA interface, which will enable full 2D and 3D transfers in a
  future version.
* Refactor out confusing `VectorizationContext::createKernel()` function.
* Remove some dead code from Uniform Value Analysis. Remove the
  `runUniformValueAnalysis()` function and use Analysis Manager to get the
  result instead.
* Add regression test for varying LCSSA PHI note handling during Divergence
  Analysis. Fix LIT tests for Mask Varying loads.
* Add `cargo::function_ref`, a lightweight non-owning reference to a callable.
  This can be used in place of `std::function` whenever you don't need to own
  the callable, such as passing a callback as an argument which isn't stored
  anywhere.
* Vecz: Use llvm/ADT containers (maps, sets) instead of stl containers in all
  Control Flow related analyses and transform passes/functions. Replace
  BlockQueue and LoopQueue with more efficient alternatives.
* Vecz: Control Flow Conversion only creates the BOSCC object when BOSCC is
  actually being used.
* Vecz: Control Flow Analysis and Divergence Analysis have been refactored to
  remove interdependencies. Divergence Analysis is now responsible for holding
  the Basic Block Tags and the Loop Tags, and for computing the Dominance
  Compact Block Ordering. Control Flow Analysis now only determines whether any
  control flow conversion is necessary and possible.
* Vecz: Moved state related to Control Flow Linearization out of the Divergence
  Analysis Result, and store it locally in the Cotrol Flow Conversion Pass
  instead.
* Refactored the use of Vectorization Units for kernels and builtins. Removed
  dead code for packetization of builtins.
* Vecz: Control Flow Conversion now stores block exit/entry masks internally,
  instead of in the Divergence Analysis Result.
* Added a high level overview of the ComputeMux Compiler to the introduction of
  the ComputeAorta Overview documentation.

Bug fixes:

* An LLVM verification failure was fixed in which `core` barrier builtins were
  previously being created with parameter attributes for parameters they didn't
  have.
* A crash was fixed in `core::utils::HandleBarriersPass` pass that occurred when
  the `IsDebug` flag was passed to the pass while the `Module` contained
  vectorized kernels.
* `spirv-ll` now correctly handles group ops that map to sub-group builtins in
  OpenCL.
* Vecz Divergence Analysis no longer modifies the Uniform Value Analysis.
* Vecz Divergence Analysis no longer marks loads in divergent blocks as varying.
  This became unnecessary with the introduction of the "mask varying" attribute.
* `vecz` now better handles the case when packetization produces a constant
  value.
* `VECZ` now vectorizes subgroup reductions and broadcasts when the input is
  uniform across the subgroup.
* Fixed an issue where the corresponding ComputeMux Compiler for a given device
  would not be detected correctly when setting the CMake option
  `CA_COMPILER_ENABLE_DYNAMIC_LOADER` to `ON`, resulting in an offline-only
  OpenCL driver.
* The `OpMemoryBarrier` instruction now generates a call to a wrapper function
  in the IR with equivalent semantics. This fixes an edge case where introducing
  control flow directly into the current function produces invalid IR.
* `CA_PASS_PREFIX` no longer enables all debug instrumentations even if they
  weren't specified via environment variables: `CA_PASS_PREFIX` must always work
  in conjunction with one or more other `CA_PASS_*` options.
* `CA_PASS_VERIFY` and `CA_PASS_VERIFY_DUMP` are enabled by default in the new
  pass manager, as they are with the legacy pass manager.
* The flags argument to barriers is now correctly extracted from the semantics
  argument of the `OpControlBarrier` instruction rather than the memory scope
  field.
* `vecz` will no longer leave around invalid PHIs on packetization failure,
  leaving around a valid `llvm::Module` under any scenario.
* Fixed a never-actually-triggered bug where copying a value from one key of a
  DenseMap to another could in theory trigger a rehashing and invalidate one of
  the references before reading/writing the copied value.

## Version 1.67.0 - 2022-03-01

Upgrade guidance:

* The `out` parameter offered by the `RunVeczPass` pass has been removed. Targets
  should instead retrieve vectorization information via metadata in the Module.
* The RISC-V finalizer was modularised, see the new `PassPipeline` class.
* The `VectorizationContext` no longer maintains a cache of declared internal
  builtins. Instead, it uses the `Module` as the ground-truth. All functions in
  the `Module` which begin with the internal vectorization prefix string are
  considered "builtin".
* The vectorizer no longer checks whether it `canVectorize` a module when
  running a custom pass pipeline. The check only makes sense on pre-vectorized
  IR and often fails on IR that is mid-way through the vectorization pipeline.
* References to the old Codeplay subgroups extension have been renamed to more
  accurately reflect their meaning and intended purpose:
  * `mux_device_info_t::max_subgroup_size` has been renamed to
    `mux_device_info_t::max_work_width`.
  * `compiler::Kernel::getDynamicSubgroupSize` has been renamed to
    `compiler::Kernel::getDynamicWorkWidth`.
* vecz lit tests now check for both i32 and i64 insertelement and
  extractelement index types to better match LLVM 14's default choice of i64.
* The `BuiltinInfo::getSubgroupLocalIdBuiltin()` function now returns the
  invalid builtin ID when none exists, rather than the unknown builtin ID.
* The `AlignModuleStructsPass` pass was converted to a pass that runs on LLVM's
  new `PassManager`.
* The RISC-V `TakeSnapshotPass` was converted to a pass that runs on LLVM's new
  `PassManager`. A legacy version -- `TakeSnapshotLegacyPass` -- which can
  continue to run on LLVM's legacy `PassManager` was introduced to manage the
  transition.
* The `host` target now uses the new PM to manage its loop/slp optimizations.
* A new piece of `Function`-level metadata has been introduced --
  `!codeplay_ca_kernel`  -- which singles out specific kernel functions under
  compilation. It has one operand: an `i32` which is either 1 if the kernel is
  also an "entry point" or 0 otherwise.
* A new piece of `Function`-level metadata has been added to track work-item
  orders: `!work_item_order`.
* Several passes are no longer added and run once per function, and are instead
  added and run once per module: `HandleBarriersPass`, `AutoDmaPass`,
  `MakeFunctionNameUniquePass`, and `host`'s `AddFloatingPointControlPass`.
* Several passes no longer take kernel names on construction, instead using
  `!codeplay_ca_kernel` entry points: `host`'s `AddFloatingPointControlPass`,
  `riscv`'s `AddElfMetadataPass`, and the common `PrepareBarriersPass`,
  `HandleBarriersPass`, and `MakeFunctionNameUniquePass` passes.
* Several passes no longer take kernel names on construction, instead using
  `!codeplay_ca_kernel` metadata: `ReduceToFunctionPass`,
  `FixupCallingConventionPass`, `ReplaceLocalWorkItemIdFunctionsPass`,
  `AddKernelWrapperPass`, `ReplaceLocalModuleScopeVariablesPass`.
* The `ReduceToFunctionPass` pass now operates on `!codeplay_ca_kernel`
  metadata by default. The old version which works on an explicit list of names
  is still available, though is deprecated.
* The formerly vecz-specific `vecz_metadata.h` header in `utils` was renamed to
  `metadata.h` and now also contains general metadata helper functions.
* A new pass -- `EncodeKernelMetadataPass` -- was introduced, which is intended
  to be run very early. It adds `!codeplay_ca_kernel` entry point,
  `!work_item_order`, and `!reqd_work_group_size` `Function` metadata.
* The `AddReqdWorkGroupSizeMetadataPass` was removed in favour of the
  `EncodeKernelMetadataPass`.
* The `HandleBarriersPass` now takes work-item orders from `!work_item_order`
  metadata rather than on pass construction.
* The `AutoDmaPass` now writes out the required DMA sizes to `!dma_reqd_size`
  Function metadata, rather than to an out parameter in C++. The `host`
  target's `CreateEntryHookPass` pass receives this information via metadata
  rather than upon pass construction. The `AutoDmaPass` takes local sizes and
  work-item orders from metadata, too.
* Use of `getElementType` on `llvm::PointerType` has been removed in favour of
  `getPointerElementType` as a temporary workaround. It is strongly encouraged
  for new code to specify the pointee type explicitly rather than relying on
  the pointer type.
* A new `multi_llvm` typedef has been added -- `AlignIntTy` -- to help manage
  the underlying integer type of LLVM's various alignment data structures,
  which changed in LLVM 14 from `uint32_t` to `uint64_t`.
* The Core API and specification has been removed in this release. Any remaining
  Core targets contained either in-tree in `modules/core/source` or elsewhere
  must now be ported to the ComputeMux API. See
  [ComputeMux Runtime Specification](doc/modules/mux/runtime-spec),
  [ComputeMux Compiler Specification](doc/modules/mux/compiler-spec) and the
  [migration guide](doc/modules/mux/migrating-from-core).
* A number of interfaces for using the vectorizer have significantly overhauled
  how the vectorizer works, and breakingly deprecates a number of legacy
  interfaces in favour of new versions. The old versions have been renamed to
  indicate that we will frown at you if you continue to use them.
    - `createRunVecPass` -> `createRunVeczLegacyDestructivePass`
      The old `createRunVeczPass` would overwrite the original scalar kernel,
      and need to be created once per kernel. This prevented multiple
      vectorizations as the scalar kernel was deleted.
    - `createRunVeczLegacyDestructivePass` maintains this old behaviour, but you
      should transition to the new `RunVeczPass` with the necessary custom
      analyses to selectively vectorize kernels in which you are interested when
      you are ready. A future patch will remove the legacy interfaces (TBD).
* `utils::CostModel` query functions now take `llvm::Function *` instead of
  string name of function.
*  The metadata query interface, `parseOrigToVeczFnLinkMetadata` is now
   overloaded to optionally return a a list of vectorization factors/kernels.
   The old interface still exists and operates as before, but in use asserts
   that only one vectorization is present to ensure upgrades have been handled
   completely.
* New members were added to a couple of Mux structs:
  `mux_device_info_s::max_hardware_counters` and
  `mux_query_counter_s::hardware_counters`. These should at least be initialized
  to sensible defaults by any Mux implementation implementing query counter
  support.

Feature additions:

* Vecz is now able to vectorize/widen most allocas, avoiding unnecessary
  scatter/gather operations and accessing data contiguously instead (or
  strided/interleaved access). Scalable vectorization factor is partially
  supported.
* A new set of `multi_llvm` helpers have been introduced in
  `multi_llvm/attribute_helper.h` to deal with the management of parameter and
  return attributes.
* A vecz lit test script -- `pp-llvm-ver.py` -- has been added to make managing
  FileCheck checks across different LLVM versions easier.
* Vecz: a new analysis pass has been created, the Packetization Analysis, which
  traverses the function from its vector leaves through their operands, marking
  instructions for packetization where required. Not all varying instructions
  will need to be packetized, for instance the address operands of memory
  operations don't need to be packetized if they have a linear stride. Moving
  this code into its own pass allows other transform or analysis passes to use
  this information in a clean way, without having to store or modify state. For
  instance, the SIMD Width Analysis needs this information, prior to
  packetization.
* Legality of VP intrinsics are now checked beforehand to make sure we
  do not need splitting, not present in LLVM < 14.
* The packetizer can now packetize `insertelement` instructions
  without going through memory in the RISCV target.
* Legality of RISCV intrinsics are now checked manually instead of
  relying on `TargetLowering`.
* Implement `cl_khr_command_buffer` entry-points recording image commands to
  a command-buffer: `clCommandFillImageKHR`, `clCommandCopyImageKHR`,
  `clCommandCopyImageToBufferKHR`, and `clCommandCopyBufferToImageKHR`.
* The `cl_khr_command_buffer` extension entry-point `clCommandNDRangeKernelKHR`
  now verifies its arguments according to the specification.
* VECZ now correctly vectorizes the `sub_group_any`, `sub_group_all`,
  `get_max_sub_group_size`, `sub_group_broadcast`, and
  `get_enqueued_num_sub_groups` builtins when vectorizing subgroups. These also
  work when vectorizing with vector predication.
* VECZ now correctly vectorizes the subgroup scan operations when vectorizing
  subgroups. These also work when vectorizing with vector predication.
* Vecz: OffsetInfo no longer creates any `Instruction`s or `ConstantInt`s
  during analysis, since we can deduce whether a value has a linear dependence
  on the work item ID without knowing the actual value of the stride, which is
  now only computed if it is a known integer. Actual stride values can be made
  manifest afterwards, if required, by calling the `manifest` method. This
  allows us to make decisions based on contiguity of memory access during
  analysis passes.
* `multi_llvm::LegacyPMAdapterMixin`, simplifies wrapping new passes to be run
  by the `legacy::PassManager`.
* `utils::createSimpleLazyCLBuiltinInfo` now provides a less-verbose way to
  construct a `utils::BuiltinInfo` from a builtins module and an executable
  module.
* Vecz: a new analysis pass has been created, the Stride Analysis, which
  computes linear strides of pointer expressions used by memory operations.
  Stride-related code has been moved out of Uniform Value Analysis into
  this new analysis, and the old mechanism by which unnecessary stride
  instructions would be created temporarily and then deleted, has been
  removed.
* The host target now fully implements performance counter support through the
  Mux query pool interface. This support can be enabled by having PAPI installed
  on your system and enabling the `CA_HOST_ENABLE_PAPI_COUNTERS` CMake flag.
  Note that for now this only works on Linux systems due to support for
  measuring on worker threads being platform specific.
* Added Mux struct members `mux_device_info_s::max_hardware_counters` and
  `mux_query_counter_s::hardware_counters`, with accompanying spec language
  describing their function.
* The RISC-V ComputeMux target implementation has been updated to disable
  deferred compilation and produce subgroup information in the generated ELF.
  This means that subgroup tests will work for precompiled binaries. This also
  changes the maximum workgroup size to be the same as the work group maximum.
* Vecz/OffsetInfo:
  * Can determine memory stride of index expressions involving Or/Xor/And in
    more cases, and can now add two strided values together.
  * Don't create "Offset" expressions since these are no longer used.
  * Simplify the "Flags" enum to make Offsets easier to reason about correctly.
  * Use a bitmask to keep track of possibly-set bits instead of keeping track of
    left/right shift amounts.
* Implement the `cl_khr_mutable_dispatch` extension entry-point
  `clGetMutableCommandInfo`, which was previously a stub.

Non-functional changes:

* Moved all compiler types in `host` to the `host` namespace.
* Moved all compiler types in `riscv` to the `riscv` namespace.
* Update the Vecz documentation to reflect current state of the vectorization
  process.
* Document new analysis passes, Stride Analysis and Packetization Analysis.
* `veczc` now uses the new PassManager internally

Bug fixes:

* Fixed an issue where spurious random characters were sometimes printed after
  dumped IR when using `CA_RISCV_DUMP_IR`.
* `spirv-ll` now correctly handles the `get_max_sub_group_size`,
  `get_num_sub_groups` and `get_enqueued_num_sub_groups` builtins.
* Fixed an issue in the RISC-V target where `-cl-wfv=never` was not taking
  precedence over the `CA_RISCV_VF` environment variable.
* Vecz/OffsetInfo: Fix UB in bitmask computation of i64 types.

## Version 1.66.1 - 2022-02-25

Upgrade guidance:

* The ComputeMux Runtime has loosened lifetime requirements for target
  implementations:
  * If a `mux_kernel_t` was created from a `mux_executable_t`, it is guaranteed
    that the `mux_kernel_t` will be destroyed before the `mux_executable_t`.
  * If a `mux_executable_t` was created from the result of calling
    `compiler::Kernel::createSpecializedKernel`, it is guaranteed that the
    `mux_executable_t` will be destroyed before the `compiler::Kernel`.
* Added an upgrade guide describing the steps needed to migrate a Core target
  to ComputeMux.

Bug fixes:

* Fixed a regression in performance when compiling OpenCL or Vulkan programs at
  runtime when using the `host` ComputeMux target.
* `-cl-vec=all` was fixed to enable both the `-cl-vec=slp` and `-cl-vec=loop`
  pre-vectorization optimizations as designed.
* Move the CMake logic that adds the compiler definitions for host's target CPU
  option from the Mux implementation into the compiler implementation, where it
  belongs.
* Fixed an issue where multiple calls to `clBuildProgram` on the same program
  object would not update the executable when using a ComputeMux compiler that
  does not support deferred compilation.
* Restore CMake options to Mux target that were dropped during the port from
  Core, this includes fixing a regressed test.

## Version 1.66.0 - 2022-02-01

Upgrade guidance:

* Host is no longer a Core target, so "host" should no longer be passed to
  `CA_CORE_TARGETS_TO_ENABLE`. Instead, when explicitly enabling targets "host"
  should now be passed to `CA_MUX_TARGETS_TO_ENABLE`.
* RISC-V is no longer a Core target, so "riscv" should no longer be passed to
  `CA_CORE_TARGETS_TO_ENABLE`. Instead, when explicitly enabling targets "riscv"
  should now be passed to `CA_MUX_TARGETS_TO_ENABLE`.
* `add_mux_compiler` has been renamed to `add_mux_compiler_target`.
* The meaning of the `COMPILER_INFO` argument in `add_mux_compiler_target` has
  now changed. It should now be set to the fully-qualified name of a
  free-standing function of the following signature:
  `void getCompilers(compiler::AddCompilerFn add_compiler)`.
  * See "Enumerating `compiler::Info`'s" in the
    [ComputeMux Compiler Specification](doc/modules/mux/compiler-spec) for more
    information.
* `compiler::Kernel::createSpecializedKernel` has been renamed to
  `compiler::Kernel::createMuxSpecializedKernel` and now takes a `mux_device_t`
  and `mux_allocator_info_t` as additional arguments. Compiler implementations
  should instead now implement a new and simpler function
  `compiler::BaseKernel::createSpecializedKernel`, which instead returns a
  binary containing the specialized kernel function.
* `compiler::Info::supports_runtime_compilation` has been renamed to
  `compiler::Info::supports_deferred_compilation`.
* `compiler::Info::createTarget` now takes a `compiler::Context` as an argument,
  and the context argument has been removed from
  `compiler::Target::createModule`.
* The `mux_kernel_s::sub_group_size`, `core_scheduled_kernel_s::sub_group_size`
  members and the `compiler::Kernel::getSubGroupSize()` method have been added.
  For targets supporting sub-groups these properties should be implemented to
  return appropriate values for a given kernel. Note that users of these
  properties currently assume sub-group size is static i.e. it does not depend
  on local enqueue size, that sub-groups are always in the x-dimension and that
  the sub-groups doesn't depend on any runtime properties of the device.
* Optional `max_work_dim` metadata has been added to the function which takes an
  i32 as the only element which indicates the highest work item dimension used.
  This metadata will be produced if SPIR-V fed to `spirv-ll` contains the
  `OpExecutionMode` `MaxWorkDimINTEL`. Release candidate ASP versions of
  `computecpp` will produce this if `-femit-dimension-metadata` is contained in
  the command line.
* The ``max_work_dim`` metadata allows the compiler to optimize out work-item
  scheduling loops in dimensions higher than the known maximum.
* The `core::utils::HandleBarriersPass` is no longer given kernels'
  vectorization factors. Rather, it infers them from metadata produced by the
  vectorizer itself. The `core::utils::createHandleBarriersPass` function has
  dropped the associated parameter.
* The `uint32_t max_num_sub_groups` and `bool sub_groups_support_ifp` fields
  have been added to the `core_device_info_s` and `mux_device_info_s` structs.
  A target that does not support sub-groups **must** set `max_num_sub_groups =
  0` and `sub_groups_support_ifp = false`.
* The vectorizer's `ControlFlowConversionPass` now replaces the function with
  `unreachable` on failure to help ensure the Module is always in a verifiable
  state.
* The vectorizer's `MiddleOptsPass` was removed. The passes contained within
  were bubbled up to be run on the vectorizer's top-level PassManager instead.
* The vectorizer's `CleanupPass` was removed. The passes contained within were
  bubbled up to be run on the vectorizer's top-level PassManager instead.
* The vectorizer's `InlinePostVectorizationPass` is now a new-style pass and
  its API has changed accordingly.
* The vectorizer's `InstructionCombinePass` was removed. The passes contained
  within were bubbled up to be run on the vectorizer's top-level PassManager
  instead.
* The vectorizer's `BuiltinInliningPass` is now a new-style pass and its API
  has changed accordingly.
* The vectorizer's `BasicMem2RegPass` is now a new-style pass and its API
  has changed accordingly.
* The vectorizer's `LoopRotatePass` has been renamed to `VeczLoopRotatePass`,
  is now a new-style pass, and its API has changed accordingly.
* The vectorizer's `PreparationPass` was removed. The passes contained within
  were bubbled up to be run on the vectorizer's top-level PassManager instead.
* The vectorizer's `InstCombinePass` proxy wrapper was removed as it was hiding
  an implicit conversion which was exhibiting unexpected behaviour. The
  vectorizer now relies on the default settings of LLVM's stock `InstCombinePass`.
* The VectorizationUnit no longer owns or manages the PassManager and its pass
  pipeline; The responsibility for this has been moved into the
  VectorizationContext.
* All Core and Mux targets will only be enabled if *both*
  `CA_MUX_TARGETS_TO_ENABLE` and `CA_CORE_TARGETS_TO_ENABLE` is set to `""`.
* CityRunner now accepts the `--color={auto,always,never}` mode flag to control
  when color escape sequences are printed. The `--no-color` option has been
  removed, `--color=never` should be used instead.
* Bump mutable-dispatch extension implementation to Revision 8 of draft
  specification.

Feature additions:

* Implement the `clGetCommandBufferInfoKHR` and
  `clCommandBarrierWithWaitListKHR` entry-points from the
  cl_khr_command_buffer extension.
* The compiler now emits assumptions into the IR that the local sizes are never
  0 in any dimension. This helps LLVM produce more optimal code, especially
  around work-item loops.
* The "riscv" Core implementation has now been split into a ComputeMux Runtime
  implementation and a Compiler implementation. The riscv implementation that
  used to live in `modules/core/source/riscv` has been removed.
* The packetizer can now packetize vector splats without going through
  memory in the RISCV target.
* OpenCL 3.0 builds now support the `clGetKernelSubGroupInfo` entry point. The
  implementation queries the device for sub-group support but currently assumes
  the trvial case i.e. sub-group = work-item if the device reports support.
* The "host" Core implementation has now been split into a ComputeMux Runtime
  implementation and a Compiler implementation. The host implementation that
  used to live in `modules/core/source/host` has been removed.
* OpenCL 3.0 builds now support the optional sub-groups feature. The initial
  implementation is naive and has a sub-group width of one, meaning a sub-group
  is just an unvectorized work-item. Tests have been added that for the time
  being, assume this behavior.
* A new vectorizer 'choice' was added: VectorPredication. This choice will
  instruct the vectorizer to produce a vector-predicated kernel safe to run on
  any work-group size, even those smaller than the vectorization width.
* The RISC-V target can now pass `VP` to `CA_RISCV_VF` to control vector
  predication, e.g., `CA_RISCV_VF=1,S,VP`. It is disabled by default.
* The vectorizer can vector-predicate kernels consisting of binary operations
  and masked and unmasked memory accesses: loads, stores, scatters, gathers,
  strided accesses. Other operations that do not exhibit side effects are
  supported in this mode, but are left unpredicated. Simple boolean reductions
  are supported but first sanitize their inputs so that out-of-bounds elements
  don't influence the result.
* VECZ now correctly vectorizes `sub_group_reduce_add`, `sub_group_reduce_min`,
  `sub_group_reduce_max` builtins when vectorizing for non-degenerate
  subgroups. All data types are supported, except for floating-point types in
  min/max reductions. These also work when vectorizing with vector predication.
* `get_sub_group_id`, `get_sub_group_local_id`, `get_sub_group_size`, and
  `get_num_sub_groups` now return the correct values for non-degenerate
  subgroups.
* The `utils::OptimalBuiltinReplacement` pass can now replace `__abacus_fmin`
  and `__abacus_fmax` builtins with LLVM `llvm.minnum` and `llvm.maxnum`
  intrinsics, respectively. These improve the performance of `clamp` and
  `smoothstep` builtins, in addition to the `fmin` and `fmax` builtins.
* The packetizer is able to splat scalable vectors by a fixed factor. This
  allows it to successfully scalably packetize in more cases.
* The packetizer is now able to packetize varying vector loads and stores when
  scalably vectorizing.
* The packetizer does a better job of re-splatting existing (uniform) splats,
  leading to better codegen.
* Added platform toolchain support for RISC-V 64 Linux
* OpenCL 3.0 builds will now query the underlying `mux_device_info_s` for
  sub-group support rather than hard coding sub-groups to off.
* Define the sub-group barrier builtins as nops. This should be valid for the
  trivial case that sub-group == work-item and for any implementation that does
  not support IFP (e.g. VECZ/WFV) since the operations will happen in lockstep
  regardless.
* Added `scalable_vector_support` flag to the `compiler::Info` data structure,
  indicating whether or not the compiler supports generating code using scalable
  vectors.
* Add OpenCL example application using the sub-groups feature.
* Adding the following snippet to the `lit.local.cfg` file in a target-specific
  test directory disables all tests if the target is not available:
  ```
  if not 'TARGET' in config.root.targets:
      config.unsupported = True
  ```
* The packetizer no longer falls back on instantiation for mask-varying vector
  instructions when the mask value is a splat value.
* The vectorizer now more accurately tracks and obeys store alignment.
* The packetizer is now able to scalably vectorize splat-like shuffles.
* A new command-line option `-debug-vecz-pipeline` was introduced, allowing
  introspection of the vecz PassManager.
* A new command-line option `-vecz-passes` was introduced, allowing
  a customisable pass pipeline during vectorization. Both LLVM and
  vecz-specific passes are supported. The syntax is that of LLVM's `opt` tool,
  e.g., `-vecz-passes=scalarize,packetizer,view-cfg`
* Support for `-print-after <pass>` and `-print-before <pass>` options was
  added on LLVM versions 12 onwards.
  * Handle the `properties` parameter to `clCreateCommandBufferKHR` in the
    cl_khr_command_buffer extension.
* Added `scalable_vectors` flag to the `compiler::Options` data structure,
  indicating whether or not the executable should be finalized with scalable
  vectors.
* The barriers pass now encodes `codeplay_ca_wrapper` metadata on kernels it
  produces, which can be used to inspect how kernels produced by various
  middle-end optimizations have been combined to form a given kernel.
  * Report the following missing `cl_khr_command_buffer` entry-points, which
    were unimplemented but are now stubs: `clCommandBarrierWithWaitListKHR`,
    `clCommandCopyImageKHR`, `clCommandCopyBufferToImageKHR`, `clCommandCopyImageToBufferKHR`,
    `clCommandFillImageKHR`.
  * `cl_khr_command_buffer` implementation now supports the
    `CL_DEVICE_COMMAND_BUFFER_REQUIRED_QUEUE_PROPERTIES_KHR` flag.
* The packetizer can now packetize `extractelement` instructions
  without going through memory in the RISCV target.
* Add OpenCL example application using the mutable-dispatch extension
  layered on-top of `cl_khr_command_buffer`.
* `core::utils::createLoop` has been updated to take a list of induction
  variables which the user can manage during loop execution.

Non-functional changes:

* Vecz: Uniform Value Analysis remove redundant function
* The Vectorization Unit no longer owns a TargetTransformInfo object. Instead,
  it is queried from the Vectorization Context by function.
* Construct the Uniform Value Analysis directly when checking if a function can
  be vectorized, instead of getting it from the function analysis manager, to
  avoid having to temporarily set the vectorization unit for the scalar
  function.
* The code that decides whether a kernel is worth vectorizing is moved out of
  the Vectorization Unit and into its own class, Vectorization Heuristics.
* Vectorization passes now consistently preserve VectorizationUnitAnalysis and
  VectorizationContextAnalysis, avoiding needless recompoutations.
* The ScalarizationPass and ControlFlowConversionPass no longer make use of the
  Vectorization Unit.
* All `createXXXPass(PassManager &)` pass creation methods have been removed in
  favour of consistently using the pass constructors directly, mirroring LLVM's
  own style.
* Vecz Memory Operation creation functions only need the Vectorization Context,
  not the Vectorization Unit.
* Change default for `cl_intel_unified_shared_memory` USM extension from `OFF`
  to `ON`. USM will now be available in the released ComputeAorta binaries.
* The Vectorization Unit private methods that facilitate the creation of the
  vector kernel from the scalar kernel have been turned into free functions,
  which now reside in a `vectorization_helpers.h` and corresponding .cpp file.
* Vecz: Uniform Value Analysis: Separate computation of pointer base and stride
  value into two functions.
* Vecz: Return OffsetInfo structs by value.
* Vecz: Move pointer analysis code from Uniform Value Analysis into OffsetInfo
* Vectorization statistics held by the Vectorization Unit have been moved into
  the Vectorization Context.
* Deal with the packetization of any function arguments before packetizing
  instructions, so they don't have to be special-cased later on. This also
  removes the burden of instantiating arguments from the instantiator, which
  enables the complete removal of all dependence on the Vectorization Unit
  from the Instantiator.
* The Packetizer now holds onto a copy of the vectorization dimension, instead
  of querying the Vectorization Unit every time.
* Vectorization Unit no longer has a method to access the Vecz Choices from its
  Vectorization Context.
* Created a new struct to hold only the minimal amount of data needed to work
  with the result of a vectorization of a builtin function, to replace the
  local usage of additional Vectorization Units in the packetizer when
  packetizing builtins.
* The Offset Info stride computation need not keep track of the Work Item ID
  call instructions so this data has been removed from the struct.
* The CFG Analysis Result does not need a reference to the VU.
* Continuing to eliminate reliance on the Vectorization Unit by making the
  packetizer take a reference to the Vectorization Context at construction
  instead of getting it from the Vectorization Unit every time.
* Remove the `VectorizationUnit::deleteInstructionNow()` function, which is
  already implemented in the `IRCleanup` class.

Bug fixes:

* Fix the Core compatibility layer not copying the outputs for some
  `muxGetSupportedQueryCounters` requests.
* The `builtins` module now correctly incorporates ComputeMux target builtin
  capabilities when deciding what combinations to build.
* The `OpAtomicExchange` SPIR-V operation is now translated to the correctly
  mangled llvm function in `spirv-ll`.
* UnitVK's couterpart to UnitCL's regression 19 memcpy test has been renamed in
  line with the fix applied to UnitCL, the two had gotten out of sync.
* The `check-cross-ComputeAorta` target is no longer dependant on
  `check-UnitCore`.
* `spirv-ll` will no longer attempt to generate calls to the LLVM frem
  instruction. This is to avoid adding a relocation for various cmath mod
  functions and keep the SPIR-V path in line with the OpenCL C path by always
  calling the abacus builtin for these operations.
* UnitCL's offline kernel generation CMake will now generate offline kernels
  for both Mux and Core targets, this is a temporary measure but a necessary
  one.
* `clCreateKernel` no longer returns `CL_OUT_OF_HOST_MEMORY` if used with a
  ComputeMux Compiler that does not support deferred compilation.
* Instructions must be marked for packetization before running the Simd Width
  Analysis in order for the liveness-based analysis to function. Otherwise it
  will revert to the result of the widest common data type analysis. This is
  a patch to restore previous behaviour pending the proper fix to the Uniform
  Value Analysis.
  * Our `cl_khr_command_buffer` extension implementation now supports printing
    from device-side `printf` in kernels recorded to a command-buffer. This
    capability was previously reported by our implementation but broken.
* Fix various bugs in the UnitCore and UnitMux tests for query counters.
* Fix possible undefined behavior in a UnitCL test for 
  `clEnqueueMigrateMemINTEL`.
* The following platform extensions are now correctly returned by
 `clGetDeviceInfo` with the `CL_DEVICE_EXTENSIONS` argument when enabled:
  * `cl_codeplay_extra_build_options`
  * `cl_codeplay_kernel_exec_info`
  * `cl_codeplay_soft_math`
  * `cl_khr_create_command_queue`
  * `cl_khr_icd`
* Correct the trivial implementation of the `sub_group_scan_exclusive` builtins
  to return the identity for the given operation.
* Fix `CA_RISCV_DUMP_ASM` environment variable dumping assembly more than once
  on the `riscv` target.

## Version 1.65.0 - 2021-12-07

Upgrade guidance:

* `compiler::Context` is now ready for use immediately after calling
  `compiler::createContext()`, and `compiler::Context::init` has been removed.
* `compiler::Target::init` and `compiler::Target::listSnapshotStages` now
  returns a `compiler::Result` instead of a `bool`, allowing more concise status
  codes to be returned on failure.
* `compiler::BaseTarget` no longer requires a `cargo::optional<mux_device_t>`
  and `mux_allocator_info_t` to be passed to its constructor.
* `compiler::available` has been replaced with `compiler::getCompilerForDevice`.
  The resulting `const compiler::Info *` object should be passed to
  `compiler::createContext` and `compiler::createModule` to select the compiler
  implementation to use.
* The following members of `mux_device_info_s` have been moved to
  `compiler::Info`:
  * `compilation_options`
  * `vectorizable`
  * `dma_optimizable`
* Removed `mux_device_type_compiler` as it is no longer relevant now that
  the compiler library exists. `~mux_device_type_compiler` can now safely be
  replaced with `mux_device_type_all`.
* `compiler::BaseModule`'s constructor no longer requires a
  `mux_allocator_info_t` or `cargo::optional<mux_device_t>`.
* `core::utils::RunVeczPass` and `core::utils::VeczPassOptions` were moved into
  `vecz`. Users of that pass or struct should include `vecz/pass.h`
* `vecz::VectorizationFactor` was moved to `core::utils`. Users of that struct
  should include `utils/vectorization_factor.h`.
* `vecz`'s mangling framework (`NameMangler`, `Lexer`, `TypeQualifier`,
  `TypeQualifiers`) were moved into `utils`. Users of that functionality should
  include `utils/mangling.h`.
* The overload of `utils::NameMangler::demangleName` which retrieves argument
  types and qualifiers changed its API. It would previously return `bool` (true
  on success) and modify its first in/out parameter with the demangled name. It
  now returns the demangled name directly, returning an empty string on
  failure. The first input parameter is now taken by value.
* Targets that are currently not making use of the `ReplaceCoreDmaPass` will
  need to define the new `__core_dma_write_1D` and `__core_dma_write_2D`
  builtins in order to support the `__local -> _global` async builtins. These
  `__core` builtins are equivalent to the already existing write variants
  except they DMA in the opposite direction `__local -> __global`.
* ComputeMux runtime targets have moved from `modules/mux/source` to
  `modules/mux/targets`.
* `compiler::loadLibrary` now returns `std::unique_ptr<Library>`, and
  `compiler::unloadLibrary` has been removed. Destroying or resetting the
  `std::unique_ptr` object will now unload the library.
* `compiler::Info::createTarget` and `compiler::Target::createModule` now return
  the result wrapped in a `std::unique_ptr`.
* `mux/muxConfig.h` has been renamed to `mux/config.h`.
* `mux/muxSelect.h` has been renamed to `mux/select.h`.
* The command-buffer extension is now reported as `cl_khr_command_buffer`
  rather than `cl_codeplay_command_buffer`, any existing application code
  will need to be modified to check for the KHR extension.
* Provisional release of `cl_khr_command_buffer` contains the following
  API changes from previous internal version:
  * Reordered `clEnqueueCommandBuffer` arguments.
  * Removal of `INFO` from command-buffer query enums.
  * `cl_ndkernel_command_properties_khr` is renamed
    `cl_ndrange_kernel_command_properties_khr`.
  * New `CL_COMMAND_BUFFER_FLAGS_KHR` and `cl_command_buffer_flags_khr` symbols.
  * Additional of `CAPABILITY` to `cl_device_command_buffer_capabilities_khr`
    enums.
* `compiler::createModule` has been replaced with the `createModule` method in
  `compiler::Info`.
* Internal header `CL/cl_khr_command_buffer.h` has been removed and
  the OpenCL-Headers bumped to include now public definitions of
  the provisional `cl_khr_command_buffer` extension.
* Internal header `CL/cl_khr_mutable_dispatch.h` has been created to
  contain definitions of unratified extension. This may be used to
  prototype applications using this extension but is not guaranteed
  to be stable.
* `vecz`'s `BuiltinInfo` framework was moved into `utils`. Users of that
  functionality should include `utils/builtin_info.h`. Additionally,
  `CLBuiltinInfo` was moved to `utils/cl_builtin_info.h` and `DXILBuiltinInfo`
  to `utils/dxil_builtin_info.h`.
* The `vecz`'s `BuiltinInliningPass` no longer replaces builtins with inline
  IR (it still inlines certain intrinsics in vectorizer-specific ways). The
  responsibility for that specific optimization was transfered to the
  `utils::OptimalBuiltinReplacement` pass. Users should schedule this pass to
  run before invoking the vectorizer.
* The vectorizer now uses a `llvm::FunctionPassManager` for most of its
  "new-style" internal passes. The `run` functions of such passes have thus
  changed to the more standard `run(llvm::Function &,
  llvm::FunctionAnalysisManager &)`. The old `VectorizationUnit &` parameter
  can now be retrieved via the new `VectorizationUnitAnalysis` analysis "pass".
* The `VectorizationContext` for a function can now be retrieved via the new
  `VectorizationContextAnalysis` pass.
* Vecz passes now automatically, implicitly skip after failures by using a
  `PassInstrumentation` callback function. This is replaces the old method of
  explicitly checking for failure in their `run` methods. If any vecz pass
  should not be skipped, it may implement `static bool isRequired()` and return
  true.
* `compiler::Kernel::getUnderlyingCoreKernel()` and
  `MuxKernelWrapper::getUnderlyingCoreKernel()` now returns the `core_kernel_t`
  as a `void*`, and should be casted to `core_kernel_t` if this method is
  used in an OpenCL extension.
* The `UniqueOpaqueStructs` pass has been added which can be used to remap
  opaque structure types in a module. Any target linking together two
  `llvm::Module`s, or deserializing an `llvm::Module` in a different context to
  that in which it was created may need to use this pass in order to remap
  suffixed opaque structure types to their unsuffixed equivalents. See the core
  host implementation for details of how to use this pass.

Feature additions:

* The `compiler::Info` data structure has been added to the ComputeMux Compiler
  API. An instance of `compiler::Info` uniquely identifies a compiler for a
  ComputeMux device. The list of all available compilers can be retrieved using
  the `compiler::compilers()` function.
* Update `cl_intel_unified_shared_memory` to version 1.0.0 which
  includes implementing deprecated entry-point `clEnqueueMemsetINTEL`
  as well as checking for new properties
  `CL_MEM_ALLOC_INITIAL_PLACEMENT_DEVICE_INTEL` and
  `CL_MEM_ALLOC_INITIAL_PLACEMENT_HOST_INTEL`.
* New Vecz pass "Uniform Reassociation Pass", that rearranges expressions
  involving both uniform and varying values in order to minimise the number
  of instructions that require packetization and the number of vector splats.
  It will also split up branches into separate uniform and varying conditions,
  in order to take better advantage of partial linearization.
* The `async_work_group_copy`, `async_work_group_strided_copy` and
  `wait_group_events` builtins are now mapped to `__core` builtins rather than
  being hard coded to a naive CL C implementation. This allows targets to make
  use of hardware specific DMA functionality to implement the builtins. Targets
  that cannot support the `__core` builtins need only run the
  `ReplaceCoreDmaPass` to provide the naive implementation in their compiler.
* Update `cl_khr_command_buffer` extension to provisional release version
  0.9.0, which became public as part of OpenCL spec release 3.0.10.
* Implement command-buffer mutability as a separate
  `cl_codeplay_mutable_dispatch` extension layered on `cl_khr_command_buffer`
  and defaults to OFF.
* Verify arguments to ComputeMux compiler APIs and add negative testing to
  `UnitCompiler` to verify this.
* The vectorizer is now able to packetize the `llvm.ctlz`, `llvm.cttz`,
  `llvm.ctpop`, `llvm.sadd.sat`, `llvm.uadd.sat`, `llvm.ssub.sat`, and
  `llvm.usub.sat` intrinsics.
* ComputeMux compiler (`modules/compiler`) now exposes the CMake function
  `add_mux_compiler` to add new compiler targets, similar to `add_mux_target` in
  the ComputeMux runtime. In-source compiler targets should be added to the
  `modules/compiler/targets` directory, and should depend on the `compiler-base`
  CMake target.
* Added `CA_MUX_COMPILERS_TO_ENABLE` CMake option to override the list of
  compilers that is enabled.
* Added `CA_EXTERNAL_MUX_COMPILER_DIRS` CMake option to specify external
  ComputeMux compiler targets that should be included in the compiler build.
  * Under `PartialScalarization`, the vectorizer no longer scalarizes
    vector-to-scalar bitcast instructions. This can improve performance
    dramatically.
* The `utils::OptimalBuiltinReplacement` pass can now replace `add_sat` and
  `sub_sat` builtins with LLVM `llvm.[us]add.sat` and `llvm.[us]sub.sat`
  intrinsics.
* More of the vectorizer's internal passes register correctly with the standard
  instrumentations, allowing, e.g., `-print-after-all` to provide more
  coverage.

Non-functional changes:

* The packetizer stores the value of the vectorization factor on construction,
  so that it doesn't need to query the Vectorization Unit all the time.
* Scalarization Threshold has been removed.
* VectorizationContext::needsScalarization() moved into Scalarization Pass
  implementation.
* Changed the work item ordering in the barrier pass when using Partial
  Vectorization with barriers so that it matches the implementation when there
  are no barriers, which is the ordering described in the documentation, i.e.
  all the vectorized work items are done first, followed by all the scalar work
  items.
* The `vecz::InstantiationAnalysis` class has been refactored into the
  free-standing function `vecz::needsInstantiation`.
* `buildAfter` utility method of the Vectorization Unit is turned into a free
  function and moved into packetization_helpers.h, and also into
  control_flow_conversion.cpp in an anonymous namespace.
* The functionality to "delete instructions later" is moved out of the
  Vectorization Unit into a new utility class, and the `deleteInstructionsPass`
  is removed, with instruction deletion now the responsibility of the passes
  that require it.
* The two separate implementations of the Wrapper Function generation
  (`MakeWrapperFunction` and `MakeNoBarriersWrapperFunction`) have been merged
  into a single function that handles both cases (i.e. barriers present, and no
  barriers present).
* The Core compatibility layer will now be disabled if no Core targets are
  available.
* Moved the `OptimalBuiltinReplacement` pass from `host` to `utils`, so it can 
  be used by other targets.
* Avoid using `FAIL_IF` in `CLBuiltinInfo`.
* Task 13_01/13_02 Magic Square UnitCL tests no longer override the default
  loop vectorization option

Bug fixes:

* Don't split branches inside innermost loops, to avoid confusing BOSCC
  (temporary patch until BOSCC can be fixed to handle this case).
* Fix some bugs in the ComputeMux Core compatibility layer
  * Check for null pointer before dereferencing
  * Check for updatable descriptors feature before accessing data
  * Use correct translation from Core to Mux error code in
    `muxCloneCommandBuffers()`
* The Vectorization Context's internal cache of vectorized builtin calls now
  allows the caching of vector versions of the same scalar builtin with
  different vectorization factors.
* Correct `async_workgroup_copy` and `async_workgroup_strided_copy` builtin
  implementations so that the size in bytes passed to the `__core` dma builtin
  is aligned to the type being copied. Fixes a failing `half3` DMA test where
  the size of `half3` elements in memory was incorrectly calculated as `3 *
  sizeof(half)` rather than `4 * sizeof(half)` as specified by the OpenCL C
  spec.
* Fix bug when enabling debug and partial vectorization is used, the loop debug
  was not taking the scalar functions debug for the vector function. This fixes
  UnitCL test CA_EXTRA_COMPILE_OPTS=-g $PWD/bin/UnitCL --gtest_filter=*magic_square*/xyz
* Where an alloca was packetized, any remaining uses of the original scalar
  alloca are now replaced with the first element of the packetized alloca.
* Fix bug in `CL_PROFILING_COMMAND_START` on kernel execution commands when
  the `cl_intel_unified_shared_memory` extension is enabled. This fixes a
  CTS fail in `conformance_test_profiling execute`.
* Fix for barrier pass issue where kernels call other kernels, resulting in work
  item wrapping happening on inner calls. This is fixed by not replacing
  function references where it is called.
* New implementation of code to identify BOSCC regions, that fixed an edge case
  revealed by a rarely-run test configuration.
* The vectorizer currently replaces `llvm.memset` or `llvm.memcpy` with its own
  versions. However if the alignment is set to less that the alignment needed
  for a 64 bit int, then the current replacement is inadequate. The fix is to
  not do this replacement for this case.
* Vectorizer passes now more correctly track the analyses they preserved or
  invalidated.
* Fix bug when debug is enabled and a kernel is calling another kernel. This fixes
  UnitCL test `Execution.Regression_86_Store_Local` where
  `CA_EXTRA_COMPILE_OPTS="-g"`.
* `MuxKernelWrapper::getUnderlyingCoreKernel` will now correctly return the
  underlying `core_kernel_t` if the `MuxKernelWrapper` was created with an
  offline kernel, and the Mux target is implemented using the compatibility
  layer.
* Fixed a compile error generated when assigning an rvalue reference to a
  `cargo::expected<T, E>` where `T` is move-assignable but not copy-assignable.
* Vecz: Fix handling of pointer PHI nodes during stride analysis
* Added some missing CMake library dependencies.
* Fixed alignment of parameters in unpacked argument structs, used by the
  `riscv` target.
* Added `memmove` to ELF relocations list in `host` to fix an issue caused by
  changes in LLVM 13+.
* `PrintfExecution.Printf_10_print_nan` in UnitCL now allows the `printf`
  implementation to add padding when formatting a fixed width positive number to
  align with a negative number.
* UB in the `clGetProgramInfoInvalidProgramTest` test suite inside UnitCL has
  been fixed.
* Vecz OffsetInfo Stride computation now correctly returns no stride instead
  of a zero stride for nonlinear index computations, so that scatter/gather
  operations are correctly generated where necessary.

## Version 1.64.0 - 2021-11-02

Upgrade guidance:

* `compiler::createTarget` now takes an optional `mux_device_t` as an argument,
  which is later passed to `Module`s created by the `Target`. As a result,
  `compiler::Target::listSnapshotPasses` and `compiler::Target::createModule`
  no longer need to be passed a device.
* OpenCL kernels and most builtins are no longer marked convergent by default.
  This should be [NFC], but may cause issue with certain function attributes in
  lit testing, requiring minor edits to the tests.
* LLVM 8 has now been demoted to "soft-deprecated" status, and now undergoes
  less frequent testing. Remaining users of LLVM 8 are advised to begin
  migration to a newer version.

Feature additions:

* The vectorizer now checks with the target whether each `llvm::Value` is legal
  to be packetized before doing so. Targets may implement
  `TargetInfo::canPacketize` in order to force a kernel not to be vectorized.
  RISC-V uses this hook in order to prevent the generation of scalable vector
  types which the LLVM code generator is unable to successfully compile.
* Scalar integer `convert_*` builtin functions are now converted to native LLVM
  cast instructions during the Builtin Inlining Pass of Vecz Preparation,
  working around an issue that prevented identification of memory operation
  contiguity.
* The RISC-V target now supports snapshots.

Non-functional changes:

* Added support for building against LLVM 11.1.
* When generating the work item wrapper function over a kernel that contains
  barriers, a while loop is no longer generated and individual loops over
  sub-kernel invocations now jump directly to their successors instead.
* Enabled WARN_AS_ERROR when running Doxygen.
* Suppress a warning about passing the address of an uninitialized variable to
  another function when compiling with GCC 11.0 to 11.2.
* Image support in `host` is now disabled by default. Set the CMake variable
  `CA_ENABLE_HOST_IMAGE_SUPPORT` to `ON` to enable it.
* Introduce usage of `cl_context` notification callback to report human-readable
  information to the user upon creation. This enabled reporting platform level
  errors such as failing to dynamically load the compiler library.
* Added the following sections to the ComputeMux Compiler specification:
  * `compiler::Module`
  * `compiler::Kernel`
  * Details on `__core` builtins.
  * Atomics and Fences.
* Improved the ComputeAorta Overview documentation:
  * Reworked the floating-point requirements.
  * Added the "Integer and Floating-point Operations", "Debug Info", "DMA", and
    "Types" sections to "Compiler Information > Intermediate Representation".
  * Added the "Alignment" section to "Compiler Information > Compiler
    Requirements".
  * Added ComputeMux Compiler Non-Requirements.
  * Moved the "Vectors", "Intrinsics", and "Alignment" sections from "Compiler
    Information > ComputeMux Compiler Backend" to "Compiler Information >
    Intermediate Representation".
  * Added an introduction to "Compiler Information > Intermediate
    Representation", "Compiler Information > ComputeMux Compiler Backend", and
    "Mapping ComputeMux to Hardware".
  * Added more detail to the "Intrinsics", "Address Spaces", and "Atomics and
    Fences" sections of "Compiler Information > Intermediate Representation".
  * Added the "How To Support Large Scratchpad Memories", "Mapping Algorithms To
    Vector Hardware", "Mapping Custom Instructions To Builtin Functions", and
    "Mapping Custom IP Blocks To Builtin Kernels" pages to "Example Hardware
    Feature Scenarios".
  * Added a quick start for creating a new ComputeMux Target.
  * Restructured the "Synchronization Requirements", and "Drivers Requirements"
    pages.
  * Rewrote the "Mapping ComputeMux to Hardware > Memory Requirements" section.
* Remove references to MIPS from the codebase, which has long been unsupported.

Bug fixes:

* Update `cl_platform` initialization to propagate error codes instead of
  aborting when an error occurs.
* Allocas are now correctly handled in the barrier struct when using a scalable
  vectorization factor.
* Fixed subtle edge cases in part of the Control Flow Conversion pass with a
  new, more robust implementation of the Dominance Compact Block Indexing.
* Weak atomic `cmpxchg` operations are now correctly marked as weak in LLVM.
* Fixed an issue where the LLVM flag `--riscv-v-vector-bits-min` would only
  be passed to the first compilation in the RISC-V finalizer.
* The naive software implementation of OpenCL C's
  `async_work_group_strided_copy` scatter operation is now only run on thread
  with local ID (0,0,0), matching the behaviour of the corresponding gather
  operation.

## Version 1.63.0 - 2021-10-05

Upgrade guidance:

* The `corePushBarrier`/`muxCommandBarrier` entry points have been removed from
  the Mux spec and headers. All references to these entry points should be
  removed from existing Core/Mux implementations.
* Updated `cl_khr_command_buffer` extension headers to revision 24.
* Move the compiler front-end implementation from `compiler::CompatModule`
  into `compiler::BaseModule`, so future compiler target implementations can
  make use of it.
* `clGetProgramBuildInfo` will now return `CL_BUILD_SUCCESS` if the `cl_program`
  was created with `clCreateProgramWithBinary`.
* Rename `mux_error_t` to `mux_result_t`.
* Rename `mux_error_success` to `mux_success`.
* Rename `mux_error_malformed_parameter` to `mux_error_invalid_value`.
* Moved the ComputeMux compiler implementation from `source/compiler` to
  `modules/compiler` for consistency with the ComputeMux runtime.

Feature additions:

* `oclc` now accepts `-help` as an argument, in addition to `--help` and `-h`.
* Mux targets can now be added in the same manner as Core targets using the
  `add_mux_target` CMake function. Entry points in `mux.h` will now be able to
  select Mux targets, whilst existing Core targets will continue to go through
  the compatibility layer.

Non-functional changes:

* Added documentation on:
  * Compiler Requirements, including vectorization, intrinsics, alignment and
    address spaces.
  * Atomic Requirements
  * Mux's execution model (as part of the Mux runtime specification)
  * How to expose specific hardware features
* Wrote an initial draft of the ComputeMux compiler specification. Note that
  this is still incomplete and a work in progress.
* `mux.h` is now generated by the `mux-api-generate` target, and is no longer
  based on `core.h`. The Core compatibility layer therefore implements all
  objects as shims, rather than relying on typedefs.
* VECZ lit tests now specify all of their code generation options via per-test
  command-line flags, rather than inheriting some via the global testing
  environment.

Bug Fixes:

* The `compiler` CMake target will no longer generate a link error when
  `source/cl` has been removed.
* OpenCL 3.0 now compiles correctly when linking against LLVM 13.
* If `add_ca_unitcl_check` is called in CMake with the `COMPILER` argument,
  UnitCL will only run `/(Compile|Link|Build|Execution|print)` tests.
* The `corePushBarrier`/`muxCommandBarrier` entry points, which had been
  rendered useless by a previous change to the Core spec, have been removed.
* Barriers in control flow that appears divergent will now cause a vectorization
  failure.

## Version 1.62.2 - 2021-09-24

Feature additions:

* Barrier pass is now able to remove low-cost GEPs and certain simple
  computations based on work item functions from the barrier struct, to reduce
  the barrier overhead.
* Added packetization support for the `abs` and `copysign` LLVM intrinsics.
* Enabled scalable vectorization for `insertelement` and `extractelement` LLVM
  instructions.

Bug fixes:

* Fixed an issue where a segfault would occur in some cases where not all 
  barriers would get deleted properly during the barrier pass.

## Version v1.62.1 - 2021-09-14

Feature additions:

* Barrier pass supports scalable vectors.

Bug fixes:

* Barrier struct is now deterministic.
* Fixed a failing `cl_codeplay_wfv` UnitCL test using the `riscv` target.
* Fixed an issue where some `.bin` files used by offline UnitCL tests on Windows
  were not being built properly for some targets.

## Version 1.62.0 - 2021-09-07

Upgrade guidance:

* `CL-offline` and other offline targets have been removed. This means that it
  is no longer possible to build different versions of `CL` both with and
  without a statically linked runtime compiler in the same build. The CMake
  option `CA_ENABLE_OFFLINE_LIBRARIES` and function `add_core_offline_target`
  has also been removed as a result.
* `utils::PrepareBarriersPass` must be run before Vecz for PartialVectorization
  to work properly with barriers, and in any case before `HandleBarriersPass`.
* The OpenCL extension `cl_codeplay_subgroups` has been removed in this release.
  Vector information is instead exposed through the new extension
  `cl_codeplay_wfv`.
* The `-cl-auto-vectorize` OpenCL option has been removed, as it is no longer
  used.
* `compiler::ParseOptionsResult`, `compiler::CompileResult`,
  `compiler::LinkResult` and `compiler::FinalizeResult` have been merged into
  a single enum type `compiler::Result`.
* `muxDestroy*` and `muxFreeMemory` entry points no longer return a
  `mux_error_t`, to better match the behaviour of `libc`'s `free()` and C++
  destructors.
* Updated `cl_khr_command_buffer` extension headers to revision 23.
* As `muxCreateExecutable` can no longer be used to create built-in kernels,
  `muxCreateBuiltInKernel` should be used instead.

Feature additions:

* Added support for LLVM 14.
* It is now possible to link `CL` against a dynamically linked version of the
  compiler called `libcompiler.so` when `CA_COMPILER_ENABLE_DYNAMIC_LOADER=ON`
  is passed to CMake.
  * The `CL` driver will load `libcompiler.so` by default, located next to
    `libCL.so`, unless the environment variable `CA_COMPILER_PATH` is
    set.
  * Instead of needing to recompile `CL` in offline mode, compilation features
    will be disabled if `libcompiler.so` is missing, or the environment
    variable `CA_COMPILER_PATH` is set to the empty string.
  * If dynamic compiler loading is enabled and
    `CA_RUNTIME_COMPILER_ENABLED=OFF` is passed to CMake, then the `CL` driver
    will not have the capability of loading a compiler library, therefore it
    will always behave in offline mode.
* Added a new OpenCL extension `cl_codeplay_wfv` to expose whole-function 
  vectorization information through OpenCL.
* Removed the OpenCL extension `cl_codeplay_subgroups`. Vector information is
  instead exposed through the new extension `cl_codeplay_wfv`.
* VECZ now adds alignment information to `interleaved_(load|store)`,
  `masked_interleaved_(load|store)` and `gather_(load|store)`.
* VECZ will now move fence instructions associated with barriers to between the
  work item loops during `HandleBarriersPass`, unless the pass was created with
  the `remove_local_fences` option, in which case `single_thread` scope fences
  will be discarded.
* VECZ PartialVectorization now works with barriers.
* The `ReleaseAssert` CMake module is now less strict with which Clang compilers
  it supports.
* Corrected a typo in the `muxCreateExecutableWithOptionsTest` test suite.
* Unlisted public symbols are no longer stripped in Debug builds of shared
  libraries.
* Updated `cl_khr_command_buffer` extension headers to revision 23, and
  additionally report support for the simultaneous-use capability added in
  revision 22.
* Abacus helper functions are now passed inputs by const value, rather than by
  const reference.
* `muxCreateExecutable` has been simplified to only load binary executables.
* `muxCreateBuiltInKernel` has been added to create built-in kernels.
* The changelog can now be automatically collated from individual entries in
 `changelog` via the `build_changelog.py` script.

Non-functional changes:

* Tests equivalent to `UnitCore` `binary` tests were added to `UnitMux`.
* A new test suite called `UnitCompiler` has been added which tests the
  ComputeMux compiler API in a similar way to `UnitCore` and `UnitMux`.
* Added `cargo/thread_safety.h` which defines `cargo::mutex` and
  `cargo::lock_guard`. These are wrappers over `std::mutex` and
  `std::lock_guard` which define clang thread-safety attributes that that can be
  used with Clang's Thread Safety Analysis.
* Added documentation on:
  * Device memory requirements.
  * Floating point requirements.
  * Synchronization requirements.
  * Atomic requirements.
  * What ComputeAorta expects from a customer device driver.
* `compiler::Context`, `mux_allocator_info_t` and `mux_device_t` is now passed
  to `compiler::Module` during construction to simplify the API.
* Rename `compiler::Module::currentOptions` to `compiler::Module::getOptions` to
  more closely mirror LLVM.
* Remove `compiler::Context::mutex()`, and instead make `compiler::Context`
  satisfy the Lockable C++ named requirement.
* Separated barrier splitting from wrapper function generation in
  `HandleBarriersPass`.

Bug Fixes:

* `Execution.Task_03_11_Sum_Reduce4` and `Execution.Task_03_12_V2S2V2S` now pass
  when linking ComputeAorta against LLVM 13.
* Fixed an occasional segfault during the `dma_pessimize_complex_geps.cl` lit
  test.
* VECZ can now broadcast a constant splat vector for scalable vectors on LLVM
  12.
* VECZ will now fail gracefully when a broadcast of a vector can't be done.
* VECZ will fail gracefully instead of hitting an LLVM assert on nullptr during
  materialization of Masked Interleaved MemOps.
* Fixed VECZ crash due to bad dominance after interleaved load/store combining.
    
## Version 1.61.0 - 2021-08-04

Feature Additions:

* OpenCL's `fmax`/`fmin` builtins are now emitted as inline calls to LLVM's
  `maxnum`/`minnum` intrinsics with the aim of improving performance. Because of a
  CTS failure, this is not enabled on Arm targets.
* The cmake variable `CA_RISCV_DEMO_MODE` has been added. When set to true, the
  `riscv` target will dump IR and an elf file for demo/debugging purposes.
* The `noalias` attribute is now added to work-group and work-item scheduling
  info structs so that LLVM can optimize certain induction variables from
  work-group loops via LICM.
* UnitCL testing has been added for the `cl_codeplay_exec_info` extension. This
  makes use of the `cl_intel_unified_shared_memory` extension.
* There is now a second document `overview.pdf` built by the `doc_pdf` target
  containing a high level overview of the CA project and its requirements.
* VECZ now uses target info to determine subwidening packet widths.
* VECZ now limits vector width for builtins and uses call instruction arguments
  to decide vectorization width rather than just the return type.
* VECZ now allows fence instructions through the CFG pass.
* Host now has a lit test `roscc_simplify` to ensure ROSCC doesn't produce
  redundant branches.
* VECZ now supports builtins that have both vector and scalar arguments.
* VECZ now broadcasts fixed-length i1 vectors as i8 to work around lack of
  compiler support.
* VECZ now subwidens memory operations and GEP instructions.
* VECZ has removed the `createRunVeczPass` API taking an option as each
  parameter. The `host` target now passes a `core::utils::VeczPassOptions`
  struct when calling this entry point.
* VECZ added lit tests for scalarization and re-vectorization of `covert_float`
  builtins.

Non-functional changes:

* Compatibility device and command buffer objects have been added to the
  compatibility layer used to implement the ComputeMux runtime in terms of
  `core`.
* Feedback on the ComputeAorta block diagram and the associated documentation
  has been addressed.
* Missing namespace resolution has been added to `multi_llvm`.
* VECZ library command line options have been moved directly into the `veczc`
  executable and are now passed in during pass creation:
  * `vecz-auto`
  * `vecz-simd-width`
  * `vecz-scalable`
  * `vecz-choices`
  * `vecz-llvm-stats`
* The `cl_compiler::Binary` class has been removed from CL.
* The `riscv` target updated its `__ca_wrapped_kernel` symbol address.
* `UnitMux` tests equivalent to `UnitCore` tests for the following
  kinds of objects were added:
  * buffers and images
  * memory
  * command buffers
  * semaphores
  * queues
  * query pools
* VECZ choices are now parsed directly from the command line arguments.
* VK has been refactored to use the CompilerMux compiler interface.
* Comments have been corrected across the codebase in order to remove
  doxygen warnings.

Bug Fixes:

* A compiler warning regarding ignored type qualifier on Ubuntu 20.04 has been
  fixed for `gcc` version 9.3.0.
* `spirv-ll` now ensures function call operands have the correct attributes for
  all supported LLVM versions.
* Memory leaks in OpenCL 3.0 specific `UnitCL` tests have been fixed.
* The OpenCL `CL_KERNEL_ATTRIBUTES` query of `clGetKernelInfo` was corrected to
  only return a non-empty string in the case that the kernel was created via
  the `clCreateProgramWithSource` entry point.
* The CTS test `spir/test_spir kernel_attributes` test has been removed from
  the allow list due to potential spec violation.
* The `riscv` target's `runtimelib` dependency is now only rebuilt when
  necessary rather than every build.
* The locking of `cl_context`'s mutex in various OpenCL entry points has been
  updated to avoid TSAN warnings.
* The `riscv` V scalable vector extension has been unconditionally disabled
  before LLVM 13 due to lack of support in the backend.
* The `MultipleQueueMultipleAlloc` `UnitCL` USM test has been disabled due to
  intermittent failures.
* The `fast_math` compiler pass is no longer run on modules when a ComputeAorta
  build doesn't include the CL target.
* UB in the OpenCL `clEnqueuUnmapMemObject` entry point resulting from addition
  to a potentially null pointer has been fixed.
* VECZ memop "is a" queries no longer modify state.
* VECZ now applies a preheader mask to infinite loop branches to avoid kernel
  hangs on divergent code paths.
* VECZ now runs the lower switches pass after the inlining and CFG
  simplification passes allowing non-unique block predecessors in the CFG.
* VECZ's `partial_linearization15` test has been updated to work with all
  supported LLVM versions.
* The `UnitCL` test `Regression_90_Offline_Local_Memcpy_Fixed` has had the
  `--vecz-check` flag disabled as this is currently producing a false positive.

## Version 1.60.1 - 2021-07-26

Non-functional changes:

* Update ComputeMux Compiler documentation describing mapping to OpenCL concepts
  following refactor.

Bug Fixes:

* VECZ packetize funnel shift intrinsics `fshl` and `fshr`.

## Version 1.60.0 - 2021-07-07

Upgrade guidance:

* `CA_RUNTIME_COMPILER_ENABLED` is no longer used to conditionally compile
  different parts of CL if a runtime compiler is available. Instead,
  `_cl_device_id::compiler_available` is checked at runtime instead, and the
  compiler library implementation is simply stubbed out when
  `CA_RUNTIME_COMPILER_ENABLED` is not defined.
* Compiler specific OpenCL extensions are no longer disabled at CMake configure
  time when a `CA_RUNTIME_COMPILER_ENABLED=OFF` is passed to CMake. Instead,
  they are disabled at runtime in `extension::GetDeviceInfo` if the device has
  no compiler available.
* The OpenCL program binary header format has been changed. Programs saved using
  `clGetProgramInfo` with the `CL_PROGRAM_BINARIES` argument in earlier versions
  of ComputeAorta will not work with `clCreateProgramWithBinary` in this
  version. As a result, any saved program binaries should be regenerated from
  source.
* CMake now only searches for Python 3.6 or higher, effectively dropping support
  for Python 2.7. This may cause previously configured builds to break, the
  recommended solution is to configure a new, clean, build.
* Codeplay OpenCL extensions now use enumeration values reserved for Codeplay
  only use in the OpenCL registry, this is an ABI change. Only the
  `cl_codeplay_performance_counters` extension is affected, applications using
  this extension **must** be recompiled to function as expected.

Feature additions:

* VECZ can now scalarize vector `getelementptr` IR instructions.
* `veczc` will now print diagnostics when LLVM module parsing fails.
* `UnitCL` parameterized test output was improved by providing output stream
  operator overloads for custom parameterized types.
* VECZ can now run LLVM optimization passes between CFG conversion and
  packetization.
* VECZ can now broadcast vectors by scalable amounts, enabling
  PartialScalarization for scalable vectorization factors.
* VECZ no longer rejects scalar builtin calls with no vector equivalent.
* VECZ can now convert small integer vector loads into scalar loads.
* VECZ SIMD Width Analysis pass has been improved and can now run immediately
  before packetization, enables more accurate heuristics to determine the SIMD
  Width used when `-cl-wfv=auto`.
* VECZ now treats CFG divergence reduction builtins as Mark Varying.
* `utils::createRunVeczPass` has been overloaded to accept an instance of
  `vecz::VectorizationChoices`, enables targets to define the set of choices
  used during vectorization.
* RISC-V target now supports Windows.
* Add `muxCloneCommandBuffer` to efficiently copy a `mux_comand_buffer_t`.
* CityRunner now sets the `-q` quiet flag when using the GTest profile over SSH.
* RISC-V target now optionally supports linking builtins before vectorization,
  enables scalable vectorization of builtins.
* CityRunner now supports using a file containing a list of tests to be skipped.

Non-functional changes:

* `UnitCL` test `Execution.Precision_01_Pow_Func` is now disabled on Windows.
* RISC-V target now has a `lit` test suite for validating scalable
  vectorization.
* Docs: Architecture block diagram was introduced alongside a description of
  each block.
* Docs: List of valid values for `CA_USE_SANITIZER` was corrected.
* Docs: List of supported LLVM versions was moved to a separate document.
* Docs: List of supported toolchains was added.
* Porting `UnitCore` to `UnitMux` has been partially completed.
* ComputeMux Compiler interface is now pure virtual, enables future changes to
  support dynamic loading of runtime compilers.
* VECZ now has more assertions in places where LLVM entry points return a
  pointer which cannot be null by design, to resolve Klocwork issues.

Bug Fixes:

* Fix regression from 1.59.0 where `coreCreateExecutable` was being called
  lazily in the first call to `clCreateKernel` instead of during the final
  internal action of `clBuildProgram` and `clLinkProgram`.
* Fix regression from 1.59.0 in the compiler compatibility layer which now
  exposes underlying `core_kernel_t`, fixes customer extension.
* Fix `cl_kernel::kernel_exec_info_usm_flags` not being initialized by default.
* Fix instances `available_externally` linkage type present in SPIR 1.2. The
  intent according to the SPIR specification is to be equivalent to C99 inline
  semantics, however this is not how LLVM handles `available_externally`.
  Instead replace `available_externally` with `link_once`.
* Fix OpenCL-CTS `test_api/consistency_il_programs` test when running in binary
  compilation mode.
* Fix OpenCL-CTS `test_computeinfo` test due to returning an invalid format for
  `CL_DEVICE_LATEST_CONFORMANCE_VERSION_PASSES`
* Fix uninitialized error code in `clCreateCommandBufferKHR`.
* Fix a missed scalable vectorization of selects in VECZ.
* Fix Windows linker issue in `multi_llvm` header only library on Windows by not
  including `clang` headers.
* Fix tracing of `corePushReadBuffer` when an invalid `core_command_group_t` is
  provided.
* Fix RISC-V target multi-threaded compilation issues in LLVM/LLD interfaces by
  ensuring a mutex lock is held.
* Fix LLVM 13 debug info verification failures in IR produced by
  `ReplaceLocalModuleScopeVariables`.
* Fix compile errors when linking LLVM 13 in `multi_llvm::runCodegenLLVMPasses`.
* Fix LLVM 13 compile error when using `llvm::MemoryBuffer::getFile()` in
  `spirv-ll`.
* Fix using ComputeAorta repository as a submodule by replacing
  `CMAKE_SOURCE_DIR` with `PROJECT_SOURCE_DIR`.
* Fix compile errors when using GCC 11 by updating the in-tree copy of Google
  Benchmark.
* Fix compile errors when using GCC 11 by including `<limits>` in `cargo`
  headers.
* Fix internal compiler error when using GCC 11 in combination with
  `-gsplit-dwarf`.

## Version 1.59.0 - 2021-06-01

Feature Additions:

* VECZ allow widening of scatter/gather memory operations.
* Move `host`s LLVM diagnostic handler implementation to `utils` module.
* Use `utils` LLVM diagnostic handler in the `riscv` target.
* VECZ allow scalable vectorization of index vectors.
* Add disabled list option to City Runner.
* Add support for combining ASan and UBSan in a single build.
* VECZ allow sub-widening of the following:
  * PHI nodes and fmuladd intrinsics.
  * Unary Ops, Binary Ops and Casts.
  * ICmp, FCmp and Select instructions.
  * Extract Element, Insert Element and ShuffleVector instructions.
  * Builtin calls.
* `veczc` allow setting `PartialScalarization` on the command-line.
* RISC-V automatically generate libraries for compiler runtime linking.
* Update `cl_khr_command_buffer` implementation to revision 20 of the
  provisional specification.
* VECZ add full support for scalable-vector memory operations.

Non-functional changes:

* Rename "Core" terminology in USM extension to "Mux".
* Move deferred compilation logic into the compiler library.
* Introduce additional `clUpdateMutableCommandsKHR` UnitCL tests.
* Relax thread safety of `muxFinalizeCommandGroup`.
* Update Getting Started documentation.
* Move compiler library source files to `source/compiler`.
* Remove use of `const_cast` from `cl_khr_command_buffer` implementation.
* Move CMake options for the `CL` target into a separate file so they can be
  accessed outside of `source/cl`.
* Re-enable scalarization pass when scalable vectorization is enabled.
* Don't handle terminal resizing in City Runner output.
* VECZ update all lit tests to `RUN` lines which use pipes.
* Move `listSnapshotStages` to the compiler library.

Bug-fixes:

* Validate kernel names in the `riscv` target, fixes the 
  `coreCreateKernelTest.InvalidName` UnitCore test.
* Fix upstream LLVM support to commit `a3d273c9`.
* Fix BenchCL's exit code.
* Fix various issues in the `riscv` target:
  * Update `riscv/riscv.h` to version 0.42.0.
  * Fix unused variable error in release mode.
  * Fix `riscv` target double support logic.
* VECZ fix using of `multi_llvm::ElementCount` initialization error for LLVM
  versions less than 12.
* Skip `cl_khr_command_buffer` UnitCL test relying on a compiler when no
  compiler is present.
* VECZ tweak `duplicate_preheader` lit test to keep testing desired behaviour.
* Fix City Runner gtest profile rebooting twice.
* Fix assertion in `compiler::ProgramInfo` iterator interface.
* Fix performance issue when skipping all tests in City Runner.
* Fix compile error with LLVM 13 in `dx2llvm` tool.
* Fix `cargo::small_vector` correctly call destructor in move constructor and
  move assignment operator.
* Fix AArch64 OpenCL-CTS compile errors.
* Fix compile errors on LLVM 13.
* Fix `riscv` target behaviour when an ELF file contains no kernels. 
* Fix `riscv` `ReplaceLocalModuleScopeVariables` for case where `InsertElement`
  is a user.
* Fix compiler library CMake to check for file path rather than directory path.
* VECZ don't vectorize constant operands of a GEP instruction during
  packetization.

## Version 1.58.0 - 2021-05-03

Upgrade guidance:

* Application code making use of the draft `cl_khr_command_buffer` extension
  must be updated due to entry point signature changes in the latest revision of
  the header file.

Feature Additions:

* Enable `codeplay_kernel_exec_info` extension by default.
* VECZ Allow widening of masked memory operations and use them.
* Bump command-buffer implementation to spec revision 18.

Non-functional changes:

* Merged `offline` and `llvm` variants of the CL `compiler::Binary` and
  `compiler::Context` classes.
* Use internal mirror for OpenCL-Intercept-Layer submodule.
* Implemented `muxCreateKernel` and `muxPushNDRange` with kernels created by Mux
  itself.
* Move compilation option parsing from `compiler::Binary` to `compiler::Module`.
* Add documentation for `utils::ReplaceCoreMathDeclarationsPass`.
* Expand `CL_KERNEL_ARG_TYPE_NAME` testing.
* Remove `vecz-check` exceptions that now vectorize.
* Update a comment addressing an OpenCL 3.0 unknown.
* Add a command-line tool to print detected `host` target system info.
* Port LLVM bisect script to Python 3 and cleanup.
* Update Python build script to support LLVM 12 builds.

Bug-fixes:

* Add LLVM 12 checks to VECZ lit tests.
* VECZ Prevent scalable vectors from crashing the mangler.
* VECZ Disable partial scalable vectorization on kernels with barriers.
* Teach VECZ about LLVM 12 alias analysis scope intrinsics.
* Use newer OpenCV which fixes accuracy assumptions upstream.
* Add missing `REQUIRES: nospir` to UnitCL regression 100 test kernel.
* VECZ test id-indexed loads from and stores to 3-vector arrays.
* VECZ Update exit mask after moving PHI node.
* Add `-print-after-all` support to ComputeAorta LLVM passes.
* Add regular expression support to UnitCL `printf` validation.
* Scalarization failures are not necessarily a VECZ failure.
* Refactor `riscv` target device info.
  * Enables using the `cross` target with `clc` when `riscv` and `host` targets
    are both enabled.
  * Enables handling multiple devices provided by the `riscv` target.
  * Resolves a duplicate symbol issue in device info objects.
  * Improved separation between device info and device objects.
* Remove `ccache` package from the ecosystem Dockerfile.
* Add `riscv` target documentation to Sphinx documentations table of contents.
* Fix `cargo::expected`/`cargo::optional` map compilation failure when both
  headers are included.
* Make `_cl_command_buffer_khr` thread safe.
* Fixed changing specialized kernel arguments after enqueue in the `riscv`
  target.
* Return correct error code from `vkGetPhysicalDeviceImageFormatProperties`.

## Version 1.57.2 -- 2021-04-22

Non-functional changes:

* Refactored `cl_khr_command_buffer` extension so that it is better
  encapsulated, including reduction of code duplication and the addition of
  UnitCL tests.
* Added `check-UnitCL-USM` target for running UnitCL with just USM tests.

Bug-fixes:

* Fix for incrementing the command index counter when adding non-NDRange
  commands to a command buffer. Also added additional UnitCL tests to cover
  this case.

## Version 1.57.1 -- 2021-04-16

Feature Additions:

* Implemented the mutable dispatch feature of the `cl_khr_command_buffer`
  extension. This includes:
  * `coreUpdateDescriptors` and `muxUpdateDescriptors` entry point and
    accompanying spec and change log entries.
  * `hostUpdateDescriptors`, `crossUpdateDescriptors`, `riscvUpdateDescriptors`
    and `stubUpdateDescriptors` definitions.
  * Implement the OpenCL APIs for mutable dispatches.
  * Adding UnitCL and UnitCore tests for mutable kernel arguments.

## Version 1.57.0 -- 2021-04-06

Upgrade guidance:

* Removed support for LLVM 7.
* Updated OpenCL-Headers and OpenCL-ICD-Loader submodules to make use of
  upstream support for USM. As a result, you will need to do a submodule update
  to get the correct OpenCL-ICD-Loader commit to build.
  * OpenCL-Headers is at `23710f1b99186065c1768fc3098ba681adc0f253`
  * OpenCL-ICD-Loaders is at `b68b15dfe93d066193c1e40d876e6278d086a6f3`

Feature Additions:

* Added the RISC-V core target to the ComputeAorta project as well as the HAL
  submodule. This requires LLVM 12 or greater and the LLVM RISCV backend to be
  present when enabled. This target is disabled by default and may not be
  present depending whether it is licensed as part of your project.
* Added `-cl-precache-local-sizes` option to `codeplay_extra_build_options`
  extension. This flag aims to allow all kernel compilation to take place in
  `clCreateKernel` when the local sizes are known in advance. This is useful
  primarily when measuring CA's performance where eliminating time spent in
  `clEnqueueNDRangeKernel` is helpful so we can focus on measuring kernel
  execution time.

Non-functional changes:

* For VECZ created a packetization-strategy agnostic result struct, centralised
  all the plumbing and create data structures to enable widening by different
  widths in the same kernel.
* Added missing Valid Usage's to the Mux specification.
* Removed deferred compilation from Mux by deleting scheduled and specialized
  kernels from the specification.
* USM definitions in `cl_ext_intel_usm.h` and `cl_ext_intel.h` are now part of
  `cl_ext.h`. 

Bug-fixes:

* Fix `reqd_work_group_size` UnitCL test which was crashing due to unintialized
  pointer variable that was freed when a CL API call failed.
* VECZ Packetization failures no longer result in an error.
* Fixed the `RunVeczPass` to drop the vectored function if it fails to support
  partial vectorization when requested.
* CMake now verifies zlib availability during configuration by checking if the
  target compiler has zlib support. If it doesn't then neither will the
  compiler in `CA_BUILTINS_TOOLS_DIR`.
* CMake will also verify `CA_HOST_ENABLE_FP16` by only allowing it on known
  good targets. This prevents situations where the build will succeed but the
  tests will fail due to lack of FP16 support. 
* Reimplemented and fixed `native_log2` and added additional testing for it.
* Added various fixes to support the upstream LLVM main branch:
  * Refactored `ProgramInfo`, `KernelInfo` and removed `Argument/ArgumentBase`
    classes in CL Compiler.
  * Stopped using the deprecated `IRBuilder::CreateLoad API`
  * Added multi_llvm helpers for `IRBuilder CreatAtomic*` and dividing
    `ElementCount` types.
  * Fixed `masked_interleaved.ll` and various other lit tests.
  * Fixed LLVM `CloneFunctionInto` usage.
  * Started building offline kernels when testing tip.
  * Fixes for `getRegisterBitWidth()` API change.
* Added a fix to reduce precision requirements for `native_builtin` variants to
  satisfy OpenCV precision requirements.
* Added a fix to preserve integer divide by zero behaviour by deferring clang
  codegen LLVM passes.
* Added a fix for payne-hanek in abacus to use selects instead of shuffles to
  get the middle filter values and improve performance.
* Added support to allow `--vecz-check` flag to support for the VECZ order
  tests in UnitCL in the X dimension and marks them skipped for the Y and Z
  dimensions.
* Fixed half precision `pown()` UnitCL reference.

## Version 1.56.1 -- 2021-03-16

Feature Additions:

* Adds `cl_event` synchronization support to `clEnqueueCommandBufferKHR`.
* Adds the `coreFinalizeCommandGroup` entry point to core (including host
  implementation and a corresponding `mux` wrapper).
* Implements the `clCommandFillBufferKHR`, `clCommandCopyBufferRectKHR`  entry
  points.
* Updated `cl_command_buffer_khr` header to reflect the latest specification.

## Version 1.56.0 -- 2021-03-02

Feature Additions:

* VECZ
  * Started adding support for scalable vectorization factors to the VECZ
    interfaces with which users can request scalable vectorization widths from
    VECZ and pass them to the barrier pass.
    * Added the first end-to-end vectorization of simple kernels consisting of
      contiguous loads and stores, most built-in IR operations, without builtins
      (that can't be inlined) or barriers.
    * A command-line option `-vecz-scalable` was added which can force scalable
      vectorization on or off. By default it lets the user-configured
      vectorization factor through.
    * Added support for scalable widths in work-item loops.
    * Combined various approaches to VECZ widening.
  * Support for VECZ partial vectorization in the VECZ util pass.
  * Added `--vecz-check` option to UnitCL tests so that tests will fail if not
    vectorized.
  * Instantiate vectors over the scalarization threshold and stop scalarizing
    loads and stores.
* Added an option `CA_USE_SPLIT_DWARF` for splitting DWARF debug info. This
  significantly reduces binary size and speeds up linking.
* Added initial support for building with LLVM 13.
  * Add multi llvm path for reporting support for OpenCL options
  * Add ifdef for setting `DisablePCHValidation` preprocessor option, which is
    now called `DisablePCHOrModuleValidation` and is an enum instead of bool.
  * Account for renaming of `RF_MoveDistinctMDs` to
    `RF_ReuseAndMutateDistinctMDs`
  * Add ifdef for supporting new ASTReader constructor - where one of the bools
    is now an enum.

Non-functional changes:

* OpenCL 3.0 
  * Added additional tests for `atomic_compare_exchange_weak_explicit` and
    `atomic_compare_exchange_strong_explicit` builtins to the kts.
* In debug builds the environment variable `CA_HOST_TARGET_CPU` will be
  respected if it is set.
* Updated `run-cmakelint.py` to return exit codes given by `cmakelint`.
* Regenerated the builtin headers with new clang-format rules.
* Compiler Refactoring
  * Refactored and extracted compiler out of `compiler/llvm/binary.h` the
    primary changes as part of this process are the changing of a few variables
    and moving the following functions into a new class `Module`.
    * `Binary::Compile` -> `Module::compileOpenCLC`
    * `Binary::Link` -> `Module::link`
    * `Binary::Finalize` -> `Module::finalize`
    * `Binary::Deserialize` + `spir::compile` -> `Module::loadSPIR` +
      `Module::compileSPIR`
    * `spirv::compile` -> `Module::compileSPIRV`
  * Extracted LLVM module serialization from Binary to allow the LLVM module to
    be made a private member of `compiler::Module`.
* Added `restrict` to the printf prototype to match the OpenCL C specification.

Bug-fixes:

* Fixes for following tip LLVM.
  * Fixed DebugSupport lit tests due to the requirement of `dso_local` as well
    as `nsw` positioning.
  * Updated partial linearization lit tests to support LLVM tip optimizations.
  * Add multi_llvm helper function to support new `CloneFunctionInto` function
    signature.
* Fixed validity check in `clGetKernelWorkGroupInfo` when the device type is
  not custom and the query is on a non-builtin kernel.
* Added `cl_khr_fp16` pragma to `clGetKernelArgInfo` half tests.
* Fixed a few issues highlighted by the address sanitizer in the USM tests.
* Fixed a case where clc would segfault on zero sized inputs attempting to
  check for magic numbers, `clCreateProgramFromBinary` would error out trying
  to mmap zero bytes of the file.
* Fixed an issue with our batching algorithm so that there is always a
  semaphore dependency on the last pending dispatch and on all running command
  buffers. This resolves issues related to user events which resulted in out of
  order execution.
* Fix for `regression.27` test in UnitCL where it would fail on devices where
  work items aren't guaranteed to execute in order.

## Version 1.55.2 -- 2021-02-11

Bug-fixes:

* Command buffer extension handling in `_cl_command_queue` refactored
  to fix out of order execution bug that was occurring for in order queues.

## Version 1.55.1 -- 2021-02-04

Bug-fixes:

* Corrected `cl_khr_command_buffer` extension so that it does not build by
  default.
* Change the ReleaseAssert CMake module to determine its behaviour based on
  the `CMAKE_CXX_COMPILER_ID` variable rather than platform flags (`UNIX`,
  `WIN32`, `ANDROID`). This resolves an issue where MinGW is being used on
  Windows where `NDEBUG` compiler options were not detected correctly.

## Version 1.55.0 -- 2021-02-02

Upgrade guidance:

* Added a deprecation message for LLVM 7. Support for LLVM 7 will be removed in
  version 1.56.

Feature Additions:

* Added the initial `cl_khr_command_buffer` proof of concept:
  * Implement a the minimum API required to get the spec example working.
  * Add the spec sample code as an example application.
  * Add UnitCL tests for the minimal functionality.
* Continued development of OpenCL 3.0 prototype.
  * Removed `pass_fast_math` to meet the relaxed math ULP requirements in 3.0.
  * Added additional tests for C11 atomic `atomic_fetch_.*_explicit` Tests.
* Took the final steps to support USM in 1.2 with `clSetKernelExecInfoCODEPLAY`
  extension. 
* As part of our move from the Core API to ComputeMux (Mux) we have refactored
  the CL runtime to refer to Mux instead of Core. This includes using the
  `mux.h` header and renaming core types to mux types (such as `core_device` to
  `mux_device`).
* New format for output metadata in VECZ to allow to identifying attempted
  vectorization width, vectorization result (success/fail), and reference to
  the respective scalar/vector equivalent of each kernel.
  * Scalar function is linked to vectorized function.
  * Vectorized function is linked to scalar function.
  * VECZ width present in both metadata tuples
  * VECZ dimension present in both metadata tuples
* Made various changes to continue supporting tip LLVM 12.
  * `llvm::DebugLoc::get` was removed in LLVM 12 so various changes were made
    to the `multi_llvm` headers to support this.
  * Made changes to support `InstructionCost` API changes.
* Set `CMAKE_EXPORT_COMPILE_COMMANDS` by default which generates
  `compile_commands.json`.
* Updated `AddKernelWrapper` to support aligning all members of the packed args
  when the packed arg is set to `false`. The alignment is to a power of 2 equal
  to or above the size. It will use the `DataLayout` if it fits this pattern.
* Emit LLVM diagnostics from Core targets by making use of
  `core_callback_info_t`, when specified by the user, in a new 
  `DiagnosticHandler` provided to the `llvm::Context` owned by `core_finalizer_t`. Then,
  when an user sets a `cl_context` callback, any diagnostics which are emitted
  by LLVM inside a Core target will result in the `cl_context` (or
  `core_callback_info_t`) callback being called.
* Allow parsing/printing of textual LLVM IR in VECZC. The VECZ tool can now
  read and write LLVM modules in their textual form. These may be taken from
  stdin or regular files, and may be output to stdout. DXIL modules must still
  be provided as bitcode.

Non-functional changes:

* Updated the Vulkan runtime components to use Mux types and entry points.
* Added `add_ca_unitcl_check` function to CMake to make specifying check
  configs more concise.
* Continued refactoring of VECZ in particular the Instantiation Pass and
  unified the instantiation and packetization approaches.
  * Retired the `Value Tags` system as passes are now responsible for their own
    internal state.
* In VECZ, moved all Uniform Value Analysis related state into Uniform Value
  Result.
* Renamed a UnitCL kernel from `memcpy` to `memcpy_optimization` as naming a
  kernel `memcpy` is undefined behaviour.
* Updated the version check for `format-overflow` with gcc from 9.9 to 11.
* Fixed `cmakelint 1.4.1` warnings.
* Updated `clang-tidy` rules to support `clang-tidy-11`.
* Moved the `ReplaceLocalModuleScopeVariablesPass` from host to `core::utils`
* Updated `.clang-format` rules to not use a format heuristic for pointer
  alignment.
* In VECZ, we have added a `multi_llvm::ElementCount` to track the
  vectorization factor rather than SIMD width. The `ElementCount` helper wraps
  a known minimum value and whether it's scalable: a constant multiple of that
  minimum value. The vectorization factor almost invariably drops back to the
  old behaviour by calling `getFixedValue`, but not before returning a more
  graceful error if the vectorization factor is scalable. Over time it will be
  possible to generate actual scalable code with this approach. There should be
  no functionality change as nothing is currently able to request scalable
  vectorization, and most passes should fail gracefully when encountering such
  a scenario.

Bug-fixes:

* Fixed various issues to support LLVM 12 tip:
  * Added a fix to support SPIRV-LL attributes `ByVal` and `StructRet` as LLVM
    12 requires these to have types attached.
  * Update lit tests to allow for `poison` instead of `undef`, and `dso_local`
    to reflect changes to IR generated by LLVM 12.
  * Update memintrinsics lit test to take into account LLVM 12 more intelligent
    alignment calculations.
  * Updated `binary.cpp` to take into account `setLangDefaults` signature
    change.
  * Fix scalarization debug info not matching variable locations in LLVM 12+.
  * Adapted LLVM 12's new InstructionCost class API for CA's multiple LLVM
    versions support. This included removing an assert checking that
    instruction cost != -1 from `multi_llvm.h`.
* Removed some usages of `memset()` in UnitCL tests to avoid triggering
  `class-memaccess` warnings in GCC. We also stop ignoring this warning in
  CMake.
* Fixed an issue with embedding build options in SPIR `.bc` files. All options
  in `DEFINITIONS` and `SPIR OPTIONS` will now be considered correctly.
* Fixed missing remarks in VECZ. LLVM's remark system uses the "pass name" as
  the filter by which to decide whether to display or discard remarks. The
  vectorizer was setting the pass name to the empty string, meaning users
  couldn't use options such as `--pass-remarks=vecz` or
  `--pass-remarks-missed=vecz` as the documentation suggests.

## Version 1.54.0 -- 2021-01-05

Upgrade guidance:

* The `AddParamsPass` utility has been removed. Downstream code using
  `AddParamsPass` must be refactored.
  * `AddParamsPass` was not a compiler pass in its own right. It was only a
    thin wrapper class used to create module passes that call
    `core::utils::cloneFunctionsAddArg()`. This was a non-standard design
    pattern and led to confusion.
  * The recommended upgrade path is to refactor passes created with
    `AddParamsPass` to call `core::utils::cloneFunctionsAddArg()` directly.
  * Examples of refactored passes are
    `modules/utils/source/AddWorkItemInfoStruct.cpp` and
    `modules/utils/source/AddWorkGroupInfoStruct.cpp`.

Feature Additions:

* Started process for renaming the Core API to ComputeMux (referred to as Mux
  for short in project source and for implementation). This includes:
  * Created a copy of the Core specification, updating it to refer to
    ComputeMux instead. This is placed in `modules/mux` alongside
    `modules/core` whilst it is finalized.
  * Created a Mux version of the auto-generated Core header. This is
    essentially a copy-paste of the auto-generated Core header with the changes
    in the spec applied to it. However, this header is written manually, as
    most of the types are typedefs of Core.
* Continued validation of OpenCL 3.0 prototype with additional tests.
  * Added UnitCL testing for C11 atomic builtins including `atomic_init`,
    `atomic_load_explicit`, `atomic_store_explicit`, `atomic_work_item_fence`,
    `atomic_exchange_explicit`, `atomic_flag_set_explicit` and
    `atomic_flag_clear_explicit`.
  * Add `always_inline` attribute to `as_type` builtins.
* Additional support for the Intel USM extension.
  * Added `clGetEventInfo` tests for USM.
  * Added support for for passing the following USM flags to
    `clSetKernelExecInfo`:
      * `CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL`
      * `CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL`
      * `CL_KERNEL_EXEC_INFO_INDIRECT_INDIRECT_ACCESS_INTEL`
* Added initial support for KHR Command Buffer extension.
  * Register `cl_khr_command_buffer` as a device extension.
  * Add the APIs required for the proof of concept.
  * Add a test to check that `clGetFunctionAddressForPlatform` gets the
    addresses of the APIs.
* Added additional `multi_llvm` support for the latest LLVM 12 changes.
* To help debuggability, especially on Jenkins we set the environment variable
  `FILECHECK_DUMP_INPUT_ON_FAILURE` which causes filecheck to print the
  contents of stdin to stderr which gives a better idea of what went wrong.
* Re-enabled the Loop Invariant Code Motion pass.

Non-functional changes:

* Simplified some of the control flow in VECZ.
* Continued the refactoring of VECZ:
  * VECZ now scalarizes iteratively instead of recursively. This gives control
    of which values to scalarize back to the scalarization pass, and allows
    scalarizing from beginning to end instead of recursing through operands
    from the vector leaves.
  * Refactored the instatiator to make it more self-contained by moving the
    invocation of instantiation analysis and the SIMD Duplicate code into the
    instantiator.
  * VECZ now builds the scalar gather at the original vector instruction.
* Moved host-specific ELF relocations to host code.
* Removed the outdated `cmakelint` from the project source tree.
* Disabled error on `class-memaccess` warning in `gcc-10.2`.
* Removed explicit host dependency in the cross builds by using libraries
  specified in `CA_CORE_CROSS_COMPILERS` when linking into the cross target.
* Added clarifying documentation on how to build `llvm-spirv`.
* Started process of replacing OpenCL type wrappers in UnitCL to unify the
  different approaches into a single set of wrapper types.
  * Replaced `tycl_s` with `ucl::s` in UnitCL.
  * Added `ucl::Half` output stream support.

Bug-fixes:

* Fixed a VECZ case where stores marked incorrectly as vector leaves would
  generate non-performant code.
* Fix for LLVM 12 removing `llvm::cl::ParseEnvironmentOptions` by emulating
  using `llvm::cl::ParseCommandLineOptions` and passing through the environment
  variable.
* Fix for properly attaching loop metadata following LLVM conventions with
  spirv-ll.
* Fixed an unused variable warning for older versions of LLVM in the
  `multi_llvm` headers.
* Fixed a `std::array` initialization issue in the `clGetKernelInfoTest`
  UnitCL test.
* Fixed debug info intrinsic being created using `undef` values. When an
  `Instruction` is deleted, its debug uses change to `undef` or and empty
  `MDNode`. We now make sure not to let through the intrinsic if that's the
  case.
* Fix for module verify fails with VECZ by using the element alignment when
  scalarizing mem ops.
* Various minor fixes as reported by `klocwork`, primarily added additional
  asserts.
* Fixed dereferencing NULL inputs by adding checks and `INVALID_VALUE` returns
  CL image and buffer read/write/copy entry points.
* Added missing flag to 32-bit builds for forcing use of SSE instructions over
  x87.

## Version 1.53.1 -- 2020-12-10

Bug-fixes:

* Fix assumptions in `clGetKernelInfo` and `clEnqueueNDRangeKernel` tests where
  the `reqd_work_group_size` kernel attribute could exceed the values for a
  device's `CL_DEVICE_WORK_ITEM_SIZES` or `CL_DEVICE_WORK_GROUP_SIZE` properties.

## Version 1.53.0 -- 2020-12-01

Upgrade guidance:

* The Core API has been bumped a patch version to `0.40.2` in order to add the
  following core builtins:
  * `size_t __core_get_global_linear_id()`.
  * `size_t __core_get_local_linear_id()`.
  * `size_t __core_get_enqueued_local_size(uint)`.
  * Add `core_source_type_llvm_120` and `core_source_capabilities_llvm_120` for
    supporting LLVM version 12.0.0.
* The `CA_ENABLE_HOST_IMAGE_SUPPORT` option is now set to `OFF` by default.

Feature Additions:

* The project is moving to tracking tip LLVM. To support this we now allow
  CMake to configure the build system even in the presence of LLVM version
  which are not officially supported. We now emit an `AUTHOR_WARNING` instead
  of `FATAL_ERROR` in this case. By making this a developer warning it is
  possible to pass `-Werror=dev` to cmake to optionally match the old
  `FATAL_ERROR` behaviour e.g. in a CI environment.
* Added initial LLVM 12 support.
  * This required the addition of a `LinearPolyBase` class which serves as a
    base class for `ElementCount` and `TypeSize`. This tries to represent a
    linear polynomial where only one dimension can be set at any one time
    (i.e. a `TypeSize`/`ElementCount` is either fixed-sized) or scalable-sized,
    but cannot be a combination of the two. This changes the way to query the
    number of elements via ElementCount and the size via TypeSize.
* Continued development on OpenCL 3.0 prototype with additional features.
  * Support for `ctz` builtin in `spirv-ll` including tests.
  * Support for `EnqueuedWorkgroupSize`, `LocalInvocationIndex` and
    `GlobalLinearId` global OpVariable in `spirv-ll`.
  * Support for `spirv-ll` atomics.
* Additional support for the Intel USM extension.
  * Implement `clSetKernelExecInfoCODEPLAY` extension as first step to
    supporting USM in the OpenCL 1.2 driver.
  * Implement `CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL` and add support for this to
    `clSetKernelExecInfo` for the USM extension including appropriate testing.
  * Implement USM APIs `clEnqueueMemAdviseINTEL` & `clEnqueueMigrateMemINTEL`
    as no-ops.
* Improved compiler extension support.
  * Added support to the extension module for reporting compiler and runtime
    extensions separately.
  * Added support to the CL compiler `Options` struct for tracking compiler and
    runtime extensions separately from definitions.
  * Registers support of compiler extensions.
* We now use a new liveness analysis pass in Control Flow Conversion/BOSCC and
  the SIMD Width Analysis pass which gives us more performant IR analysis and
  also fixes some issues with redundant PHI nodes being generated.
* Added `CA_CORE_ENABLE_SHARED_LIBRARY` option to allow building `core` as a
  standalone shared library.

Non-functional changes:

* Added `--repeat` option to City Runner to allow repeated running of tests.
* Added `-vecz-dump-report` option for vecz. This was previously a hardcoded
  value.
* Added some additional information to our abacus documentation, in particular
  information on ULP and how to achieve precision targets.
* Additional host passes have been made more generic and moved into
  `core::utils`, specifically the `AddKernelWrapper`, `AddWorkItemInfoStruct`,
  `ReplaceLocalWorkItemIdFunctions`, `AddWorkGroupInfoStruct`,
  `ReplaceNonLocalWorkItemFunctions` and `AddWorkItemFunctionsIfRequired`
  passes.
* Scalarization pass functions have been moved into the scalarizer utility
  class and the `ScalarContext` struct has been refactored away.
* Added UnitCL and UnitCore tests to verify zero-sized enqueue behaviour is
  correct.
* Improve build error output when building kernels so that error codes will
  be emitted.
* Modified the `std::function` signature passed to `CloneFunctionArg` to use
  `llvm::Module` rather than `llvm::Context`. This is to allow accessing of the
  `llvm::DataLayout`.
* We now run SIMD Width Analysis at the start of the Scalarization Pass when
  `-cl-wfv=auto` is specified. Also use new SIMD width strategy that picks the
  widest of the two main strategies.
* Compile kernels in `clCreateKernel` if they provide `reqd_work_group_size`
  and cache the result to avoid compiling when the kernel is enqueued.
* Removed the `-fno-builtin` option from Abacus.

Bug-fixes:

* Fix `mmap` returning incorrect return code on failing map when using memory
  tracer.
* Fix `host` extensions being reported for all devices, no just `host`. Fixed
  and clarified the documentation for this as well.
* Fix undefined behaviour in `Execution.Barrier_22_Barrier_Local_Arrays` test.
* Fix for doubles being generated in spirv printf tests.
* Fix for LinearID UnitCL test which was would result in a segmentation fault
  in certain conditions.
* Fix for CMake to allow installing of kernels correctly when using
  `CMAKE_INSTALL_PREFIX`.
* Fix issue with `oclc` not performing correct `OCLC_CHECK`.
* Fix non-deterministic failure with `OpenCL-CTS spir` tests. The cause of
  these failures was a file system data-race when multiple instances of the
  `conformance_test_spir`executable we extracting zip files containing the SPIR
  bitcode test inputs in parallel. To resolve this issue, the `no-unzip` flag
  has been added to the spir test entries in the conformance test CSV files and
  the `OpenCL-CTS-spir-unzip` target has been added to unzip these prior to
  execution. The unzipped files are also installed alongside the zip archives,
  this allows the `conformance_test_spir` executable to be run successfully
  with or without the `no-unzip` flag. This includes adding the
  `OpenCL-CTS-spir-unzip` target to `ALL`.
* Fixed an issue with a UnitCL kernel named `memset` by renaming to
  `memset_kernel`.
* Fixed an issue with `PassSpirFixup` discarding call instructions attributes.
  This seemed to be caused by replacing of `ReadNone` attributes with
  `ReadOnly`, as such conservatively apply these instead.
* Fixed an issue with the reduce register pressure pass so that it no longer
  attempts to move candidate alloca instructions as they must remain contiguous
  and within the entry block of the function.

## Version 1.52.1 -- 2020-11-20

Feature Additions:

* Define specification for and implement generic address space SPIR-V extension
  for supporting USM.
  * With this extension enabled a SPIR-V module can pass the `Generic` storage
    class for all of its pointer type declarations to indicate that no address
    space information is included in the declaration.

Bug-fixes:

* Fix bug in UnitCL USM tests which were not releasing `cl_event`s.
* Fix bug passing `CL_MEM_ALLOC_HOST_PTR` flag to `clCreateBuffer()` introduced
  in previous release for core devices which don't support host coherent
  memory.
* Support copying to and from arbitrary host pointers in USM
  `clEnqueueMemcpyINTEL`.
* Permit kernel args being set by the runtime on `__private` address space
  pointers when experimental SPIR-V extensions are enabled.
* In kernel entry points for USM perform an internal retain of `cl_event`. This
  prevents segfaults when the USM extension is enabled but the kernel doesn't
  have any USM arguments.

## Version 1.52.0 -- 2020-11-03

Upgrade guidance:

* The Core API has been bumped a minor version to `0.40.0` in order to clarify
  when caching of memory allocations between device and host takes place. Core
  modifications include:
  * The underlying value of enum `core_allocation_type_alloc_device` has
    changed from `0x2` to `0x1`, ensure the symbolic type name is used to
    preserve existing behaviour after upgrade.
  * `coreAllocateMemory()` argument `host_pointer` has been removed as CL code
    now uses `coreCreateMemoryFromHost()` for this purpose.
  * `core_allocation_capabilities_e` enums have been renamed to reflect the
    memory characteristics of the hardware, rather than API code inferring this
    from how memory can be allocated.

Feature Additions:

* Added support for LLVM 11. 
* Expanded UnitCL CMake handling of kernel options regex with additional regex
  checks. This allows more fine grained specfication of compiler definitions
  and options for the different tools we use on our kernels. These new options
  include:
  * `DEFINITIONS` for specifying compiler definitions, 
  * `SPIR OPTIONS` for specifying options for the SPIR compiler.
  * `SPIRV OPTIONS` for specifying clang SPIR-V compilation options.
* Continued development on OpenCL 3.0 prototype with additional features.
  * Added OpenCL features to `CL_DEVICE_OPENCL_C_FEATURES` and appropriate
    tests.
  * Tie SPIR-V support to availability of a compiler.
* Continued development of `cl_intel_unified_shared_memory` extensions. This
  includes:
  * Fixes for UnitCL tests when checking if device has `HostMemAlloc` support.
  * Implemented `clSetKernelArgMemPointerINTEL` entry point.
* Added a new standalone VECZ tool, `dx2llvm`, to convert DXIL bitcode files to
  LLVM bitcode, dropping the DXIL bitcode header and root table.
* Spirv-ll now supports translating of subgroup builtins and generates the
  correct builtin calls.
  * This must be enabled via the `CA_ENABLE_SPIRV_LL_EXPERIMENTAL` cmake flag.

Non-functional changes:

* Re-enabled several disabled UnitCL tests that were disabled due to specific
  host requirements (such as fp64) which weren't respected. 
* Fixed inefficient SPIR generation where `spirv-ll` converted calls from
  `memset` into `memcpy` by adding additional `memset` generation cases.
* Added support for experimental storage class with `spirv-ll`.
* The Control Flow Conversion pass has been moved before scalarization. This
  allows handling of masked loads and stores as well as other potential
  benefits such as late evaluation of the SIMD Width Analysis.
* Cleaned up some aspects of our CL extension code including removing empty
  functions and fixing comments.
* Fixed wrong optimization of ARM NEON instructions when using `-cl-wfv=auto`.
* Logic for implementing OpenCL `CL_MEM_USE_HOST_PTR` has been moved to
  ComputeAorta's CL code from the Core device implementation. Resulting in
  removal of the argument `host_pointer` from `coreAllocateMemory()` in Core
  version `0.40.0`.


Bug-fixes:

* Fixed an issue with building ReleaseAssert on Windows with CMake 3.11 due to
  string matching of `/D NDEBUG`.
* Fix cross-compilation issue with `arm_neon_store` test not compiling for the
  right architecture.
* Fixed `op_fcmp_on` SPIR-V lit test failure caused by a change in behaviour of
  `glslangValidator` by moving some tests from GLSL path to the SPIR-V assembly
  path.
* Fixed an issue with DXIL thread ID calls being incorrectly instantiated.
* Various minor fixes as reported by `klocwork`, primarily added additional
  asserts. 
* Added a fix for using multiple ICDs with Windows 10 in the check targets. The
  check targets now explicitly select ComputeAorta for use in case other ICDs
  are installed.
* Fixed `CL_DEVICE_MAX_CLOCK_FREQUENCY` reporting 0 when using host core
  implementation.
* Fixed generation of `!llvm.loop metadata`, previously this was attached to
  the header of the loop. However, the LLVM spec says it should be attached to
  the latch of the loop. The loop metadata  handling logic has been rewritten
  to support this.
* Fixed Abacus FTZ precision failures caused in 1.51. 
* Fix for UnitCore test calling `SetUp` multiple times.

## Version 1.51.0 -- 2020-10-06

Upgrade guidance:

* The Core API has been bumped to `0.39.3` as part of
  work-in-progress support for the `cl_intel_unified_shared_memory` OpenCL
  extension. This extension is disabled by default in CMake until complete,
  Core modifications include the following changes.
    * Forbid mapping already mapped memory objects with `coreMapMemory`.
    * `core_memory_property_host_visible` is now a required property of memory
      objects.
    * Devices are now required to report capability
      `core_allocation_capabilities_alloc_host` rather than
      `core_allocation_capabilities_use_host` with `coreCreateMemoryFromHost`
      entry point as this implies hardware has cache coherent memory with host.

Feature Additions:

* Continued development on OpenCL 3.0 prototype with additional features.
  * Implemented `clSetContextDestructorCallback` and `ctz` builtin.
* Added LLVM 10.0.1 to list of supported versions.

Non-functional changes:

* Continued the multi-stage refactoring of VECZ:
  * Refactored the use of the Simd width in vectorizer.
  * Separate Simd width analysis in its own file.
* Refactored `threadPoolProcessCommands` for improved readability and to
  eliminate some TSAN warnings.
* Added null properties test for `clCommandQueueInfoTest`.
* Bumped to latest OpenCL-Headers commit.
* Added `const` to VECZ `BuiltinInfo` and other internal VECZ classes.
* Enabled `CA_HOST_NUM_THREADS` in all build types, previously this was only
  available in Debug mode.
* VECZ PHI node strides are now analyzed in simple cases.
* VECZ will remove redundant items from the barrier struct. This gives a
  performance improvement in certain cases.
* Added a helper function that detects common access patterns to
  `spirv_ll::Builder` to optimize builtin global uses.

Bug-fixes:

* Fix for `clSetKernelExecInfo()` when SVM is not supported.
* Fix for `coreAllocateMemory()` alignment bug when using a user provided
  pointer by checking that it meets the requirements of the passed `alignment`
  parameter.
* Fix for precision of half `atan2pi` and `log1p`.
* Fix incorrect test output with `Precision_89_Half_atan2_zeros`.
* Fix to allow `aligned_alloc` to allocate one byte in UnitCL.
* Fix for host to ensure that `duration_query` in
  `threadPoolProcessCommands()` is updated when the
  `host::command_type_begin_query` and `host::command_type_end_query` commands
  are processed.
* Fix for unnecessary instantiation of allocas in structs with VECZ if they are
  non-varying.
* Replaced `//fall through` comments with `CARGO_FALLTHROUGH` and
  `LLVM_FALLTHROUGH` to ensure proper behaviour with `gcc`.

## Version 1.50.0 -- 2020-09-02

Upgrade guidance:

* The Core API has been bumped a major version to `0.39.0` as part of
  work-in-progress support for the `cl_intel_unified_shared_memory` OpenCL
  extension. This extension is disabled by default in CMake until complete,
  Core modifications include the following changes related to memory objects.
    * New `alignment` parameter to `coreAllocateMemory`. Minimum number of
      bytes to align to, a zero value preserves previous behaviour.
    * New `handle` member of `core_memory_s` to represent underlying memory
      address. For host visible memory this should be a pointer cast, and
      for device local memory a unique handle.
    * New `coreCreateMemoryFromHost()` entry point for using pre-allocated host
      memory on device. May be implemented in a similar fashion to
      `coreAllocateMemory()` with `core_allocation_type_use_host`, or
      stubbed out to return `core_error_feature_unsupported` if device does not
      support use of host memory.
  See the [Core changelog](doc/modules/core/changes.md) for details.

Feature Additions:
* Continued development on OpenCL 3.0 prototype with additional features.
* Implemented a series of Core changes for USM.
    * Added alignment argument to coreAllocateMemory to forward from CL
      allocation entry points.
    * `handle` member to `core_memory_s` representing a pointer handle which
      `clDeviceMemAllocINTEL` can return.
    * New entry point `coreCreateMemoryFromHost` to allow `clHostMemAllocINTEL`
      to create a device side memory binding for the host allocation.
    * Related spec and UnitCore changes, as well as changelog upgrade guidance.
* New SIMD Width Analysis process for VECZ will estimate SIMD width from live
  ranges. This is used whenever `vectorizer::vectorize()` does not provide any
  hint on the SIMD width, for example when the caller passes `0` as parameter.
  The value may be adjusted later inside the function `vectorizer::vectorize()`
  and therefore differ from actual value used. This should provide performance
  improvements in certain cases.
* Implemented `astype` using clang's `__builtin_astype` builtin.

Non-functional changes:
* Continued the multi-stage refactoring of VECZ:
    * Additional steps for stage 3 covered splitting of `vectorizer.cpp` and
      splitting out passes into different files.
    * Remove `AllocaWideningPass` which was not being used.
* Bumped OpenCL-ICD-Loader submodule to tip.

Bug-fixes:
* Fix for City Runner so that 100% of testing being skipped results in an exit
  code of `0` instead of `1`.
* Implemented various precision improvements in the half implementation of
  `asinh`, `acosh`, `sinh`, `sincos`, `tan` and `tanpi`.
* Fix for UnitCL kernels installation. When ComputeAorta is not the root of the
  CMake tree, files would get installed to the wrong directory.

## Version 1.49.0 -- 2020-08-04

Upgrade guidance:

* The Core API has been bumped and the executable flags related to
  `vectorization_order` have been renamed to `work_item_order` to better
  reflect reality.  See the [Core changelog](doc/modules/core/changes.md) for
  details.
* The OpenCL build flag `-cl-wfv-order` has been renamed to `-cl-wi-order` to
  better reflect reality, i.e. the flag can have an effect on programs
  independently of whether WFV is used.
* `utils::createHandleBarriersPass()` must now be passed a parameter of type
  `enum util_work_item_order_e` to specify the work item dimension priority.

Feature Additions:

* Continued to develop OpenCL 3.0 prototype with additional features.
* Add support to build for QNX including a toolchain for QNX x86_64 and
  associated fixes to UnitCargo. Added QNX macros and stub functionality to
  TracerGuard and `system.cpp` to support compilation with `qcc`.
* Add support to OpenCL-Intercept-Layer for dumping and injecting binaries.
    * To enable support enable both `CA_CL_ENABLE_ICD_LOADER` and the new
      `CA_CL_ENABLE_INTERCEPT_LAYER` option.
* Add `CA_HOST_TARGET_CPU` CMake option for enabling CPU specific codegen.
* Add a mechanism to skip a host lit test that fails when a target CPU for host
  is specified.
* Add working directory to PATH in City Runner so we can avoid specifying
  relative paths in the CSV files for `binary` and `spir-v` modes.
* Started laying the groundwork for the CL side of implementing
  `cl_intel_unified_shared_memory` USM extension. This includes extension
  documentation, API usage UnitCL test, stubbed implementation of entry points
  in CL and Host side allocations. This extension is disabled by default in
  CMake.
* Add `enqueue_range` to the `thread_pool` to allow an nd range worth of work
  to be enqueued on the thread pool in one go with out relinquishing the thread
  pool mutex.

Non-functional changes:

* Extended the Core documentation generation to include several CMake
  functions.
* Skip some UnitCL precision tests that won't work with 32-bit ARM.
* Converted `unitcl.md` to `unitcl.rst` and updated documentation to include
  diagrams of how the execution tests' kernels CMake works.
* Extend documentation of `Work Item Order` and `vec_order` parameter.
* Started the multi-stage refactoring of VECZ:
    * Stage 1 covered renaming and formatting of non-public source files. This
      includes removal of the `vecz_` prefix for all files and applying
      clang-format to the relevant files to bring things in line with the rest
      of the code base.
    * Stage 2 covered creating subfolders for internal files. The source
      directory will now be organised into public headers, private headers and
      private source.
    * Stage 3 covered splitting of `vectorizer.h` into `vectorization_unit.h`
      and `analysis_manager.h` and updated include accordingly.

Bug-fixes:

* Ensure that push constants can be set before a pipeline is bound or if
  multiple command buffers use the same pipeline. This fixes a failing IREE test
  with VK.
* UnitCL fix for relative path issue with kernels in Visual Studio builds.
* Fix `constexpr` problems with `cargo::optional` when compiling for QNX.
* Fix SPIR calling convention in `HandleBarrierPass`. The `HandleBarrierPass`
  was splitting kernels into new functions using the parent calling convention.
  This should not apply to `SPIR_KERNEL` functions which should be split into
  `SPIR_FUNC` functions.
* Disable `--vecz-check` when it is incompatible with `-cl-vec=all` on AArch64.
* Add a fix for detection of native binaries for QEMU based lit testing.
* VECZ fix for handling special edge cases where loads or stores with uniform
  addresses resulted in non-uniform masks and subsequently the associated load
  being marked as non-uniform.
* Implemented various precision improvements in the half implementation of
  `acos`, `cosh`, `tgamma`, `tanh`, `exp`, `expm1`, `sinpi`, `cospi`, `erf` and
  `erfc`.
* For lit test CMake, use `ca_add_copy_file()` instead of `*configure_file()`
  for lit tests to avoid unnecessary configuration.

## Version 1.48.1 -- 2020-07-16

Bug-fixes:

* Vectorization order preference option `-cl-wfv-order` is now parsed also
  during JIT compilation and not only during offline compilation.
* Vectorization order priority handling in `HandleBarrierPass` now works
  with any vectorization order preference.
* UnitCL test `ktst_vecz_tasks_task_13` has been rewritten to avoid concurrent
  writes during the reduction stage.
* VECZ fix for cases where uniform values were scaled unnecessarily.

Non-functional changes:

* Struct `kernel_s` in the `host` reference implementation now keeps track of
  the vectorization order preference specified at build time.

## Version 1.48.0 -- 2020-07-02

Upgrade guidance:

* `Vectorizer::Vectorize()` exposes an additional parameter to set the
  dimension index to vectorize on. Developers should expect the previous
  behaviour when passing `0` (x dimension). `createRunVeczPass()` similarly
  exposes an additional parameter with the same intended use.
* struct `core_executable_options_s` has an additional field of type
  `core_vectorization_order_e` that stores the vectorization order
  preferences.  Developers should expect previous behaviour when using the
  default value `core_vectorization_order_default`. This is an experimental
  feature and the naming is not yet final.
* `BuiltinInfo::isBuiltinUniform()` requires an additional parameter since the
  result may depend on the vectorization dimension being considered.

Feature Additions:

* Continued to develop OpenCL 3.0 prototype with additional features.
* The `host` reference implementation now supports vectorization on dimensions
  other than `x`.
* `veczc` now supports the additional command line option `-d {0|1|2}` to
  specify the vectorization dimension.
* `clBuildProgram()` accepts the vectorization order preference as optional
  parameter using the syntax `-cl-wfv-order={xyz|xzy|yxz|yzx|zxy|zyx}`.
  Default value is `xyz` and should reflect previous behaviour. Dimensions
  other than the first one are used only in `HandleBarrierPass`.
* Added SPIR-V 1.0 support to `clc`.
* Expanded UnitCL testing with offline SPIR-V variants.
* Added `Remove Intptr Pass` to VECZ for removing `PtrToInt` when a `PtrToInt`
  is followed by an `IntToPtr`, a `PtrToInt` used by a PHI node or a `PtrToInt`
  where the pointer type is `i8*` followed by an integer add or subtract.
* Added `-cl-wfv-order` build option extension for specifying the
  vectorization priority order. This is an experimental feature and the naming
  is not yet final.

Non-functional changes:

* Unify UnitCL device versioning checks `UCL::getNumericDeviceVersion()`,
  `UCL::isDeviceVersion()`, and `UCL::isDeviceVersionAtLeast3x()` into
  `UCL::isDeviceVersionAtLeast()` with the help of the new `UCL::Version` type.
* Refactor VECZ `vecz_vectorizer.cpp` and separate some of the passes logic into
  `vecz_passes.cpp`.
* For lit tests, we now copy the files to the build directory using the new
  `add_ca_copy_file()` rather than `configure_file()` or
  `add_ca_configure_file()`. This stops unnecessary CMake reruns when making
  changes to `host` and `vecz`.
* `CA_DISABLE_DEBUG_ITERATOR` now defaults to `OFF` by default.
* Refactor the offline kernel compilation CMake pipeline. `.cl` files will
  trigger rebuilds as before but will be reparsed to avoid being compiled with
  stale options. Any kernels that don't need building will have stub files
  generated. This allows `regenerate-spir-spirv` to track dependencies to avoid
  rebuilding kernels unnecessarily. Compiling IR to `.bin` files is now aware
  of requirements so we no longer attempt to compile IR that we aren't able to.
* Bump ICD commit and pull latest OpenCL-Headers.
* Added UnitCL `uint2`, `long2` and `ulong2` validators.

Bug-fixes:

* VECZ fix where a zero-extended offset could cause an underflow and result in
  an invalid base address. This could cause the strided `MemOp` to become invalid.
* VECZ will avoid generating redundant shifts/divides during memory stride
  calculation.
* Fix gcc-9 only compilation issue where a redundant move is required when
  returning a `cargo::expected` object holding a `command_queue`.
* Fix `clc` executable path when generating UnitCL tests when cross-compiling
  in offline-only mode.
* Fix command queue extension properties by restoring the call to
  `extension::ApplyPropertyToCommandQueue()` when creating a `cl_command_queue`
  from a call to `clCreateCommandQueueWithProperties` or
  `clCreateCommandQueueWithProperitesKHR`.

## Version 1.47.0 -- 2020-06-02

Upgrade guidance:

* None.

Feature Additions:

* Continued to develop OpenCL 3.0 prototype with additional features.
* Added preliminary support for LLVM 11.
* `oclc` now supports loading SPIR-V.

Non-functional changes:

* Removed Vulkan `Doxygen.cmake` and `Sanitizer.cmake` modules since they are
  unused.
* Increased documentation on CMake and VECZ.
* Refactor some uses of `llvm_unreachable`.
* Added `multi_llvm` module to help group different fixes and workarounds for
  specific versions of LLVM in a single place. This avoid duplication of the
  same fixes in different files.

Bug-fixes:

* Fixes for UnitCL with MinGW including addressing a MinGW specific memory leak
  and making cross-process temporary file names safer.
* Fixed an issue where `clBuildProgram` would fail to find a definition for
  `printf.1` when compiling SPIR-V with multiple printf calls.
* Refactor UnitCL softmath tests to omit ULP calculation from the validation of
  native maths builtin results to avoid undefined behaviour.
* `host-offline` CMake target now respects `CA_ENABLE_HOST_IMAGE_SUPPORT`,
  `CA_HOST_ENABLE_FP64` and `CA_HOST_ENABLE_FP16`.
* Fix for supporting `printf` in SPIR and SPIR-V kernel tests.
* Fix long divides on 32-bit offline builds by adding relocation for `__divdi3`.
* Fix for `cargo` to support C++14, used when linking against LLVM 10+.
* Fix for VECZ handling of DXIL thread ID.
* Fix for incorrect assumption with branches in BOSCC uniform blocks from
  non-div-causing blocks
* Host specific UnitVK tests will now check for ComputeAorta device during
  setup.

## Version 1.46.1 -- 2020-05-28

Feature Additions:
* VECZ now supports being built standalone as a static library.

Bug-fixes:
* ASHR underflows are accounted for with VECZ by applying byte size strides at
  GEP level.
* Fix VECZ triggering UBSAN by ensuring bit-shift check is done as 64-bit.
* When testing ComputeAorta through the ICD loader `OCL_ICD_VENDORS` will be
  set to `/dev/null` to ensure that ComputeAorta OpenCL is the only runtime
  picked up.
* Fix MSVC 2015 compilation issues related to defaulted constructors.

## Version 1.46.0 -- 2020-05-13

Upgrade guidance:

* 32-bit Windows builds are no longer supported, CMake will now print an error
  if you attempt to build this configuration. If this breaks an active use-case
  for you then let the ComputeAorta team know as soon as possible.

Feature additions:

* OpenCL 3.0 prototyping has advanced significantly, but this is not present
  in all releases.
* LLVM 10.0 is now fully supported, ComputeAorta may be linked against it.
* VECZ will now handle early-return branches in kernels specially when BOSCC is
  disabled. This can make a particularly large difference to performance for
  kernels that follow this early-return pattern. It is essentially a vastly
  simplified version of BOSCC, called ROSCC for "return on super-condition
  code". It respects the early-return if the entire vector super-condition
  will take it, because otherwise linearization forces the removal of the
  early-exit.

Non-functional changes:

* Windows builds of ComputeAorta now respect the `CA_HOST_ENABLE_FP64` CMake
  option, but the default is unchanged. I.e. doubles are still disabled by
  default for `host` on Windows builds.
* Refactor how `cl_program` code handles the five possible input sources from a
  monolithic implementation to a class hierarchy based one.
* The CL `cl_kernel` code no longer uses raw `new`/`delete` preferring `cargo`
  data structures instead.
* The ELF loader now has pre-written machinery for replacing relocations of
  external functions in debug builds, and in such builds `memcpy` and `memset`
  relocations are replaced with calls to variants of those functions with some
  bounds checking.
* UnitCL ULP calculations now match the OpenCL CTS for `float` and `double`
  types. No implementation code had to be changed to match this change.
* Remove some code for older unsupported LLVM versions.
* Some of our custom CMake modules are now documented.
* Various headings and cross-links in our documentation have been tidied up.

Bug-fixes:

* Fix issue where fp16 half to integer saturated conversions of negative values
  could result in the wrong value.
* Fix issue where VECZ was not deinterleaving some masked loads when it should.
  This did not affect correctness, but was suboptimal.
* Fix issue where VECZ could cause guarded divides by non-constant values to
  become unguarded during pre-linearization. Specifically this meant that
  divides by zero could be introduced.
* Fix issue in VECZ BOSCC where earlier optimizations could cause loops to be
  bypassed, but BOSCC did not blend that bypass correctly.

## Version 1.45.0 -- 2020-04-08

Upgrade guidance:

* Changes to OpenCL CMake utilities:
  * All OpenCL example applications which belong to a Core target should be
    added to the build using the `add_ca_example_subdirectory()` command to
    delay creation of the target until after the `source/cl` build tree has
    been processed, check that OpenCL is enabled using `if(CA_CL_ENABLED)`,
    and `include(${ComputeAorta_SOURCE_DIR}/source/cl/cmake/AddCACL.cmake` in
    the example `CMakeLists.txt` to access the OpenCL CMake options, commands,
    and pre-processor definitions. For an example integration, see
    `modules/core/source/host/extension/example/CMakeLists.txt`.
  * All targets which link OpenCL should use the `add_ca_cl_executable()` and
    `add_ca_cl_library()` commands for creation to automatically link either
    the `CL` target or `OpenCL` target when the OpenCL-ICD-Loader is enabled
    by setting the option `CA_CL_ENABLE_ICD_LOADER=ON`.
  * All check targets which use OpenCL should use `add_ca_cl_check()` command
    to automatically set the required environment variables when the
    OpenCL-ICD-Loader is enabled by setting the option
    `CA_CL_ENABLE_ICD_LOADER=ON`.
* The upstream Khronos OpenCL headers have been updated to the latest
  versions. Note: This means that the header license has changed from MIT to
  Apache 2. See the [license file](source/cl/external/OpenCL-Headers/LICENSE)
  for details.

Feature additions:

* There is now a `CA_CL_STANDARD` CMake option. This has a default value of
  1.2, and currently no other value should be expected to work.  OpenCL 3.0 is
  under active development, but will not be present in all releases.
* VECZ will now hoist control-dependent instructions, if it is safe to do so,
  when the branch will be more expensive than the instruction after
  vectorization.
* The `host` reference implementations of OpenCL and Vulkan will now report
  `CL_KHRONOS_VENDOR_ID_CODEPLAY` or `VK_VENDOR_ID_CODEPLAY` as the vendor ID
  (these two enums are the same value).

Non-functional changes:

* The upstream Khronos Vulkan headers have been updated to the latest
  versions.
* ComputeAorta's CMake structure is now described in the documentation.
* UnitCL now has `PlatformTest`, `DeviceTest` and `ContextTest` generic
  fixtures to reduce the amount of boiler-plate setup for future tests that
  will require such functionality.
* Many additional UnitCL tests have been enabled, either due to bug fixes or
  due to supporting more modes of test execution.
* Make the code clean with the following additional clang-tidy checks:
  bugprone-unused-return-value; bugprone-macro-repeated-side-effects;
  bugprone-forward-declaration-namespace; bugprone-integer-division;
  bugprone-misplaced-widening-cast.
* Make City Runner output for skipped tests cleaner.
* Move the debug backtrace functionality into its own target,
  `debug-backtrace`, so that it can be used in a compilerless configuration.
* If the `CA_ENABLE_CARGO_INSTRUMENTATION` option is set then backtraces of
  points where `cargo::small_vector` exceeds the small buffer storage are
  printed.

Bug-fixes:

* Fix various ELF relocation issues affecting 32-bit x86, Armv7, and AArch64.
  These generally exhibited as offline-compiled kernel binaries crashing.
* OpenCL extensions that require compiler support (`cl_khr_spir`,
  `cl_khr_il_program`) will not be reported when a compiler is not available.
* Store the values of the `reqd_work_group_size` attribute in the kernel
  metadata during offline compilation so that it can be respected properly
  when loading the binary later.
* Legacy SPIR kernels using the `readnone` attribute will no longer miscompile
  as we change the `readnone` to `readonly` when we consume the SPIR.
* Correct a miscompilation of an AArch64 reduction intrinsic in VECZ that
  could cause crashes.
* In the reference implementation of the asynchronous work group copy
  functions only the first work item now does the copy, eliminating the
  possibility of values being overwritten by other workitems.
* UnitCL will now complain about an illegal `--unitcl_device=x` option
  for the single device case.

## Version 1.44.2 -- 2020-03-25

Feature additions:

* Add a utility function to mutate scalar loop induction variables in partial
  vectorization. This allows customer targets to implement their own partial
  vectorization aware unrolling when scheduling work-items.

Bug-fixes:

* Do not assume, in the barrier pass, that all call sites have a name.
  Although this is true for OpenCL C, SPIR, and SPIR-V input, if a customer
  target injects assembly via a compiler pass this will not have a name.
* Address a potential `nullptr` dereference in VECZ that Klocwork pointed out.

## Version 1.44.1 -- 2020-03-13

Feature additions:

* Improve the quality of the IR used for condition reductions in VECZ BOSCC.

Bug-fixes:

* Correct an `x86_32` relocation in the ELF loader that caused some kernels
  which called external functions to have illegal memory accesses.  This was
  actually quite rare due to aggressive inlining, but cases where the loop
  optimizer inserted memory copy intrincsics between memory address spaces were
  affected.
* Preserve all IR flags when VECZ packetizes comparison instructions, e.g.
  related to fast-math style settings.
* Improve accuracy of fp16 `hypot` function to match specification requirements
  for edge-case conditions.

## Version 1.44.0 -- 2020-03-03

Feature additions:

* Partial vectorization performance overhead has been reduced by moving the
  conditions check outside the z-loop.
* Masks generated by CFG conversion have been simplified.
* Improve `BOSCC` performance with various kernels by applying a heuristic that
  rejects BOSCC regions if all blocks contain a small number of side-effect free
  instructions.
* Barriers in Core are now marked as convergent.

Non-functional changes:

* Update the `build.py` script to support `MinGW` builds.
* Default `clang-tidy` and `clang-format` has been moved to 9. As a result the
  `CA_CLANG_TIDY` define has been removed as it is no longer needed.
* Support `clang-tidy-10` by disabling certain checks.
* Various other `clang-tidy` related changes including enabling some LLVM and
  googletest specific checks and enabling
  `readability-braces-around-statements`,
  `readability-redundant-access-specifiers`, `cert-dcl58-cpp`, `cert-dcl16-c`
  and `performance-for-range-copy`.
* The CFG Analysis Pass and BOSCC are now in their own files.
* Building with `clang-10` is now possible.
* Half ULP UnitCL error algorithm has been refactored to support return `-INF`.

Bug-fixes:

* Add `EarlyCSE` as a VECZ preparation pass to prevent `IndVarSimplify pass`
  causing excessively slow compilation with certain test cases.
* Fix an issue where VECZ would insert instructions before allocas that would
  cause issues with the barrier pass.
* VECZ will no longer illegally move PHI nodes to the middle of a basic block
  when reducing register pressure.
* Fix half precision UnitCL tests including `mix()`, `smoothstep()`,
  `degrees()` and `radians()` where references were incorrect.
* Skip `Precision_90_Half_Ldexp_Edgecases` where devices do not support
  denormals.
* Modified abacus `sinh()` for half to return `-INF` correctly.
* Disable LLVM `FRem` instruction from being generated with `spirv-ll` on
  32-bit ARM.
* Fix VECZ reachability issue which could cause NULL pointer to be returned and
  dereferenced.
* Fix map/unmap flushes in user callback commands. Where a command-group
  contains a read-unmap immediately followed by a write-map, flushing from
  device to host was not being performed.

## Version 1.43.0 -- 2020-02-11

Feature additions:

* Addition of support for LLVM version 9.0.1 release.
* There are two new Core module builtins, `__core_usefast()` and
  `__core_isembeddedprofile()`, and there is a new floating point capability
  flag that devices can set if they are fully IEEE-754 conformant.
* Add hooks for custom cost-models to decide when to use loop-vectorization
  and/or VECZ and/or each VECZ optimization choice.
* The barrier pass has had a partial rework, there should not be any functional
  changes, but the struct that stores state across barriers should now be
  smaller.  This improves compile time and memory use.
* Use `const` attribute, instead of `pure`, for builtin functions that do not
  access memory.

Non-functional changes:

* Rewrite VECZ reachability analysis, the only external effect of this should
  be improved VECZ compile time for kernels with complex control flow.
* If a Khronos SPIR generator is specified at configure time then it will be
  tested to ensure that it is configured as we expect (with asserts enabled, to
  ensure consistently formatted IR).
* Automatically skip UnitCL offline tests for common third-party OpenCL
  implementations used for cross-verification.
* The code-base is once again clang-tidy-9 clean, partially via disabling more
  checks in the configuration file.
* Update documentation from referencing Ubuntu 16.04 to Ubuntu 18.04.

Bug-fixes:

* The OpenCL `v{load,store}_half` functions once again use a software
  implementation for FULL profile on FTZ hardware, as FTZ hardware does not
  match the OpenCL spec.
* VECZ now marks reduction intrinsics, which may be introduced by LLVM loop or
  SLP vectorizer, as needing scalarization as they are not vectorizable.
* Work around a bug in the legacy Khronos SPIR generator that prevented one of
  our tests compiling to SPIR.
* Fix some uninitialized variable warnings that seem to only occur on a single
  specific configuration (GCC 7.4 RelWithDebInfo).
* Fix undefined-behaviour due to under-aligned allocation.

## Version 1.42.1 -- 2020-01-24

Non-functional changes:

* Enable most LLVM lit tests for all LLVM versions, previously we disabled them
  on all versions except 8.0.
* Fix some mismatch of `_chktsk` vs `__chkstk` symbols in the ELF loader, but
  this did not affect any known programs.
* Split up `ktst_regression.cpp` to avoid MinGW debug builds crashing, due to
  the number of symbols in that file, while linking UnitCL.

Bug-fixes:

* Fix MinGW release assert builds where some incorrect flags broke linking.
* Support MinGW builds with LLVM 8+ by matching the changed `LARGEFILE_SOURCE`
  and associated flags.
* Don't setup `_chkstk` or `__chkstk` symbols for MinGW builds.
* Improve precision of fp16 `ldexp` function due to issues found by the
  recently added OpenCL `fp16` CTS test cases.
* Fix fp16 `lgamma` for the input `0.0` and a case were we returned `NaN` but
  should have return `Inf`.
* Fix a rounding edge-case in the UnitCL reference `mix` function, but we
  currently disable these tests as we seem to be stricter than the CTS.

## Version 1.42.0 -- 2020-01-07

Feature additions:

* The Core API has been extended such that various fast math properties are
  reported on creating an executable. Note that the CL frontend already does
  its own optimizations based on this information, including setting the
  appropriate flags on the instructions in the IR, so it is possible that a
  Core target does not need to do anything else. It is also possible, however,
  that some backends may benefit from knowing that these optimizations are
  possible globally.
* The OpenCL ICD loader can now be optionally included in the ComputeAorta
  build and tests will automatically link-to and run-through the ICD loader.
* The BOSCC mode within VECZ now creates far fewer Phi nodes. Mostly this
  reduces compile-time in large kernels with complex control-flow.

Non-functional changes:

* Re-unify the UnitCL `Execution` test naming. Tests that recently had `Gen`
  appended to their name have now had that removed again.
* SPIR and SPIR-V tooling requirements for generation have now been documented
  and files have been regenerated with the specified versions of these tools.
* The many additional variants of `UnitCL` run by the `check` target now only
  run tests that match common patterns for compiler tests. This reduces the
  duplicate runs of the non-compiler tests that were testing identical
  behaviour for every variant.
* Running the `check` target in sanitizer configurations now automatically sets
  the environment variables to the suggested configuration.
* If the ELF loader cannot resolve a symbol then when in a debug build it now
  prints the name of that symbol.
* Add SPIR, SPIR-V and offline modes to more UnitCL Execution tests.

Bug-fixes:

* Fix an issue where if two kernels are contained in a program, and they both
  use local memory, then memory corruption may occur as only the first kernel
  was compiled correctly.
* Fix an issue with the fp16 implementation of `pown`. For the case of
  `pown(INFINITY, 0)` we were returning `0` instead of `1`.
* Fix an issue where fp16 `atan2` and `atan2pi` weren't matching the specified
  outputs for various combinations of +0.0 and -0.0.
* Fix an issue where offline compiled kernels for ARMv7 that required
  `__fixdfdi` or `__floatdidf` segfaulted because our ELF loader did not map in
  these functions.
* Close a few memory leaks in UnitCore tests when the tests were skipped.

## Version 1.41.0 -- 2019-12-03

Feature additions:

* Auto-generate a `UCL::isDevice_${core_target}` function to aid customer teams
  with writing tests that are device specific.
* Replace `noduplicate` attribute for `barrier` type functions with `convergent`
  attribute. Removes `CA_DISABLE_NODUPLICATE` CMake flag added in version 1.40
  as this is now standard behaviour. Utility pass `RemoveNoDuplicates` is also
  deleted from the code base as we no longer set this in the builtins header.

Non-functional changes:

* Update CMake so that ComputeAorta can be built with an undefined behaviour
  sanitizer.
* Run clang-tidy on offline-compiler library files in any online-compiler build
  where the offline libraries are also built.
* Delete the old `configuration.py` scripts and dependencies, as far as we know
  no one is using these anymore.
* Reduce the clang-tidy time of some heavily parameterized tests by only
  analyzing a single parameter configuration.
* Correct rendering of some markdown tables in the documentation.
* Add more `-cl-opt-disable` UnitCL test configurations to the extended set of
  `check` tests.
* UnitCL now respects the `CA_CL_ENABLE_OFFLINE_KERNEL_TESTS=OFF`
  configuration.
* Regenerate a few SPIR-V assembly tests with a more recent spirv-tools.
* Remove some legacy performance calculation scripts.
* Remove most references to internal services.
* Improve documentation on regenerating SPIR & SPIR-V kernel binaries used for
  testing.
* Vulkan ICD manifest has been added to the `install` build target and now
  uses a relative path to the VK shared library.

Bug-fixes:

* Always translate SPIR-V `OpTypeImage` as a pointer in spirv-ll.
* Don't do 64-bit aligned loads when parsing SPIR-V when we can't guarantee
  that the pointer is 64-bit aligned.
* Fix some command line argument parsing bugs on custom options introduced by
  Core targets.  This is mainly to address usability issues in clc and oclc.
* The clc tool no longer adds a spurious space to the values provided to the
  `-S` or `-x` options.
* Fix two minor overflow issues in UnitCL tests that were caught by the
  undefined behaviour sanitizer.
* Fix a memory leak in the FuzzCL test runner.
* Use correct library name for offline build OpenCL export definitions on
  Windows, which should have an `-offline` suffix.
* Update UnitCL test `barrier.13_barrier_shift_loop` to use global memory
  fences rather than local, as the test is operating on global memory not
  local.

## Version 1.40.3 -- 2019-11-29

Feature additions:

* Expose the VECZ options controlled by the `CODEPLAY_VECZ_CHOICES` environment
  variables through the pass interface so that Core targets may set them
  programmatically.

Non-functional changes:

* We no longer require external builtins, or tools to build builtins, to build
  an LLVM-less configuration.

Bug-fixes:

* Fix an issue in VECZ where a loop inside a divergent loop could be considered
  non-divergent because it didn't diverge itself, but everything inside a
  divergent loop is divergent.
* Fix an issue where normalization inside the CPU barrier pass could crash when
  it processes a phi node with no uses.
* Use LLVM's mem2reg pass inside of VECZ, to help canonicalize the IR.  This
  fixes an issue where using `-cl-opt-disable` in combination with
  `-cl-wfv=always` resulted in incorrect code.
* Ensure that the OpenCL library version is updated in line with the current
  source code, rather than the point that a build directory was initially
  configured.

## Version 1.40.2 -- 2019-11-13

Feature additions:

* In the OpenCL implementation, when parsing OpenCL C code, the Clang frontend
  optimization level is now set at `-O3` instead of `-O0`.  Note that although
  this is a small change it has wide-ranging effects on the IR produced by
  Clang, and could expose latent compiler bugs.

Bug-fixes:

* Fix an assertion trigger in VECZ when trying to split a list that happened to
  already be sorted, i.e. where one side of the split would be empty.
* Fix an issue in the Auto DMA pass where a loop backedge count was
  insufficient to determine the size of the DMA operation due to the loop-exit
  block not dominating a memory operation.  This was exposed by using the loop
  rotation transformation, but could in principle result from hand-written
  code.
* Fix an issue in the Auto DMA pass where a function's argument being unnamed
  resulted in a failed lookup.
* Initialize some member variables in a UnitCL test that would result in
  spurious test failures on devices where the tests were skipped.

## Version 1.40.1 -- 2019-11-07

Non-functional changes:

* Add CMake option `CA_ENABLE_OFFLINE_LIBRARIES` to enable or disable support
  for online and offline libraries in the same build.

## Version 1.40 -- 2019-11-05

Feature additions:

* It is now possible for Core implementations to expose their own compiler
  build flags through OpenCL `clCompileProgram`, `clBuildProgram` and `clc`
  paths.  See the Core API changelog for details.
* The OpenCL `-cl-no-signed-zeros`, `-cl-finite-math-only` and
  `-cl-unsafe-math-optimizations` are now connected to Clang language options
  and thus may have a greater effect than previously.
* Implement the SPIR-V `SPV_KHR_no_integer_wrap_decoration` extension. Note
  that this means that if using the Vulkan SDK to provide spirv-tools then the
  minimum version of the Vulkan SDK required has been raised to 1.1.97.
* We are investigating stopping the use of the `noduplicate` attribute for
  `barrier` type functions, and only relying on the `convergent` attribute.  As
  this may have an effect on optimization passes, there is an experimental
  `CA_DISABLE_NODUPLICATE` flag that eliminates the `noduplicate` attribute
  from OpenCL C inputs, but has no effect on SPIR or SPIR-V inputs.  Using this
  flag should not affect correctness, but may affect compiler optimization
  decisions.
* Support online and offline libraries in the same build, this introduces the
  `CL-offline`, `host-offline`, `core-offline`, `builtins-offline`,
  `compiler-offline`, `extension-offline`, and `CL-offline` build targets. Core
  targets can be added to the `CL-offline` using the `add_ca_offline_library`
  CMake utility function. This feature is disabled when
  `CA_RUNTIME_LIBRARY_ENABLED=OFF`.

Non-functional changes:

* Update to the latest Khronos OpenCL headers. Note that because these are
  unified headers that support all OpenCL versions, these updated headers will
  cause messages to be printed when building OpenCL programs unless
  `-DCL_TARGET_OPENCL_VERSION=120` is set while building that program.
* Update to the latest Khronos SPIR-V headers.
* The `CA_ENABLE_TESTS` CMake option now actually enables or disables all
  testing, and a new `CA_ENABLE_EXAMPLES` CMake option has been added to do the
  same for sample code.
* The `clGetImageInfoParamTest` tests now use much less memory, this is
  especially helpful if running `UnitCL` in qemu.
* It is no longer possible to build `UnitCL` separately from the rest of
  ComputeAorta, as no one was using this build configuration.
* The OpenCL-specific compiler module is now built as a static library as it
  was never intended to be built as a shared library.
* Regenerate some UnitCL SPIR-V kernels with a more recent `llvm-spirv`.
* Fix various Klocwork issues at severity levels 2-4.
* The `clc` tool will now use the input's file name in compiler error messages.
* A simple `strip-header` script has been added to remove the metadata from an
  OpenCL offline kernel, e.g. as produced by `clc`.
* A simple `parallel.py` script has been added. It is useful for parallel
  testing use cases with simpler requirements than use cases which require City
  Runner.
* In builds configured to be able to regenerate SPIR and SPIR-V, dependencies
  have been adjusted so that there should not be unintended regenerations while
  building, nor unintended deletions while cleaning.
* The code-base is now clang-tidy-9 clean, partially via disabling some of the
  new checks in the configuration file.

Bug-fixes:

* Fix various VECZ issues uncovered by using both loop-vectorization and
  packetization without scalarization together.
* Fix a VECZ issue where loop-vectorization and VECZ interaction resulted in
  incorrect divergence analysis.
* Fix a VECZ issue where true infinite loops, i.e. those with no exit edge,
  were incorrectly vectorized. Although it is not possible to write a useful
  version of such a loop in the input source, loop unswitching may turn a loop
  that is maybe infinite into a loop that is definitely infinite but maybe
  never executed. This definitely infinite loop caused vectorization issues.
* Fix a VECZ issue where an `alloca` is initialized with a uniform value and
  thus not widened but is then used in a widened memory operation.
* Fix an issue where setting an internal debug support build flag caused `clc`
  to unintentionally output intermediate files.
* Close a memory leak in the `vkVectorAddition` Vulkan sample code.

## Version 1.39.1 -- 2019-10-08

Feature additions:

* VECZ now supports partial vectorization by creating a peeled loop for
  non-divisible vector widths, the new feature is enabled using the
  `PartialVectorization` choice.

## Version 1.39 -- 2019-10-01

Feature additions:

* VECZ now supports packetization without scalarization by duplication of vector
  instructions, the new feature is enabled using the `PartialScalarization`
  choice.
* VECZ now supports the DXIL `getDimensions` intrinsic.
* VECZ now supports the DXIL bitcast operations.
* Enhance `run_cities.py` test runner to support execution across multiple
  devices connected via SSH.
* Change the default name for the OpenCL library from
  `libOpenCL.so.1.2`/`OpenCL.dll` to `libCL.so.1.39`/`CL.dll`, additionally
  provide two new CMake options to control the library name `CA_CL_LIBRARY_NAME`
  and library version `CA_CL_LIBRARY_VERSION`.

Non-functional changes:

* Internal clone of the Khronos Group Vulkan-Headers repository found at
  `source/vk/external/Khronos` has been updated to the most recent version.
* Disable two UnitCL half precision tests,
  `HalfMathBuiltins.Precision_08_Half_Ldexp` and
  `HalfMathBuiltins.Precision_82_Half_pown`, pending future fixes to corner case
  failures resulting from random input data.
* CMake build system now distinguishes between OpenCL runtime and compiler
  customer extension integration.
* Extend UnitCL kernel execution testing previously limited to OpenCL C sources
  to SPIR, SPIR-V, and offline compiled binary inputs.
* Remove arbitrary limit on the number of possible `cl_event` callbacks.

Bug-fixes:

* Fix Klocwork `UNINIT.STACK.MIGHT` issues.
* Fix Klocwork `SV.TAINTED.PATH_TRAVERSAL`, `NPD.CHECK.MIGHT` issues.
* Fix Klocwork issue where `cargo::error_or` was missing assignment operators.
* Fix a variety of Klocwork severity 2 issues.
* Fix a variety of Klocwork severity 3 issues.
* Fix an undefined-behaviour issue in math builtins using local pointers.
* Fix loss of precision in `lgamma_r` intermediate computation.
* Fix UnitCL `stdout` redirection issue for offline-only Windows release with
  assertions builds.
* Fix `run_cites.py` google test runner profile to output test results to a
  single XML file.
* Fix FuzzCL destructor threading issue and continuous integration script.

## Version 1.38.1 -- 2019-09-12

Bug-fixes:

* FuzzCL now respects `CA_CL_ENABLE_OFFLINE_KERNEL_TESTS`.
* Resolve numerous issues throughout the codebase where a `nullptr` may have
  been dereferenced.
* Resolve issues throughout the codebase where a function may have returned
  a reference to a function-local variable.
* Identify Klocwork false positives with an in-code `KLOCWORK` comment.
* Minor stylistic improvements throughout to improve Klocwork linting.

## Version 1.38.0 -- 2019-09-03

Feature additions:

* UnitCL now supports offline compilation testing for multiple device targets.
* Introduce optional pre-vectorization LLVM passes in Host. Includes Loop
  vectorization, SLP vectorization and Load/Store vectorization passes.
* Extend City Runner scheduling to group tests into task pools based on resource
  usage, allowing more granular control of test concurrency.
* Introduce new OpenCL runtime fuzzing tool FuzzCL, utilising LLVM's coverage
  guided fuzzing library `libFuzzer` to search for data races and other
  threading issues. Our FuzzGenCorpus binary can first be used to generate a
  corpus of runtime calls that will seed the random mutations needed by FuzzCL,
  leading to more efficient fuzzing.

Non-functional changes:

* Remove full versions of `conformance_test_integer_ops` and
  `conformance_test_thread_dimensions` tests from wimpy spreadsheets.
* Explicitly set SPIR-V version to 1.0 when generating `.spv`.

Bug-fixes:

* `half` precision UnitCL Execution tests now pass with VECZ enabled.
* VECZ now handles `unreachable` LLVM instructions.
* Update `g++` version check for `-faligned-new` support to greater than 7.0.
* Create AArch64 specific VECZ lit tests for shuffled load.
* Use correct binary type in UnitCore query pool test.
* Various Sphinx fixes to documentation HTML build.

## Version 1.37.1 -- 2019-08-27

Non-functional changes:

* Change documentation theme.
* Remove LLVM 4 check for `check-UnitCL-opt-disable-debug`.

Bug-fixes:

* Refactor BuiltinSimplification `sin`/`cos`/`tan` lit tests.
* Fix indices type for vectorized work group calls in VECZ.
* Explicitly create the `spir` directory to fix Makefile builds.
* Fix `pow`/`powr` single precision bug.
* Skip building any kernels for the cross target in UnitCL.

## Version 1.37.0 -- 2019-08-08

Feature additions:

* Qemu can now be used for running the various `check` targets for
  cross-compile builds.
* Qemu is now used if available to run `clc` to generate compiled versions of
  offline compile tests, meaning that providing an external native `clc` is now
  optional for cross-compile builds.
* The `vload_half`, and `vloada_half` functions will now use native fp16
  conversion functions on architectures that support fp16, while continuing to
  use the software emulation for architectures that do not.
* Core targets can now add additional BenchCL dependencies via the
  `${CoreTargetName}_EXTERNAL_BENCHCL_DEPS` CMake variable.
* We now enable fp16 instructions on 32-bit ARM `host` builds.

Non-functional changes:

* The `TEST_F_EXECUTION` macro is now considered stable, it automatically
  creates a `TEST_F` version of a test for standard execution, offline compiled
  execution, SPIR execution, SPIR-V execution and offline compiled SPIR
  execution cases.  It does not yet support `TEST_P` parameterized tests.
* Various tests have been converted to use `TEST_F_EXECUTION` where possible,
  but this work is not yet complete.
* Eliminate the `HOST_IMAGE_SUPPORT` and `CA_ENABLE_IMAGE_SUPPORT` support
  variables and replace them with `CA_ENABLE_HOST_IMAGE_SUPPORT`.  Other Core
  targets control image support via their own mechanisms.
* VECZ compiler statistics now record more possible vectorization failure modes.
* Python build scripts now all support Python3.  Python2 is still supported.
* The code-base can now be built with Visual Studio 2019.
* The code-base is now clean with `clang-tidy-8` using the provided
  `.clang-tidy` configuration.
* Document how to use the "Intercept Layer for OpenCL" as we are trialing using
  it for some testing.
* Various missing dependencies between tidy targets and generated code have
  been added.
* XML test outputs created by the `check` target are now removed by the `clean`
  target for CMake configuration that support this, i.e. either when using
  Makefiles or if using CMake 3.15 or above.

Bug-fixes:

* ComputeAorta will now accept and build the string `""` as an OpenCL program.
* SPIR-V consumption now handles variable width literals in `OpSwitch`.
* SPIR-V consumption now respects `OpExecutionMode ContractionOff` to the
  extent that is feasible with LLVM IR.
* SPIR-V consumption now implements `OpLogicalEqual` and `OpLogicalNotEqual`
  correctly.
* SPIR-V consumption now mangles mathematical builtins that take a signed int
  as an additional operand correctly.
* Correct some issues where the implementation for
  `cl_khr_create_command_queue` extension did not return the correct error
  codes.
* Correct some of the test cases for the `cl_codeplay_performance_counters`
  extension that did not match the intended functionality.
* Corrected build configuration issue that caused 32-bit x86 builds to have
  `x86_64` in their `CL_DEVICE_NAME` value.
* Fix a segfault in BenchCL due to using memory after it was out of scope.
* VECZ passes can now be created with a null `TargetMachine` again.
* The use of recursion in OpenCL kernels is not allowed, but VECZ would enter
  an infinite loop when presented with one.  It now detects the recursion and
  return an error code when building the program.
* The use of irreducible control flow in OpenCL kernels is implementation
  defined, but VECZ would enter an infinite loop when presented with this.  It
  now detects the control flow pattern and rejects vectorizations.
* Fix a segfault within internal debug functionality, that is not included in
  release builds, when presented with an OpenCL kernel that calls an undeclared
  function.

## Version 1.36.0 -- 2019-07-10

Feature additions:

* Add support for queries allowing to report performance statistics to
  applications.
* Add support for query counters.
* Add support for the `cl_khr_create_command_queue` OpenCL extension.
* Core now accepts 3D descriptions of memory in the
  `corePushWriteBufferRegions`, `corePushReadBufferRegions` and
  `corePushCopyBufferRegions` entry points. This allows implementations more
  flexibility when leveraging their own DMA hardware.
* Define and implement the `cl_codeplay_performance_counters` extension
  defining an OpenCL API providing the application access to performance
  counters.

Non-functional changes:

* Dissociate Arm and AArch64 in the VECZ `TargetInfo` class.
* Explicitly disable `cl_khr_int64_base_atomics` and
  `cl_khr_int64_extended_atomics` OpenCL extensions as they are not supported.
* Move VECZ lit tests into the `modules/vecz` directory.
* Gather input lit files with their associated filecheck tests.
* Move lit tests into the host directory as they are only used with a host
  build.
* Prevent reconfiguring a build from an offline build to an LLVM build or
  vice-versa.
* `vkCmdCopyBuffer` now uses `corePushCopyBufferRegions` instead of
  `corePushCopyBuffer`.
* Extend cross-compiling documentation in `developer-guide.md`.
* Extend the UnitCL Execution tests to automatically generate SPIR, SPIRV and
  Offline tests. This is still a work in progress and the new test system may
  still change in the future.
* Remove `aorta_pulse` tool and `test.sh` jenkins script.
* Globally rename `ARM` to `Arm`.
* Unify OpenCL platform and device naming for native and cross builds.
* Use `DeviceTest` fixture in UnitCore tests reducing code duplication.
* Add a `cargo` free function `as` allowing to create standard arrays and
  vectors from `array_view`s.
* Cross-compiling offline kernels is now tested. Consequently,
  `-DCA_EXTERNAL_CLC=<path>` is required for cross builds. Alternatively,
  `-DCA_CL_ENABLE_OFFLINE_KERNEL_TESTS=OFF` can be set.

Bug-fixes:

* Check buffer alignment when calling `clCreateSubBuffer`.
* Fix half `atan2pi` denormal issue by adding intermediate expression when FTZ
  expression is below a certain threshold.
* Fix half `erfc` denormal issue on FTZ architectures.
* Fix `lgamma_r` sign calculation by using the plot of `gamma` instead of using
  `sinpi` as an intermediate expression on FTZ architectures.
* Fix vector `lgamma` by swapping the horner polynomial condition on FTZ
  architectures.
* Allow `OpImageQuerySizeLod` to account for different integer types.
* Fix `SPIR-V` mangling for `get_work_dim`.
* Fix return types of builtin calls made by relational and logical `SPIR-V`
  instructions.
* Fix `x86` device name by specifying `x86` on a 64-bit machine compiling for
  32-bit.
* Fix a Windows issue where the ELF loader would fail to find `chkstk`.
* Fix half `tgamma` FTW issue due to the lack of denormal support.

## Version 1.35.0 -- 2019-06-11

Feature additions:

* The fp16 versions of `atan2pi`, `cos`, `erf`, `erfc`, `sin`, `sincos`, `tan`,
  and `tgamma`, are now implemented in purely fp16 arithmetic.  This means that
  all fp16 builtins are now implemented in purely fp16 arithmetic.
* The `vstore_half`, and `vstorea_half` functions will now use native fp16
  conversion functions on architectures that support fp16, while continuing to
  use the software emulation for architectures that do not.
* Add custom buffer descriptors to Core to allow customer projects to create
  their own custom buffer types via extensions.
* Add and move extension hooks within CL so that customer projects can extend
  `clSetKernelArg` and `clGetKernelArgInfo`.
* It is now possible to build the `host` Core target without LLVM, meaning that
  if it is possible to build a reference ComputeAorta OpenCL `EMBEDDED_PROFILE`
  implementation without LLVM.
* The JSON tracer based code now use an `mmap`'ed file instead of appending to
  a file on Linux builds.  This significantly reduces the overhead of recording
  traces in intensive sections.
* Add a `-cl-wfv=auto` VECZ mode that does an extremely small amount of
  analysis to skip vectorization on some kernels that vectorize particularly
  poorly.  This option may be used to control a more complex analysis in the
  future.
* VECZ no longer considers constant `phi` nodes as varying if they have an
  incoming edge from a divergent block.
* Within the `host` Core implementation we no longer do compiler level locking
  when processing builtin or pre-compiled kernels.
* `BenchCL` is now included in the `install` target.

Non-functional changes:

* Remove the `stub` Core implementation because it is no longer required to
  support testing as `host` supports a full LLVM-less configuration.
* Re-add `ArgumentList` accessors to UnitCL `Execution` fixture.
* Various trivial cleanups and tweaks found  by running `clang-tidy-8`.
* Eliminate use of `StructurizeCFGPass` from VECZ, this required a redesign of
  the linearisation approach but avoids corner cases where increased
  control-flow graph sizes greatly increased compilation time.
* Include vectorization statistics in the `veczc` tool.
* Simplify the code implementing support for image reads and writes,
  conversions, prefetch, and extended instructions in SPIR-V.
* Clean up use of `add_ca_executable` and `add_ca_library`, and add a
  `target_ca_sources` macro that wraps `target_ca_sources` to support the
  additional tidy features on the additional source files.
* Use `clang-tidy` with Abacus and the image library if it available.
* Remove some dead code in UnitCL and UnitVK GLSL tests.

Bug-fixes:

* Fix parsing of vectors passed by value as a parameter of builtin kernels.
* Fix passing `__local` pointer arguments to a function from a kernel, although
  a lot of code exhibits this pattern it was very hard to hit the bug in
  reality due to extensive inlining.
* Within the barrier pass demote more LLVM IR virtual registers to memory
  values to resolve some cases where incorrect values were being restored.
* Include basic blocks created during linearisation with a single predecessor
  in the BOSCC analysis to resolve miscompiles resulting in some complex
  kernels not terminating.
* VECZ will now correctly scalarize instructions whose value is used by both
  scalar and vector instructions.
* Don't attempt to create scalar masked gather operations in VECZ, as a scalar
  gather is just a load.
* VECZ now correctly replaces uses of loop live values in the pure exit of
  divergent loops.
* Move the `UnifyFunctionExitNodesPass` LLVM pass after the
  `CFGSimplificationPass` to ensure VECZ works with kernels that have one and
  only one exit block.
* Fix JSON tracing of OpenCL entry points when using the ICD loader.
* Correct minor issues when building with GCC 8.

## Version 1.34.0 -- 2019-05-07

Feature additions:

* The fp16 versions of `sinh`, `tanpi`, `pow`, `powr`, and `pown` are now
  implemented in purely fp16 arithmetic.
* Support SPIR-V `OpQuantizeToF16` and `OpSpecConstantOp FMod` instructions
  that are required for Vulkan.
* The `CA_PARALLEL_LINK_JOBS` CMake option has been added when using `ninja` to
  build ComputeAorta.  This limits the number of parallel links, which is
  particularly helpful for debug builds where parallel linking may exhaust
  available memory.
* BenchCL now has an option to select a specific device to benchmark when a
  platform contains multiple devices.
* CMake support for customer teams to be able to add their own benchmarks to
  BenchCL has been added.

Non-functional changes:

* Rework the `cl_codeplay_program_snapshot` OpenCL extension and update the
  Core API to support these changes.
* Within CL code the SPIR compile path is now only crossed when compiling a
  SPIR module.
* spirv-ll now uses a provided LLVM context rather than creating a new one per
  translation.
* Most SPIR-V parsing state state has moved from `spirv_ll::Context` to
  `spirv_ll::Module`.
* spirv-ll can now return errors when parsing an invalid SPIR-V module.
* Various changes to how coverage is supported in CMake and scripts have been
  made, enabling an improved continuous integration workflow.
* VECZ now uses LLVM diagnostic handlers to report messages, this also allows
  the OpenCL implementation to report late compiler messages through
  `cl_context` callbacks if one has been registered.
* The new builtin kernel parser added in 1.33.1, to allow builtin kernels to be
  used in compiler-less builds, has been significantly refactored.
* Most host-specific OpenCL tests have now been moved into the `host` module.
* De-duplicated internal `ASSERT` and `ABORT` macro headers.
* `cargo::string_view` now an improved pretty-printer for use in GDB.
* clang-tidy now always considers asserts during analysis, even within a
  release build.
* Use clang attributes to teach the Clang static analyzer when Cargo containers
  get reinitialized after a move.
* The `tidy` target is no longer part of `check-ComputeAorta` so as to allow
  more flexibility when running in continuous integration.  There is a new
  `check-tidy-ComputeAorta` target if the old behaviour is required.
* Various fixes to Doxygen to improve the quality of the generated output.
* Start adding scripts to enable quick testing of OpenCL ecosystem projects.

Bug-fixes:

* Change our testing of OpenCL `select` NaN behaviour to match that expected by
  the OpenCL CTS.
* Multiple fixes were made to VECZ BOSCC to fix issues discovered via the
  SYCL-DNN test suite.
* VECZ now correctly vectorizes loop-live values when loop-closed SSA phi-nodes
  have multiple incoming values, i.e. due to a loop having multiple exits.
* The `host` barrier implementation now tries to put `alloca`s immediately
  after the barrier when converting registers to memory accesses.  This
  shortens some live-ranges and thus means that fewer values need to be saved
  between work-items across the barrier.  This is particularly useful for
  architectures that only have a small amount of stack memory available.
* Fix multiple theoretical issues related to possible null-pointer
  dereferences, use of dangling pointers, or using objects after they have been
  moved.  All were found using `clang-tidy-8`.  The standard `clang-tidy`
  version for testing, however, is still version 6.
* Untangle the AutoDMA and VECZ pass managers such that AutoDMA can be used
  without VECZ.

## Version 1.33.3 -- 2019-04-15

Non-functional changes:

* Update Core specification to include built-in kernel details.
* Bump CTS to include a fix for building on MinGW.
* Assert `CA_ENABLE_DEBUG_SUPPORT` CMake option is set when
  `CA_ENABLE_DEBUG_BACKTRACE` is.
* Add testing for `void*` built-in kernel parameter types.

Bug-fixes:

* Fix `CA_HOST_ENABLE_BUILTIN_KERNEL` such that its initialization depends on
  `CA_ENABLE_DEBUG_SUPPORT`.
* Fix a data-race in a SPIR-V UnitCL test.

## Version 1.33.2 -- 2019-04-09

Bug-fixes:

* Fix builtin kernel declaration parser to support `*name` pointers style.
* Fix fp16 `hypot` by checking for NaN.
* Fix `combine_fpext_fptrunc` pass by allowing fp16 to be combined.

## Version 1.33.1 -- 2019-04-08

Feature additions:

* Addition of support for LLVM versions 8.0.1 & 9.0.
* Builtin kernels now work in a compiler-less configuration.

Non-functional changes:

* Addition of the `CA_ENABLE_DEBUG_BACKTRACE` CMake option that is now
  separated from `CA_ENABLE_DEBUG_SUPPORT` which used to cause symbol stripping
  errors on local debug builds. `CA_ENABLE_DEBUG_SUPPORT` now defines
  `CA_DEBUG_SUPPORT_ENABLED`. We now error out when trying to include headers
  of disabled features.
* Remove no longer needed `CA_BUILTINS_HALF_SUPPORT` CMake option.
* Bump core spec & update documentation on built-in kernel declarations.
* Add support in `build.py` for cross-compiling with `cl_khr_fp16` enabled.
* Addition of builtin kernels meant to test kernel arguments. This is used to
  test various arguments combinations for the offline compiler once the clang
  frontend dependency will be removed.

Bug-fixes:

* Allow for FTZ in `atan2` and `atan2pi` undefined behavior at inputs
  `(0.0, 0.0)`.
* Fix `pow` and `pown` for devices that do not support denormal half values.
* Fix UnitCL test `clGetDeviceInfoTest` by testing
  `cles_khr_2d_image_array_writes` only if images are supported.
* Query device address width for correct `size_t` width argument passing in
  SPIR-V Asymc Copy execution test.
* Fix a conditional jump based on an uninitialized SPIR-V `SamplerID` value.
* Fix `spirvll::iterator` to compile with `clang-8`.

## Version 1.33.0 -- 2019-04-02

Feature additions:

* Significant rework of the VECZ linearizer to eliminate the need to use
  StructurizeCFGPass, generally resulting in cleaner and smaller control-flow
  after vectorization.
* We now generally translate internal error codes into more appropriate OpenCL
  error codes, previously `CL_OUT_OF_RESOURCES` was over-used.
* VECZ will now eliminate common GEP instructions prior to scalarization.

Non-functional changes:

* The `CA_EXTERNAL_CLC_DIR` CMake option has been replaced by the
  `CA_EXTERNAL_CLC` option, that points directly to a `clc` binary instead of
  the directory that it contains.  This simplifies specifying an external `clc`
  on Windows builds.
* Significant refactor of the OpenCL specific compiler code to provide a
  cleaner separation between the standard compiler code and offline compile
  variants.
* Cross-compile ARM CMake now searches for any supported GCC version, rather
  than being hard-coded to 4.8.
* Significantly reduce the amount of lit test regeneration that occurs when a
  build directory is reconfigured.
* VECZ will no longer consider verification failure to require an abort in
  release builds, it will warn instead. Debug builds still abort. Either way,
  verifications failures should not happen, and there are no known cases that
  cause such a failure.
* Wire up the `cl_context` callback in the OpenCL and Core APIs in preparation
  for providing late-compilation information and more specific error reporting
  from command processing.
* Update the documentation on how OpenCL maps to Core queues and semaphores.
* `spirvll::iterator` now fulfils all the requirements of a C++ input iterator.

Bug-fixes:

* Fix SPIR-V failure when a variable is decorated as a builtin, but was not
  actually used.
* Fix two SPIR-V failures due to incorrect handling of the `Float16` type.
* Fix a SPIR-V issue related to mangling of builtins that treat their operands
  as signed.
* Correct the `half` implementation of `normalize` to match the OpenCL
  specification w.r.t. infinities.
* VECZ now treats control-flow dependent loads on divergent paths as varying.
* VECZ was trying to use a masked interleaved load for certain scalar loads, it
  now just uses the direct scalar load.
* Fix a few UnitCL tests that inadvertently used denormal values for `half`
  inputs to just always use normal values, thus avoiding differences between
  FTZ and non-FTZ architectures.

## Version 1.32.2 -- 2019-03-27

Feature additions:

* Implement the consumption of SPIR-V OpImage, OpLifetimeStart, OpLifetimeStop,
  OpMemberDecorate, OpGroupMemberDecorate and various atomic exchange
  instructions. These were not required for the OpenCL CTS, but the list of
  supported OpenCL SPIR-V instructions is now complete.
* Implement the `half` variants of the `lgamma` and `lgamma_r` builtins in pure
  `fp16` arithmetic.
* City Runner's GTest profile can now list all available tests.

Non-functional changes:

* Document in the source the reasons why certain consumption for certain SPIR-V
  instructions is not implemented, i.e. because they are only required for
  OpenCL 2.0+.
* Refactor SPIR-V type handling.
* If the `stub` Core target is built it will now abort if the device info is
  queried, to make it clear that nothing is there.

Bug-fixes:

* Modify UnitCL's handling of ULPs w.r.t. overflow and infinities to match the
  OpenCL CTS.
* Fix VECZ bug that affected the use of structs from SPIR-V input.
* Correct a data-race on Clang initialisation that resulted in a crash on
  AArch64 during aggressive parallel compilation on separate `cl_context`
  objects.
* `clc` was previously failing to report the OpenCL build log for a certain type
  of compilation failure, it now prints this log.
* Fix CMake caching issue affecting customer team extension libraries.
* Set the precise version of 'breathe' required for documentation generation as
  newer versions have backwards incompatible changes.

## Version 1.32.1 -- 2019-03-15

Feature additions:

* Allow building an offline-only ComputeAorta CL without having to provide an
  external builtins library.
* Implement the consumption of all the binary SPIR-V atomic instructions
  required for OpenCL 1.2.
* Implement the consumption of SPIR-V `OpGroupAsyncCopy`, `OpGroupWaitEvents`,
  and `OpMemoryBarrier` instructions.
* Allow comments in the input spreadsheet of the GTest profile of City Runner.

Non-functional changes:

* Define the `__STDC_FORMAT_MACROS` macro for any GCC build to cover the range
  of cross-compiles that may require this, such as MinGW builds.

Bug-fixes:

* Transform SPIR TBAA metadata into modern TBAA metadata, correcting an issue
  where SPIR kernels and builtin functions had incompatible TBAA metadata such
  that invalid alias analysis was performed.
* Clamp fp16 geometric test inputs to normal numbers to match CTS behaviour.
* Correct a denormal accuracy test that did not account for FTZ architectures.
* Correct the consumption of nonreadable, nonwriteable, volatile and coherent
  SPIR-V decorations.
* Fix minor CMake issue causing incorrect SPIR-V tool warnings to be produced.
* Fix minor CMake issue causing uninformative deprecation warnings to be output
  on MinGW builds.

## Version 1.32.0 -- 2019-03-07

Feature additions:

* It is now possible to produce a ComputeAorta CL library without having LLVM
  linked in.  Naturally this library will only be able to execute pre-compiled
  or builtin kernels, and this limitation forces the implementation to
  `EMBEDDED_PROFILE`.  This is just an initial implementation, it has been
  verified to build via a new `stub` Core target, it has been verified to run
  via the `host` Core target modified to not report compiler support.  A version
  of `host` that does not require LLVM will be in a future release.
* OpenCL SPIR-V consumption is now almost complete -- the OpenCL CTS SPIR-V CTS
  now passes, but a few remaining known issues means that the extension is
  still disabled by default.
* VECZ as used in `host` can now vectorize kernels that have a local
  workgroup size that is not a power of two, as long as the size is divisible
  by a valid vector size.  E.g. a size of 12 will now get vectorized to the
  width of 4.
* Use LLVM SCEV analysis to identify and merge contiguous but interleaved
  memory operations in VECZ, replacing a pre-existing but less general
  implementation of this functionality.  E.g. the new version can combine
  interleaved accesses even if the accesses are non-sequential (as long as they
  are contiguous as a whole).
* VECZ will now produce more efficient use of scalarized builtin calls for
  certain cases where analysis was previously suboptimal.
* More LLVM "cleanup" passes are now run after vectorization to help normalize
  the IR after the large vectorization changes have been made.
* ComputeAorta VK now supports the `VK_KHR_storage_buffer_storage_class`,
  `VK_KHR_variable_pointers`, and `VK_KHR_get_physical_device_properties2`
  extensions.  I.e. the extensions required to consume a SPIR-V shader produced
  by the clspv tool.

Non-functional changes:

* Temporarily have a few fp16 builtin functions fallback to their fp32 variants
  pending bugs related to FTZ being fixed.
* Clarify what is allowed to happen in callbacks provided to `coreDispatch`.
* AArch64 cross-compile is now included in `clc` by default.
* `core::utils::vector` is marked as deprecated, the most obvious alternative
  is `core::small_vector`, but `cargo::small_vector` may also be appropriate.
* Remove a small dependency of VECZ on `modules/utils`.
* Eliminate some harmless warnings when running `printf` UnitCL tests.
* Even release builds now strip symbols as some customer driver static
  libraries contain debug symbols.

Bug-fixes:

* The offline binary header format now entirely uses precise bitwidth
  data-types, resolving a theoretical issue related to `size_t` having a
  different size between different targets in some circumstances.
* Correct an issue that was preventing  OpenCL extensions within the `host`
  target from defining a per-extension macro for OpenCL C.
* Various bug fixes related to consumption of SPIR-V "decorations" have been
  made.
* Correct the consumption of SPIR-V `OpCopyObject`, `OpCompositeConstruct`,
  `OpAtomicIIncrement` and  `OpAtomicIDecrement` instructions.
* Fix saturated conversion of infinite halfs to int/long on fp16 FTZ
  architectures.
* Correct issues in fp16 `mix`, `smoothstep` and `async_work_group_copy` tests.
* Fix a data-race in debug support code initialized by `clCreateContext`.
* Fix a bug in VECZ BOSCC mode where some loops within a kernel ended up with
  multiple preheaders after vectorization.
* Resolve a data race in VK when multiple semaphore callbacks are triggered on
  the same queue simultaneously.

## Version 1.31.1 -- 2019-02-22

Feature additions:

* Rewrite the CMake builtins library regular expression script as a python
  script. This vastly improves build times for Core targets using fp16.
* Improve vectorization in cases where small analysis bugs, such as not
  recognising duplicate incoming edges to phi nodes, caused sub-optimal
  code-generation.
* Support uniform conditions in VECZ of the form that may be produced by the
  structurize CFG pass.
* Correctly implement OpControlBarrier in OpenCL SPIR-V consumption.

Non-functional changes:

* Allow `add_bin2h_command` CMake macro to be used by customer projects.

Bug-fixes:

* Ensure that when dispatching a command group in CL code that all semaphores
  are unique, fixing a rare and non-deterministic threading issue resulting in
  an OpenCL error code being returned.
* Fix an issue with denormal FTZ fp16 to fp32 conversion function that was
  resulting in slightly incorrect values for some inputs.
* Fix fp32 `lgamma_r` in FTZ mode. Previously NaN was incorrectly being
  returned for denormal inputs in this case, now return 0.
* Fix `tgamma` handling of FTZ architectures.
* Fix saturated conversion of floats to int when the input is NaN.
* Fix fp16 `cbrt` sometimes producing incorrect results for denormal inputs on
  FTZ architectures.
* Fix OpenCL SPIR-V image name mangling.
* Fix application of SPIR-V alignment decorations.
* Correctly set `CA_RUNTIME_COMPILER_ENABLED` by default in generic CMake so
  that customer Core targets do not need to.
* Fix an issue where the early-CSE triggers a bug in VECZ due to rearranging
  operand order in a stride calculation.
* Fix a VECZ issue that mis-compiled code where the dimension of
  `get_global_id` depended on another call to `get_global_id`.

## Version 1.31.0 -- 2019-02-08

Feature additions:

* OpenCL commands are now batched into a smaller number of Core command
  groups, with command groups being dispatched only once a triggering condition
  occurs, for example a flush on the queue.  This includes an extensive
  reworking of OpenCL command queue internals, removing the concept of
  "deferred" dispatches specific to user events and replacing it with "pending"
  dispatches.
* Add support for LLVM 7.0.1, 7.1.0 and 8.0.0 (assuming no breaking changes
  before it releases).
* Many more `half` builtin functions have now been implemented, but not yet all
  of them, so the `cl_khr_fp16` extension is still not reported by default.
  This work is nearing completion, most of the changes in this version involve
  FTZ algorithm corrections.
* Add support `OpGroupDecorate` in SPIR-V consumption.
* Multiple `-L` (library search path) parameters may now be used with City
  Runner.

Non-functional changes:

* Started ground work for producing an OpenCL library without compiler support,
  a ComputeAorta OpenCL implementation will now generally respect a Core
  implementation that reports that it does not have a compiler.  In such
  configurations, entry points that require a compiler will return the correct
  error code.  An external copy of the `clc` tool can be provided to allowing
  building the offline testing for this configuration.
* SPIR-V testing in UnitCL has been expanded, all SPIR tests are also now
  present as SPIR-V tests.
* Lit tests that can only support a single LLVM version now target LLVM 8.0.
* Updated Google Test source to a more recent version, bringing support for the
  `GTEST_SKIP` macro.  This change involved no license changes.
* VECZ's internal `double` code is now only enabled if the Core device reports
  support for `double`.
* Vulkan object creation code has been refactored to use RAII, so memory leaks
  due to failed object creation should not be possible.
* Core specification clarifications have been made around the use and
  validation of allocators.

Bug-fixes:

* For the OpenCL `printf` builtin, each work item's starting point in the
  shared buffer is now guaranteed to be aligned to 4 bytes, matching the
  requirements of the atomic addition used to reserve space.
* The `clc` tool will now produce an output when compiling an input that
  contains no kernels or functions.
* CMake dependencies on Abacus builtins are now broader, hopefully resulting in
  fewer stale builtin issues.  Due to CMake limitations it is unlikely that
  this has been 100% resolved.
* The scalar `remquo` implementation now sets it's quotient to zero for all
  early exit conditions.
* Fix a memory use-after-free bug in VECZ BOSCC mode.
* Close a memory leak affecting VECZ, debug info, and LLVM 4.0.
* Correctly process SPIR-V loops that have been marked with both "Unroll" and
  "Don't Unroll" hints (we assume that they cancel out).
* Various compiler related bug-fixes have been made for issues exposed by doing
  common sub-expression elimination early in our optimization pipeline.
* Resolved some non-deterministic behaviour in VECZ code-generation caused by
  relying on the unspecified order of function argument evaluation in C++.
* DWARF debug info sections are no longer considered while relocating an
  offline compiled ELF object.
* Handle a previously missed case for strings and `printf` in an OpenCL kernel
  that is compiled with `-cl-opt-disable`.

## Version 1.30.0 -- 2019-01-16

Feature additions:

* A major new VECZ transformation has been added but is disabled by default:
  Branch on Supercondition Code. This will speculatively assume that control
  flow is uniform, with a dynamic check to switch to a divergent path for
  control flow where this is not the case. See the VECZ documentation for more
  information.
* Provide Core implementations with a mechanism to provide a header for
  additional builtin functions that they provide. See 'Target-Specific Builtins
  Header' in the [Core documentation](doc/modules/core.md).
* Many more `half` builtin functions have now been implemented, but not yet all
  of them, so the `cl_khr_fp16` extension is still not reported by default.
* ComputeAorta now inserts an LLVM IR `fence syncscope('singlethread')`
  instruction for each `mem_fence` and `barrier` builtin for OpenCL. This is
  technically weaker than the OpenCL spec would seem to imply, but stronger
  than the previous approach, and strong enough for any current test to pass.
  This fence only prevents the compiler from moving memory operations over the
  fence, it should not result in any extra instructions in the output.
  However, some backends still have problems with even this so a utility pass
  has been added if a Core implementation wishes to strip these IR
  instructions: `core::utils::createRemoveFencesPass`. This should be used as
  late as possible in the compilation pipeline to minimise the chance of an
  illegal memory transformation.
* For OpenCL if a kernel is executed without a specific local work group size,
  and the global work size is not evenly divisible by the preferred local size
  we now progressively half the preferred size until an evenly divisible value
  is found. Previously we jumped straight to the worst case of choosing a
  local work group size of 1.

Non-functional changes:

* Users of the barrier pass no longer need to wrap it's use in Reg2Mem and
  Mem2Reg passes, it now handles this itself directly.
* The version of the OpenCL CTS that ComputeAorta uses has been updated to the
  latest `cl12_trunk` from the Khronos GitLab, plus all additional pending
  Codeplay merge requests from the Khronos GitLab.
* ComputeAorta VK will now work with the latest versions of the Vulkan SDK.
* ComputeAorta VK will now work with the Vulkan SDK installed via the Ubuntu
  package manage.
* OpenCL builtins that are 'pure' functions now have this specified as an
  attribute, potentially giving the compiler more freedom to move calls to
  these functions around.
* Various clean-ups to documentation to fix broken links and include some parts
  in the rendered output that were previously missed.

Bug-fixes:

* Work around the Khronos SPIR generator producing invalid IR when a kernel
  calls another kernel as a function in a SPIR module (the call-site and the
  function have mismatching calling conventions).
* Fix the OpenCL `rotate` builtin when rotating by 0 bits.
* Fix an issue with the implementation of barriers when vectorizing certain
  kernels with a local work group size greater than 16.
* Correctly vectorize kernels when attempting to combine interleaved memory
  operations. These cases were being mis-compiled previously.
* Fix an issue where the vectorizer was overly conservative when considering
  which instructions within a basic block are non-uniform.
* Correctly vectorize kernels kernels where the local dimension access is maybe
  `0`, i.e. kernels with code that contain the pattern `get_local_id(i)` where
  `i` might be `0`.
* Resolve a data race related to registering debug information that only
  affected builds with `CA_DEBUG_MODULE_ENABLED` set (i.e. not release builds).
* Fix an issue in `build.py` where `CA_ENABLE_API` was not respected on Windows
  builds (did not affect direct use of CMake).
* All `vkDestroy` and `vkFree` functions in the Vulkan implementation now match
  the Vulkan specification w.r.t. `VK_NULL_HANDLE`.
* Close a long-standing but hard-to-trigger memory leak in the CPU barrier
  compiler pass.

## Version 1.29.0 -- 2018-12-20

Feature additions:

* Add the ability for a customer Core target to export their own OpenCL
  extensions.
* If Tracer support is compiled into a build then the output of trace files can
  now be controlled at runtime via the `CA_TRACE_FILE` environment variable.
* Many more `half` builtin functions have now been implemented, but not yet all
  of them, so the `cl_khr_fp16` extension is still not reported by default.
* Add different 'categories' of traces to the internal profiling code.
* When vectorizing we now recognize more cases where multiple interleaved
  memory operations can be combined into a single contiguous operation.
* When vectorizing we no longer instantiate intrinsics that have a vector
  equivalent and no side-effects.
* Add support for the `RelWithDebInfo` CMake build type.

Non-functional changes:

* Support stronger const-ness of strings passed through the OpenCL API than the
  API requires itself.
* Clean-up internal OpenCL queue code, in preparation of a major rework of
  OpenCL queueing in a future version.
* Test more 'special' input numbers directly in the fp16 tests.
* Refactor some OpenCL compiler code to make its memory handling safer.
* VECZ is now included in the `tidy` target, requiring some non-functional
  changes to help the static analyzer.

Bug-fixes:

* When duplicating functions for the purposes of implementing barriers we now
  preserve more metadata.
* Correct a compiler crash when using `clc` to offline programs containing
  multiple kernels with different function signatures.
* Correct multiple issues arising when setting kernel arguments for OpenCL
  builtin kernels, previously this would often result in `clSetKernelArgs`
  returning an error code on valid uses.
* Correctly handle Core devices that report support for multiple builtin
  kernels, previously always the first kernel was used.
* Correct a vectorization crash when constant vectors were passed to some
  OpenCL image builtin functions.
* Correct some UnitCore tests that were doing invalid memory allocations.
* Fix a use-after-free memory issue in the barrier pass.
* Close memory leaks in VK.

## Version 1.28.1 -- 2018-11-19

Feature additions:

* Initial support for compiling ComputeAorta with MinGW-w64 5.3 is provided.
  This should be a NFC for all other compilers.  MinGW is not a fully supported
  platform, but the only currently known issues are that (a) using `printf` in
  an OpenCL kernels with a `%a` format specifier will result in output that
  differs from the OpenCL specification, and (b) BenchCL can only be compiled
  with MinGW natively on Windows, not in a cross-compile configuration.

Bug fixes:

* A memory leak when using VECZ has been closed.
* A CMake issue resulting in incorrect C++ defines when multiple Core targets
  are included in a single build has been fixed.
* Two issues that prevented the `cross` target added in 1.28.0 from building
  have been fixed.
* An unhandled case in VECZ that was causing poor quality IR in some uses of
  XOR has been completed.

## Version 1.28.0 -- 2018-11-07

Feature additions:

* It is now possible to cross-compile offline `host` kernels, e.g. offline
  compile a kernel for AArch64 CPU from an x86 machine.
* Provide usability improvements to the `clc` tool, it can now list available
  compilation targets and will print the build log on a compile failure.
* The `fma` half builtin function has now been implemented.
* The `vload`/`vstore`/`shuffle`/`prefetch`/`printf` OpenCL-specific SPIR-V
  instructions have been added to spirv-ll.
* Provide CMake support for customer targets to extend UnitCL and UnitVK with
  their own tests.
* Add the ability to choose the device to query in `oclc`.

Non-functional changes:

* Various clarifications have been made to the Core specification, these are
  not intended to require implementation changes.
* Extend internal compiler-pass debug functionality to passes added by
  `llvm::PassManagerBuilder`, in addition to manually added passes.
* `UnitCore` now pretty-prints Core error codes.
* Add GDB pretty-printing support for `cargo` containers.
* Bump the included google-test library to version 1.8, no license changes.

Bug-fixes:

* Fix issue related to duplicating debug info within barrier functionality,
  this fixes possible crashes when the `-g` and `-cl-opt-disable` OpenCL build
  flags were combined.
* Fix issue where VECZ would break SSA form for `GEP` instructions in certain
  dominance relationships.
* Fix issue where VECZ would assert when packetizing function calls with an
  `alloca` as a parameter.
* Resolve a crash when building SPIR programs in a manner that is not strictly
  conformant, but that worked for the non-SPIR path.
* Quote arguments containing spaces for the City Runner tool so that the SSH
  profile works correctly with these arguments.

## Version 1.27.2 -- 2018-10-24

Bug-fixes:

* Fix `ldexp` for denormal half numbers on FTZ platforms.
* Fix how VECZ handles the type of vectorizing the `relational` builtin on half
  types.
* Correct UnitCL testing of builtin functions like `rsqrt` on platforms that
  set `CA_PLATFORM_INTRINSIC_SQRT`.
* Correct the UnitCL tests for `PREFERRED_VECTOR_WIDTH_DOUBLE` to actually test
  `PREFERRED_VECTOR_WIDTH_DOUBLE` (rather than
  `CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE`).

## Version 1.27.1 -- 2018-10-12

Feature additions:

* Support a more recent LLVM 8.0 trunk commit (beyond the merge point of the
  D52294 patch mentioned in the 1.27.0 release notes).

Bug-fixes:

* Fix the use of the City Runner SSH profile from a Windows host.
* Add a missing CMake dependency of Core on Cargo, this would only affect a
  build if none of the included Core targets happened to depend on Cargo.
* Setup compiler flags to handle use of C++ extensions in OpenCL CTS.
* Silence compiler warnings for google-benchmark (i.e. external code).

## Version 1.27.0 -- 2018-09-27

Feature additions:

* Support the recently released LLVM 7.0.0, and 8.0.0 trunk.  These both
  require two pending upstream patches to be applied.  The first patch is only
  required if it is intended to use a different compiler to build LLVM than
  ComputeAorta (e.g. LLVM and GCC), the second is required in all
  circumstances.
  * https://reviews.llvm.org/D50710
  * https://reviews.llvm.org/D52294
* Many more `half` builtin functions have now been implemented, but not yet all
  of them, so the `cl_khr_fp16` extension is still not reported.
* Many more OpenCL-specific SPIR-V features have been added to spirv-ll, but
  not yet all of them. `cl_khr_il_program` extension is still not
  reported.
* However, the `cl_khr_il_program` extension is now implemented (but is not
  built by default) and initial support for SPIR-V testing has been added to
  UnitCL.
* The Core and OpenCL APIs now have basic support for OpenCL builtin kernels,
  `clGetKernelArgInfo` is not yet supported for builtin kernels.
* Installed executables now set a relative rpath, meaning that they should
  generally work without having to manually set `LD_LIBRARY_PATH`.
* Add a `--gtest_argument` / `-g` argument to City Runner GTest profile.

Non-functional changes:

* The Khronos SPIR-V headers are now just included in the ComputeAorta code to
  ease building of `cl_khr_il_program`, see license notes below.
* Update the Vulkan headers to the latest versions, see license notes below.
* Update google-benchmark to the latest version, no license changes.
* Add additional test to BenchCL to measure kernel enqueue overhead.
* Refactor Abacus type traits.

Bug-fixes:

* Replace the `core_device_t` in `core_finalizer_s` with a
  `core_device_info_t`, there is no longer a `core_device_t` available when
  creating a new finalizer.
* Ensure VK loads the correct builtins library with fp16 support when running
  on a Core device with fp16 support.  Vulkan does not support fp16, but
  ComputeAorta builds builtin library configurations based on what the Core
  device supports.
* Translate `core_error_failure` to a Vulkan error code in VK.
* Fix compilation with GCC 7.3.

Third-party license changes:

* The SPIR-V Khronos headers have been added, these are under the same Khronos
  MIT variant license that the OpenCL headers are under.  See the
  [License](../modules/spirv-ll/external/SPIRV-Headers/LICENSE).
* When the Vulkan headers got updated a license file was pulled in, the
  headers are under the Apache 2.0 license.  This is not actually a license
  change, just previously the license was only noted in the header files.  Now
  there is a LICENSE.txt and a note has been added to the root ComputeAorta
  LICENSE.txt.  See the [License](../source/vk/external/Khronos/LICENSE.txt).

## Version 1.26.2 -- 2018-09-25

Bug-fixes:

* Fix a build issue in `coreCoreSelect.h` with multiple Core targets enabled.
* Fix an issue with initializing multiple Core targets in `coreCreateDevices`.
* Resolve an issue where OpenCL CTS tests were interfering with code coverage.
* Revert the default of which symbols to expose publicly to the same
  behaviour as 1.25.0, as exposing LLVM symbols in debug builds causes issues
  when linking with other libraries that also use LLVM.  The 1.26.0 or 1.26.1
  behaviour can still be achieved by setting the `-DCA_ENABLE_DEBUG_SUPPORT=ON`
  CMake option.

## Version 1.26.1 -- 2018-08-31

Non-functional changes:

* Correct discrepancy between CHANGELOG and actual CMake for options described
  in 1.26.0 CHANGELOG as `HOST_ENABLE_FP16` and `HOST_ENABLE_FP64`.  These
  CMake options are now named `CA_HOST_ENABLE_FP16` and `CA_HOST_ENABLE_FP64`.
* Make the LLVM version check in the builtins modules more explicit to support
  `clang --version` output which has been modified.

## Version 1.26.0 -- 2018-08-30

Feature additions:

* VECZ now follows a 'partial linearization' policy, rather than always fully
  linearizing.  This can provide significant improvements for vectorizing
  kernels with complex control-flow.
* Many more `half` builtin functions have now been implemented, but not yet all
  of them, so the `cl_khr_fp16` extension is still not reported.
* Many more OpenCL-specific SPIR-V features have been added to spirv-ll, but
  not yet all of them, so the `cl_khr_il_program` extension is still not
  reported.
* The use of the thread-pool on `host` has been tuned to reduce contention,
  this especially helps SYCL programs.
* The City Runner tool SSH profile can now be provided with a private ssh key
  to use.
* VK now supports simultaneous use of command buffers, though for performance
  purposes it is currently best to not make use of this feature (see the [VK
  Documentation](doc/source/vk.md).
* The `host` Core target CMake now has the configuration options
  `HOST_ENABLE_FP16` and `HOST_ENABLE_FP64` to control whether or not the
  `host` device will support `half` and `double` data types respectively.
  Default behaviour has not changed.

Non-functional changes:

* The `clc` tool now directly uses the OpenCL compiler code, that was
  refactored in the previous release. This is part of the process of allowing
  `clc` to be a standalone tool so that it can be used for tasks such as
  cross-compiling kernels.
* Information about Core targets have now been separated from their
  initialization (`core_device_t` has been split into `core_device_t` and
  `core_device_info_t`).  This further aids the creation of offline compilation
  tools.
* The `host` Core target now uses a custom ELF loader to load and link an
  offline compiled binary rather than using LLVM functionality.
* The dependency of non-compiler code in the OpenCL runtime on LLVM data-types
  has been reduced to aid a future LLVM-less offline compile only build.
* There is a new `DEBUG_BACKTRACE` macro, only present when debug support is
  enabled, to aid development by printing out a backtrace without the need for
  a debugger.
* The `VkEvent` implementation has been significantly refactored, completing
  the VK synchronisation refactor (started in the previous release).
* A set of `offline` kernels are now created a build-time to increase testing
  for offline-compiled kernel support.

Bug-fixes:

* UnitVK no longer assumes that all devices support images and doubles.
* The `-cl-wfv=never` and `-cl-dma=never` OpenCL kernel build flags now have an
  effect for Core targets that enabled this features by default (i.e. the use
  of these flags will disable the use of the feature).
* A UnitCL test that was checking the alignment of members of a struct has been
  corrected to check for the correct alignments.
* UnitCL `half` tests now only test for denormal precision on devices that
  report support for denormals.
* Various issues found by using the thread sanitizer on VK have been fixed.
* A few memory leaks in UnitCore have been closed.

## Version 1.25.0 -- 2018-08-08

Feature additions:

* Core targets can now directly report whether or not they support the `half`
  and `double` datatypes, the correct Abacus configuration will then be built.
* If the `double` datatype is used in an OpenCL kernel on a device that does
  not support doubles then a sensible build error is given, rather than an
  obscure code-generation failure.
* Many more `half` builtin functions have now been implemented, but not yet all
  of them, so the `cl_khr_fp16` extension is still not reported.
* Many more OpenCL-specific SPIR-V features have been added to spirv-ll, but
  not yet all of them, so the `cl_khr_il_program` extension is still not
  reported.
* The OpenCL `-cl-single-precision-constant` build flag is now supported.
* The `CA_LLVM_OPTIONS` environment variable can now be used with debug builds
  to inject LLVM command line flags into the compilation pipeline.

Non-functional changes:

* Extensive refactor of OpenCL compiler code to eliminate dependencies between
  run-time and compile-time concepts.  This is part of the work to allow a
  separate offline compile tool (`clc`, in the previous release) and to allow a
  compiler-less run-time (in the future).
* `VkSemaphore` and `VkFence` implementations have been significantly
  refactored.

Bug-fixes:

* spirv-ll, when consuming Vulkan SPIR-V with logical addressing, will now
  generate LLVM IR for the SPIR triple that matches the target device, rather
  than the host platform.
* Fix `0.0` and `-0.0` getting converted to `INF` and `-INF` respectively in
  `half` reference functions in UnitCL.
* UnitCL tests that previously wrote into buffers that have been mapped as
  read-only are now fixed, by mapping the buffers for writing.
* ComputeAorta can now be compiled without warnings by GCC 7.3.
* ComputeAorta is now clean for a defined set of clang-tidy checks.

## Version 1.24.0 -- 2018-07-27 (branched 2018-07-12)

Feature additions:

* Add an initial version of `clc`, an offline compile tool.  See the
  [documentation](../doc/source/cl/tools.md) for more information.
* The `host` target will now produce and consume actual native binaries when
  requesting or creating binaries (previously the binary was actually LLVM IR),
  including support for `-cl-kernel-arg-info` on these binaries.
* A large number of OpenCL-specific SPIR-V features have been added to spirv-ll
  (but it is still incomplete, so the `cl_khr_il_program` extension is not yet
  reported).
* The half data-type is now supported via `-cl-kernel-arg-info`.
* Some half builtin functions have now been implemented, but not yet all of
  them, so the `cl_khr_fp16` extension is still not reported.
* Building ComputeAorta with support for doubles is now optional, i.e., it is
  now possible to exclude doubles from Abacus for targets that do not support
  doubles.  Mostly this has the effect of reducing binary size.
* VECZ will no-longer vectorize kernels that are entirely wrapped in an `if`
  statement that only applies to a small number of work-items in a work-group.
  E.g., `id == 0` or `id < 3`.  These edge-case kernels never experienced
  performance improvements from vectorization and could have significant
  slow-downs in pathological cases.
* Enable use of specialization constants for work-group size in Vulkan.
* A `tidy` target has been added to CMake to allow easier use of clang-tidy and
  the Clang Static Analyzer.
* City Runner now supports SSH test configurations from Windows hosts.
* LLVM 6.0.1 is now supported.

Non-functional changes:

* Add ULP-based precision testing of some builtin functions to UnitCL.  This is
  to allow testing of the on-going implementations of builtins for the `half`
  data-type.
* Use the `clspv` tool to convert some of our OpenCL tests into additional
  Vulkan tests.
* Greatly extend the test of Vulkan pipeline barriers, events, and semaphores
  in UnitVK.
* Vulkan pipeline barriers and semaphores have been re-written to make better
  use of Core pipeline barriers and Core semaphores.
* Replace `vk::as` with `vk::cast` with improved type safety in Vulkan
  implementation.
* Eliminate most use of `strlen` from the code-base, and reduce use of
  `memset`, `memcpy`, and `memmove` where possible, preferring safer C++
  solutions such as using `std::string` directly.
* Some compiler-code is being refactored so that eventually the `clc` offline
  compile tool will be independent of libOpenCL.so.

Bug-fixes:

* VECZ will no longer produce potentially illegal `addrspacecast` instructions
  around masked scatter and gather intrinsics with LLVM 5+, but must still do
  so for LLVM 4.0.
* Eliminate a minor, but long-standing, memory leak related to dominator tree
  usage in CPU barrier and DMA implementation passes.
* Fix various other memory leaks that were previously latent but have been
  exposed by improvements in support for offline kernels.
* Alignment of vectors within structs was being miscalculated for some
  targets.  Always follow the OpenCL required alignment rather than the LLVM
  reported minimum alignment for vector types.
* `cargo::small_vector::resize` could read uninitialized memory.  Account for
  that when moving items in the container.
* Ensure that CMake will not find system installs of LLVM tools for building
  builtins.
* Ensure that `char` is always treated as `signed char` when compiling Abacus
  for host (this was already handled when compiling for target hardware).
* `vkResetCommandBuffer` in Vulkan now properly resets state.
* If `vkBeginCommandBuffer` is used with a command buffer where the command
  pool was created with `RESET_COMMAND_BUFFER` we now reset the command buffer
  as required.
* City Runner will now report timeouts as failures in junit.xml, rather than as
  a "timeout", to ensure that Jenkins properly represents the failure.

Removals:

* The `cl_codeplay_extra_builtins` extension that exposed some Vulkan builtins
  in OpenCL has been removed.  This mostly existed for testing.  These builtins
  can now be tested in Vulkan directly.  Also, some of the builtins did not
  quite match their Vulkan equivalents.

## Version 1.23.3 -- 2018-07-26

Non-functional changes:

* Add a `cargo::expected` library that is essentially an implementation of the
  proposed C++ `std::expected`.  This is not yet used outside of UnitCargo, but
  it will be used in future releases.

Bug-fixes:

* Fix uninitialized variable warning in `cargo::optional`.
* Fix the return type of `cargo::optional<T>::emplace`.

Third-party license changes:

* The `cargo::expected` implementation is derived from an open-source
  implementation of expected under the Creative Commons CC0 1.0 Universal
  license.  `cargo::expected` *will* be present in distributed OpenCL and
  Vulkan libraries.  See the [License](../modules/cargo/expected.LICENSE.txt).

## Version 1.23.2 -- 2018-06-25

Feature additions:

* Revive the BenchCL micro-benchmark program.  This uses the google-benchmark
  library to benchmark various problematic OpenCL corner-cases.  It currently
  serves as a regression test for targeted performance issues.

Non-functional changes:

* Add a `cargo::optional` library that is essentially an implementation of the
  C++14 `std::optional`.  This is not yet used outside of UnitCargo, but it
  will be used in future releases.

Third-party license changes:

* The `cargo::optional` implementation is derived from an open-source
  implementation of optional under the Creative Commons CC0 1.0 Universal
  license.  `cargo::optional` *will* be present in distributed OpenCL and
  Vulkan libraries.  See the [License](../modules/cargo/optional.LICENSE.txt).
* BenchCL uses the google-benchmark library, under the Apache 2.0 license.
  BenchCL and google-benchmark *will not* be present in distributed OpenCL and
  Vulkan libraries.  See the [License](../external/benchmark/LICENSE).

## Version 1.23.1 -- 2018-06-25

Non-functional changes:

* Add LLVM 5.0.2 to list of supported LLVM versions, no changes to ComputeAorta
  where required to support this LLVM version over LLVM 5.0.1.
* Clarify what `coreGetBinary` should do when given a larger than necessary
  buffer.
* Additional tests for `sampler_t` in SPIR kernels as it requires special
  handling with recent LLVM versions.

Bug fixes:

* Fix a buffer overflow when loading SPIR binaries.
* Fix various buffer overflows within `cargo::string_view`.
* Fix a bug where `clGetEventProfilingInfo` would return profiling info for
  events that haven't happened yet if asked.  Now
  `CL_PROFILING_INFO_NOT_AVAILABLE` is returned if the event status is not yet
  `CL_COMPLETE`, as required by the OpenCL specification.
* Work around a GCC 5.2 ICE when compiling Abacus, this is a GCC bug, not a
  ComputeAorta bug.  It is fixed in newer versions of GCC.  The change is
  wrapped in an `ifdef` so it only comes into effect when compiling ComputeAorta
  with GCC 5.2.
* We no longer accept the argument `-x spir64` to `clBuildProgram` etc as it is
  not in the OpenCL specification.  Bit-width is inferred from the SPIR file.

## Version 1.23.0 -- 2018-05-31

Feature additions:

* Initial support for the `half` datatype continues with the relational OpenCL
  builtin functions being implemented.
* VECZ will now eliminate duplicate GEP instructions, doing this
  pre-vectorization can reduce the total number of vector instructions.
* Unify the UnitCL kernel test framework (KTS) so that it can also be used for
  UnitVK and exploit this by adding some kernel execution tests to UnitVK, some
  of which were created by using `clspv` on existing UnitCL tests.
* Build Windows debug builds with `_ITERATOR_DEBUG_LEVEL=0`,
  `WIN32_LEAN_AND_MEAN`, and `NOMINMAX` by default improving the speed of
  Windows debug builds at the cost of potentially missing some memory issues.
  This behaviour can be disabled by setting `CA_DISABLE_DEBUG_ITERATOR=OFF` on
  the CMake command.
* Various Vulkan tests were improved, resulting in many of the bug fixes below.
* Performance tracing code (enabled by `CA_ENABLE_TRACER`) will now not measure
  time when it was disabled.  Previously time was always measured, but only
  recorded if enabled, however the measuring turned out to have a higher cost
  than anticipated.

Non-functional changes:

* Moved the Vulkan SPIR-V parsing code (spirv-ll) into its own module in
  preparation for OpenCL SPIR-V support.
* Modify KTS such that a buffer can be used as both an input and an output in a
  test, in case triggering a failure is dependent on this.
* Allow a clang-format version to be specified for header generation by a Core
  implementation.  This means that we are no longer forcing other
  implementations to match the version of clang-format that we use (this
  behaviour was introduced in 1.22).

Bug fixes:

* Technically not a bug-fix as this only affected illegal OpenCL C or SPIR, but
  we no longer delete barriers with illegal memory fence flags, instead we
  conservatively assume a full global+local memory barrier.
* Fix the implementations of the Vulkan `(un)packHalf` builtins.
* Fix which internal builtins the Vulkan `NMin` and `NMax` builtins are mapped
  to.
* Fix the Vulkan refract builtin such that all legal combinations of argument
  types are supported.
* Fix the Vulkan `FindMSB` and related builtins such that significance is
  counted in the correct direction.
* Fix the Vulkan `RoundEven` builtin such that it actually always rounds to
  even.
* `cargo::string_view::compare` now matches the behaviour of C++17
  `std::string_view::compare`.

## Version 1.22.0 -- 2018-05-15

Feature additions:

* Initial support for the `half` datatype and optionally report the
  `cl_khr_fp16` extension to enable in testing, however, none of the builtin
  functions have been implemented yet.  This can be tested on AArch64 `host`
  devices.
* OpenCL `EMBEDDED_PROFILE` devices will now report the `cles_khr_int64` or
  `cles_khr_2d_image_array_writes` extensions if applicable.
* VECZ DXIL consumption can now understand all compute-related DXIL builtins,
  except for those specifically tied to image handling.
* Add the option `-cl-dma=always` as a compile flag for devices that support
  DMA operations.  It forces the auto-DMA pass to try to insert automatic DMA
  prefetching of kernel data.  Current behaviour of the `host` core
  implementation without this flag is to never apply the optimization.  The
  analysis is still quite limited, if it does not help then
  `async_work_group_copy` can be used directly instead.  This flag can be
  checked for by checking for the `cl_codeplay_extra_build_options` extension.
* CityRunner now has a profile for running TensorFlow tests, this is useful for
  running TensorFlow tests on 'remote' devices in situations where using Bazel
  directly may be difficult.
* OCL will now make light use of `corePushBarrier`, a.k.a. pipeline barriers,
  when scheduling work to a Core device.

Removals:

* The `CODEPLAY_DMA` environment variable is no longer checked, to achieve the
  same result use `CA_EXTRA_COMPILE_OPTS='-cl-dma=always'` instead.

Non-functional changes:

* The documentation now contains a more prominent [Getting Started
  Guide](doc/getting_started.md).
* CMake to support code coverage of ComputeAorta has been refactored.
* Refactor CMake to make greater use of `find_package` for finding
  build-related tools and dependencies.

Bug fixes:

* When linking in builtin functions we now correctly handle image pointer
  types.
* VECZ was overly conservative in marking basic blocks as divergent, failing to
  mark merge points as convergent even when this can be inferred because the
  merge block is dominated by a convergent block. This never resulted in
  incorrect execution, just sub-optimal vector code.
* Improve the suppression list for thread-sanitize testing of the OpenCL CTS.

## Version 1.21.2 -- 2018-04-18

Bug fixes:

* Fix a VECZ bug that was causing column-major memory accesses to be
  miscompiled, this could affect a SYCL program accessing a buffer using
  `get_global_linear_id` as an index.
* Fix a VECZ bug where a worklist was getting added to while being iterated
  over, this didn't ever cause any actual mis-compilations, but it disrupted
  testing on debug builds.
* Some UnitCL tests were expecting Codeplay extended build options to be
  rejected if the `cl_codeplay_extra_build_options` extension was absent,
  modify these tests to merely skip testing that situation.  We don't require
  these options to fail when the extension is not reported, we only require
  that they work when it is.
* Allow thread-sanitized testing of OpenCL on the CTS by using the
  sanitizer's black list mechanism to simply not sanitize the OpenCL CTS itself
  as there are a lot of "uninteresting" failures there.

Non-functional changes:

* Add clVectorAddition to the 'install' CMake target.
* Refactor some VK code so that memory allocation no longer happens in a
  constructor.

## Version 1.21.1 -- 2018-04-18

Non-functional changes:

* Refactor VK source structure to be conceptually closer to CL structure.
* Khronos Vulkan headers are now in `source/vk/external/Khronos`.

## Version 1.21.0 -- 2018-04-04

Feature additions:

* VECZ can now consume DXIL-flavor LLVM IR in addition to the SPIR-flavour
  LLVM IR that it currently operates on. A limited number of DXIL builtins are
  currently understood w.r.t. vectorization.
* A `CA_CL_FORCE_PROFILE` CMake option has been added to forcibly override the
  automatic OpenCL profile detection code. This is primarily useful for (a)
  testing EMBEDDED profile on a FULL profile capable device, or (b) forcing
  FULL profile for an in-progress implementation that does not yet meet some
  FULL profile requirement.
* The OpenCL implementation now acts as if any flags in the
  `CA_EXTRA_COMPILE_OPTS` environment variable were passed to
  `clCompileProgram`, and equivalently for `CA_EXTRA_LINK_OPTS` and
  `clLinkProgram`, and considers both environment variables for
  `clBuildProgram`. This provides a way to experiment with build flags without
  having to recompile the OpenCL program.
* A `-cl-llvm-stats` option has been added to `clCompileProgram` to retrieve
  internal LLVM statistics, it is equivalent to the LLVM opt tool `-stats`
  flag, i.e. statistics are printed out. The presence of this flag can be
  confirmed by looking for the `cl_codeplay_extra_build_options` extension.
* The `cl_codeplay_extra_build_options` extension can also be used to check for
  the `-cl-wfv=always` flag added in 1.19.0.  If a particular device is not
  able to vectorize, i.e. it has no vector hardware, then the flag is ignored
  with a warning.

Removals:

* The `CODEPLAY_VECZ` environment variable is no longer checked, to achieve the
  same result use `CA_EXTRA_COMPILE_OPTS='-cl-wfv=always'` instead.
* The `CA_ENABLE_LLVM_STATS` environment variable is no longer checked, to
  achieve the same result use `CA_EXTRA_COMPILE_OPTS='-cl-llvm-stats'` instead.
* The `cl_codeplay_debug_info` extension that was used to check if the `-g`
  build flag is supported has been removed, the
  `cl_codeplay_extra_build_options` extension should be used instead.

Bug fixes:

* Fix issue in VECZ where it attempted to create an illegal bitcast when
  vectorizing memcpy intrinsics.
* Used the oclgrind tool on the UnitCL 'Execution' tests and fixed a couple of
  invalid tests that worked for ComputeAorta but were not legal OpenCL in
  general.
* City runner no longer considers OpenCL CTS conversion tests that run 0 tests
  to be failures, but now counts them as skipped. This affected EMBEDDED
  profile devices that do not report support for 64-bit integers.
* The OpenCL implementation now propagates some more errors reported by a Core
  implementation up through the OpenCL API rather than just using 'asserts'.
  This likely only affected incomplete Core implementations.
* The CMake has been fixed so that a build will succeed even if no
  `CMAKE_BUILD_TYPE` is set.
* The CMake install target for OpenCL now installs the headers to `include`
  rather than `include/include`.

Non-functional changes:

* Building OpenCL extensions is no longer selected at configure-time,
  changing CMake options only affects whether or not the extension is reported.
  This makes testing more reliable. It does mean that the
  `cl_codeplay_program_snapshot` extension is always present even if not
  reported. Core implementations may or may not wish to expose additional
  snapshot points for release builds.
* Internally argument parsing for `clBuildProgram` etc has been rewritten.
* Vulkan testing has been refactored, fewer tests are now generated at
  configure time, but rather the tests have been committed to the repository.
  This simplifies the Vulkan testing CMake.

## Version 1.20.1 -- 2018-03-27

Bug fixes:

* We now accept the correct `-cl-no-signed-zeros` flag, and not the incorrect
  `-cl-no-signed-zeroes` flag that we were accepting previously.
* Quietly accept SPIR input with the `-cl-fp32-correctly-rounded-divide-sqrt`
  set, even if `CL_DEVICE_SINGLE_FP_CONFIG` does not report
  `CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT`.  This is incorrect with respect to the
  OpenCL specification, the correct behaviour is to return an error code.
  However, many OpenCL implementations do support this flag and this has lead
  to SPIR modules assuming that it is accepted.  So if a device can support the
  option we handle it, if it cannot we ignore it with a note in the build log.
  Note: In the future we may directly support this flag via Abacus.
* The City Runner GTest profile now defaults to parallel non-SSH runs.
* Expose the `vecz_mangling.h` header again, external teams depend on it.
* The `OtherBuiltins` tests continued after fatal errors, causing crashes.  The
  tests now simply fail after a fatal error.

## Version 1.20.0 -- 2018-03-16

Feature additions:

* Act upon the `-cl-denorms-are-zero` performance hint flag for x86, x86\_64,
  ARM, and AArch64 targets by setting relevant configuration registers before a
  kernel starts.  Some applications actually require this flag to be respected
  for correct behaviour as well, despite it just being a hint.
* Add the option `-cl-wfv=always` as a compile flag for devices that support
  vectorization.  It forces the whole function vectorizer to do vectorization
  if it is able.  Current behaviour for the `host` core implementation without
  this flag is to never vectorize.  An extension to allow the user to query for
  the presence of the flag will be added in a future release.

Non-functional changes:

* Update to the latest OpenCL 1.2 headers from Khronos.
* Modify VECZ to be a little more separated from OpenCL by abstracting away the
  OpenCL builtins, this makes it more adaptable to non-OpenCL use cases.
* Modify VECZ to be less dependent on LLVM TargetInfo, for use cases where it
  may not be available.
* Eliminate legacy configuration and utility code.

Bug fixes:

* Respect the required alignment of structs when vectorizing, even when the
  required alignment exceeds the target vectorization width.
* Respect the `-cl-std=CL1.1` compile flag by configuring Clang appropriately
  when it is used.
* Set the OpenCL C `__ENDIAN_LITTLE__` macro based on the relevant
  `core_device_t` property.  The macro was previously hard-coded.
* Stop Clang from setting the `CL_VERSION_2_0` OpenCL macro as we are not an
  OpenCL 2.0 implementation.
* Fix an issue in upstream Khronos OpenCL CTS where a SPIR test was doing
  illegal memory accesses and causing a segmentation fault when compiled with
  LLVM 4.0.
* Fix building on macOS, though this is an experimental platform, and may never
  reach supported status.

## Version 1.19.3 -- 2018-03-13

Bug fixes:

* Fix-up calling conventions on allowed LLVM intrinsics within SPIR, no
  functional difference but reduces noise on pass verification.
* Fix CMake issue related to not enabling assertions ourselves when
  `LLVM_ENABLE_ASSERTIONS` is set, this causes issues when including LLVM
  headers.
* Fix CMake issues that broke Vulkan cross-compiles in 1.19.1.
* Fix building ComputeAorta with GCC 4.8 and older standard C++ headers
  relating to the C++ `__STD_FORMAT_MACROS`.

## Version 1.19.2 -- 2018-03-02

Feature additions:

* Bump OpenCL 1.2 headers to latest Khronos version.
* Add an OpenCLCTS CMake target and install component.

Bug fixes:

* Fix building UnitCL with GCC 4.8.
* Correctly install UnitCL kernels to `share` when the install target is used.
* Fix UnitCL test case that implicitly had `double` constants without checking
  whether doubles are supported by the device.
* Only use -m32 when compiling 32-bit x86 (not other 32-bit architectures).
* Don't hard-code the OpenCL CTS install path at configure time.
* Correct UnitCargo compile issue with older compilers.
* Correct Visual Studio solution generation for debug builds.
* Correct mangling of masked scatter/gather intrinsics with with LLVM 4.0.

## Version 1.19.1 -- 2018-02-22

Feature additions:

* Move `modules/api` and `modules/compiler` to `source/cl`, unifying how OpenCL
  and Vulkan exist within the codebase.
* Move the `googletest` source to the toplevel `external` directory.
* Various CMake cleanups in connection with these changes.

Details of how this affects CMake for integration purposes:

* Rename CMake options:
    * `CODEPLAY_OCL_IMAGE_SUPPORT` to `CA_IMAGE_SUPPORT`.
    * `VK_LLVM_INSTALL_DIR` to `CA_LLVM_INSTALL_DIR`.
    * `OCL_EXTERNAL_BUILTINS` to `CA_EXTERNAL_BUILTINS`.
    * `CODEPLAY_OCL_32_BIT_LINUX_BUILD` to `CA_BUILD_32_BITS`.
    * `CODEPLAY_OCL_LINKER` to `CA_USE_LINKER`.
    * `CODEPLAY_OCL_USE_SANITIZER` to `CA_USE_SANITIZER`.
* Rename helper functions, when adding an:
    * Executable use `add_ca_executable`.
    * Library use `add_ca_library`.
    * Subdirectory which is a component used by other projects use
      `add_ca_subdirectory`.
* When importing an:
    * Executable use `add_ca_executable_import`.
    * Library use `add_ca_library_import`.
* Use builtin CMake variables to set C/C++ required standards:
    * `CMAKE_C_STANDARD` to 99.
    * `CMAKE_CXX_STANDARD` to 11.
* Use generator expressions for setting compile options and definitions.
* Remove broken option deprecation logic.

## Version 1.19 -- 2018-02-15

Feature additions:

* The OpenCL profile (`FULL_PROFILE` or `EMBEDDED_PROFILE`) is now dynamically
  inferred from the available resources reported through the `core` API.
  Previously it was hard-coded to `FULL_PROFILE`.
* Implement symbol mapping such that when using `perf` to profile a `host`
  implementation perf is able to understand the symbols within the JIT'ed code.
  There are also some utility scripts to help analyze the results of such runs.
* Reorganize how core implementations are integrated, they can now be
  sub-directories of the `core` module and thus take advantage of the CMake
  machinery that that provides.
* Reorganize the vast majority of the various pieces of documentation around
  the codebase into a unified `doc` directory.
* Include generated Doxygen output in Sphinx built documentation.
* Various documentation cleanups and improvements.
* Various Doxygen cleanups and improvments.
* Include a sample Vulkan compute application -- a vector addition.
* Implement and use `cargo::exchange` (equivalent to C++14 `std::exchange`).
* Separate out 'kernel' and 'scheduled kernel' creation mutexes allowing for
  more work creation parallelism if the OpenCL program is aggressively
  concurrent itself.
* Switch to a version of the OpenCL CTS that diverges less from upstream CTS.
* Support LLVM 5.0.1 and 6.0.0 (not actually released yet).

Bug fixes:

* Extend the `core` interface such that the `core_finalizer_t` is provided when
  destroying various types of kernel objects, to allow a `core` implementation
  to lock compiler resources during destruction. The `host` implementation of
  `core` now does such locking.
* Rework how LLVM compiler resources are locked within the OpenCL
  implementation, eliminating data-races when a single OpenCL program is
  compiling programs from multiple user-threads. This particularly affected
  SPIR compilation, but could manifest for OpenCL C kernels as well.
* Fix issue where handling termination of user event callbacks such that there
  was a data-race between the callback and the destruction of the event.
* Fix a data-race when an OpenCL event callback calls `clReleaseEvent` on the
  `cl_event` that it was attached to.
* Fix a race where having dependent OpenCL events could result in incorrectly
  reporting the status of a completed event as `CL_RUNNING` for a brief window.
* Fix issue where const POD kernel arguments were incorrectly retaining the
  const type qualifier when using LLVM 4.0.
* Fix issue where wrapper kernels around SPIR kernels that have `sampler_t`
  arguements did not have the `spir_kernel` calling convention.
* Fix some issues in `cargo` that affected MSVC debug builds.

## Version 1.18 -- 2017-12-05

Feature additions:

* Documentation and Doxygen has been significantly unified and restructured,
  work is continuing on this.
* Add the ability to run UnitCL via City Runner. This allows for parallelizing
  UnitCL runs, useful on simulators, and handle crashing tests gracefully,
  useful for in-development targets.
* Be more generous in accepting code that uses `reqd_work_group_size` by doing
  the correct thing even if a local size is not provided to
  `clEnqueueNDRangeKernel`.
* Core devices can now report a maximum single memory allocation size, in
  addition to a total maximum memory size.
* Add more auto-generated tests based on OpenCL kernels from open-source
  benchmarks (kernels held separately), and improvements to the test generator
  to allow this.
* MSVC builds will now automatically match the CRT used to build the version of
  LLVM being linked against, rather than the programmer having to manually
  track this.

Bug fixes:

* Fix a generic issue in the `host` barrier compiler pass where internal
  compiler data-structure iterators could be invalidated, causing arbitrary
  compilation issues.
* Fix some accuracy issues with `pow(double, double)` in value ranges not
  tested by the CTS.
* OCL will now report `CL_INVALID_WORK_ITEM_SIZE` when appropriate.
* Improve how City Runner handles timeouts over SSH by using a device side
  timeout, reduce cases of programs continuing to run beyond the timeout.
* If a corrupt SPIR kernel is provided to OCL this will now be handled
  gracefully as best as possible, with an appropriate error code being
  returned.
* Only include warnings and errors (not remarks or notes) in the OpenCL log
  when linking kernels.
* Eliminate some memory corruption due to invalid tests in UnitCL.
* Eliminate some image related memory leaks in UnitCL tests.

## Version 1.17.1 - 2017-10-24

Bug fixes:

* Correct how City Runner counts 'SKIP' tests, in version 1.17 they were
  incorrectly being considered as failed. They are now excluded from the
  calculation.
* Use of double literals in UnitCL kernels have been eliminated for cases were
  float was sufficient, i.e. these tests now work on platforms that do not
  support doubles.
* Check whether a given work group size is supported before running
  `reqd_work_group_size` tests in UnitCL.
* Correct some potential illegal memory accesses in two UnitCL kernels (only
  applies to work group sizes of less than 16).
* Correct logic for handling deprecated CMake flags.

## Version 1.17 - 2017-10-03

Feature additions:

* Add Android cross-compile support to the optional Python build scripts.
* The Core API now explicitly defines kernel work-item functions, rather the
  previous approach of implicitly following the OpenCL functions.
* City Runner timeouts over ssh are now more reliable, as the device side
  process also has a timeout.

Bug fixes:

* LLVM's `mem2reg` pass is now used before we implement barriers on `host` to
  reduce the number of live-variables that must be saved across barriers. This
  may pessimise performance, but should primarily affect the kind of obscure
  corner cases that trip up the barrier pass.

Removals:

* Deprecated `OCL_LLVM_INSTALL_DIR` in favour of `CA_LLVM_INSTALL_DIR`. The
  deprecated variable `OCL_LLVM_INSTALL_DIR` will continue to work with this
  release but will be removed in a future release.

## Version 1.16 - 2017-09-05

Feature additions:

* An additional Codeplay extension has been added that brings some GLSL
  functions into OpenCL-C. See the extension
  [description](documentation/extensions/cl_codeplay_extra_builtins.txt).
* VECZ can now linearize successive loops, i.e. it can vectorize more kernels.
* VECZ will now generate scatter/gather intrinsics for any supported LLVM
  version (5.0+ did not work previously).
* Initial support for generating API trace files that Chrome can visualize has
  been added, it is not compiled in by default.
* The `mul_hi` function on types <= 32-bits now has a more optimal
  implementation on host (by doing the calculation in 64-bits, something that
  Abacus cannot do in general).
* City runner now reports timeouts separately from failures.
* OCL now respects Core devices that report a base-alignment larger than the
  OpenCL specified minimum.
* oclc can now check expected output according to a given ULP tolerance.

Bug fixes:

* More bugs connected to phi-nodes and barriers have been fixed, allowing OpenCV
  kernels to execute correctly.
* Fix barrier bug introduced in 1.14 where functions may be removed incorrectly.
* Incorrect calling convention warnings are no longer printed for inline
  assembly (it was never actually possible to trigger this directly from
  OpenCL-C).
* City Runner no longer counts 'skipped' tests as 'passed', this does not affect
  calculating 100%, but may lower the pass percentage for architectures that do
  not yet pass the conformance tests completely.

Removals:

* Building against LLVM/Clang 3.8 or 3.9 is no longer supported (building with
  Clang 3.8 or 3.9 should still work).
* Versions of CMake 3.4.2 or lower are no longer supported when building
  ComputeAorta (i.e. 3.4.3 is the minimum).
* The in-tree versions of LLVM and Clang have been removed, providing an
  external LLVM/Clang via CMake is now the only possibility.

## Version 1.15 - 2017-08-02

Feature additions:

* The Core API can now handle 2D buffer copies directly, rather than layers
  above emulating this via multiple 1D copies.
* Multiple devices within a single `cl_context` can now synchronize buffers,
  this means that it is now possible to pass the conformance tests with multiple
  devices in a single platform.
* Extend the oclc kernel execution to handle kernels with image arguments.
* Support LLVM 6.0 tip.
* City runner now has an option to reboot remote boards when testing via SSH,
  this can be useful for ensuring that a board is in a clean state after a
  crash.

Bug fixes:

* Report denormals as supported on AArch64 (`CL_FP_DENORM`) as NEON instructions
  support them there.
* Do not use NEON instructions on AArch64 for converting 64-bit integers to
  32-bit floats, as there is no direct NEON instruction and the 2-step process
  of 64-bit integer to 64-bit float to 32-bit float does not round correctly.
* Fix a corner-case where the `pow()` function had too low ULP.
* Fixed issues with `sampler_t` being used in SPIR kernels with LLVM 4.0+ (the
  LLVM type is now a pointer, but SPIR is still an `i32`).
* Use `-ffp-contract=off` when compiling the CTS reference maths functions to
  avoid GCC changing results of the reference.
* NULL `cl_mem` parameters to `clSetKernelArg` are now accepted and passed
  through to the kernel when executed.

Removals:

* MSVC 2013 is no longer supported, various workarounds for it have been
  removed.
* Support for using PPC as a `host` implementation has been removed.

## Version 1.14.1 -- 2017-07-31

Bug fixes:

* Correct an include that assumed that the 'host' device code would be present.

## Version 1.14 -- 2017-07-11

Feature additions:

* LLVM Pass Manager builder is now used instead of a custom pass pipeline, this
  is set to level 3 for normal compiles, or level 0 for `-cl-opt-disable`
  compiles.
* Work group scheduling is optimized based on the `reqd_work_group_size`
  attribute, if present.
* Use fewer pass managers when finalizing kernels, meaning that fewer analyses
  need to be rerun.
* Return to only a single thread pool, both work and work-scheduling tasks now
  use the same thread pool.
* Mark OpenCL builtin functions related to work-groups (`get_global_id` etc) as
  'pure', allowing the compiler to optimize around them more effectively.
* VECZ will attempt to analyse memory accesses to find more cases where vector
  loads/stores can be used instead of gather/scatters even within loops where
  phi nodes are involved in address calculations.
* An alpha-version of kernel code coverage is supported by the `host` Core
  implementation. See [DEVELOPERS.md](DEVELOPERS.md) for more information.
* Various host passes have been made more generic and moved into `core::utils`,
  specifically the `ReduceToFunction`, `RemoveLifetimeInstrinsic`,
  `MakeFunctionNameUnique`, `FixupCallingConvention`, `RemoveNoDuplicates` and
  `runvecz` passes.
* The oclc tool is now able to run kernels as well as compiling them, with
  various input and output options. See the [oclc README](tools/oclc/README.md)
  for more information.
* Support LLVM 4.0.1.

Bug fixes:

* Image failures on ARM involving 8-bit types have been corrected, this was due
  to the signedness of char.
* UnitCL debug info tests only run if the relevant extension
  (`cl_codeplay_debug_info`) is present.
* Fix a few LLVM pass verification issues.
* Fix various LLVM calling convention issues exposed by more aggressive
  optimization.
* Fix a compile-time crash when compiling certain kernels using barriers.

Removals:

* The `cl_codeplay_ocl_version` extension has been removed, the improved version
  string formatting eliminates the need for this.
* The UnitCL `--platform_vendor` option is marked as deprecated (but not yet
  removed), use `--unitcl_platform` instead.

## Version 1.13.1 -- 2017-05-31

Documentation additions:

* Document how to use the ICD.
* Document various 'host' passes.
* Remove documentation stubs for some dead code.

## Version 1.13 -- 2017-05-15

Feature additions:

* Add subgroup concepts to the Core interface and the 'host' implementation to
  match that provided by pre-Core OCL. I.E. provide a method for determining the
  inferred subgroup size of a kernel given a specific local workgroup size.
* Add coverage scripts and CMake target to make producing coverage data simple.
* Update the Android build to support the more recent NDK r14
* Support debug info for automatic scope variables in the local address space.
* The bundled OpenCL headers have been updated to match those provided by
  Khronos (with a few VS2013 compile fixes).
* Add support to oclc for debugging kernels that take image type parameters.
* Automatically provide an ID per Core target, and a count of all targets in a
  given configuration.

Bug fixes:

* Fix a memory leak in the 'host' finalizer related to ownership of LLVM
  modules.
* Work around a specific upstream code-gen bug, that only affects LLVM 3.9 but
  not newer versions, when converting `char3` vectors to `short3` vectors.
* Work around a related but separate upstream code-gen bug, that only affects
  LLVM 3.9 but not newer versions, when sign extending `short3` vectors to
  `int3` vectors.
* Fix an alignment issue when a struct has a padded struct as a member.
* 'host' no longer reports images as supported if they have been disabled.
* Avoid creating multiple equivalent struct.Image types, which was causing
  issues for downstream implementations.
* Fix a bug in the 'host' core implementation when multiple semaphores are
  attached to a single dispatch, only the first was ever signalled. This bug
  did not affect OCL, it was found during VK development.
* Fix debug info on pointer variables that are live across a barrier even when
  optimizations are disabled.
* Resolve issue with DMA optimizations and LLVM 3.9+ where key functionality was
  getting incorrectly removed by dead code elimination.
* Fix a bug in the DMA optimizations on conditional loops.
* Resolve an LLVM analysis ordering issue triggered by the DMA optimizations.
* Resolve a header issue with `SIZE_MAX` preventing compiling ComputeAorta on
  CentOS.
* Use `std::getenv` instead of `getenv` for thread safety, a purely hypothetical
  fix as there are no known bugs caused by this.

## Version 1.12 -- 2017-04-18

Out-of-band release to publish results of a bug-fix pass.

Feature additions:

* IR printing debug features can now be activated for particular invocations of
  the pass for which it is being enabled, allowing reduced output when debugging
  large OpenCL programs.
* IR printing debug features now flush `stdout`.

Bug fixes:

* Fix the CPU barrier implementation w.r.t. a set of issues regarding pointers
  to live data, where those pointers are live across barriers and the barrier
  pass needs to save/restore (i.e. move) the data that they pointed to and thus
  must also update the pointer value.
* The CPU barrier implementation now respects the alignment requirements of the
  members of structs in private memory when saving/restoring the struct.
* Correctly handle the ConstantVector instructions operating on local memory
  that VECZ can produce for some inputs.
* Fix CMake issue that prevented Linux `libOpenCL.so` from having it's symbols
  versioned, OpenCL API symbols are now correctly versioned.
* Build 32-bit Windows CTS with `/LARGEADDRESSAWARE` to allow memory intensive
  tests, such as image tests, to succeed.

## Version 1.11 -- 2017-04-07

Features additions:

* Debug information is now preserved around barrier calls.
* Debug information for barrier lines is also preserved even when barrier calls
  are removed, through insertion of stub calls that a debugger can break on.
* VECZ statistics now records a vectorization ratio, accounting for additional
  scalar instructions. This gives an approximate theoretical vectorization
  speedup.

Bug fixes:

* A data race has been removed around rebuilding a `cl_program` that still has
  attached kernels despite the execution of said kernels having finished.
* A memory retention issue (effectively a memory leak caused by holding onto
  LLVM modules) has been fixed that affected long-running programs that built
  many programs.
* The OpenCL C in SPIR CTS profiling tests has been corrected to be legal.
* The OpenCL C in various SPIR CTS tests has been corrected to only define
  legal image typedefs.
* The CTS SPIR atomic tests have been corrected to not access illegal memory
  locations.
* The correct reference function is now used for CTS image stream
  `CL_ADDRESS_REPEAT` tests.
* Correct CTS max images calculation of maximum number of pixels in 3D and 2D
  array tests.
* VECZ statistics will now correctly account for internal builtin function
  calls.

## Version 1.10 -- 2017-03-07

Features additions:

* DMA optimizations now make use of scalar evolution analysis and thus better
  support kernels containing loops.
* DMA optimization passes now do more targeted DCE, focusing only on the
  specific memory operations that may now be dead.
* All compiler passes in OCL and the host Core implementation can now be
  debugged using the existing `CA_PASS_*` environment variables.
* The `CA_NUM_THREADS` environment variable can now be used to control the
  number of threads that the host Core implementation uses in its thread pool.
* The oclc tool now supports kernels with a wider range of kernel parameters.
* Cargo has gained an `array_view` non-owning container and it is used in
  various places instead of previous pointer+size pairs.
* The Clang/LLVM 4.0 release branch is now supported (and the support for tip
  Clang/LLVM has moved onto 5.0).

Bug fixes:

* A use-after-object-destroyed data race has been fixed between queues and the
  threadpool in the host Core implementation.
* A couple of memory leaks in OCL were closed, along with a few leaks in test
  suites.
* VECZ was producing linear memory operations in a particular case where
  scatter/gather operations were actually required, correct this.
* Correct VECZ's handling of the case where an `alloca` holds a pointer to a
  value stored in another `alloca`.

## Version 1.9 -- 2017-02-07

Features additions:

* DMA optimizations now support 2D kernels in specific cases, using a new 2D DMA
  core builtin.
* Multiple DMA operations will be combined into a single operation if it can be
  determined that is safe to do so.
* DMA optimizations now run before whole function vectorization.
* Devices no longer mark their device type as DEFAULT, but it is still possible
  to request the default device. Both the previous and default behaviour are
  conformant, but this version is more programmer friendly.
* If there are multiple devices in a platform only one will now be marked as the
  default, with a priority mechanism to allow devices to set their suitability
  to be the default relative to "host".
* Host now has a larger default local workgroup size.
* The core interface now requires providing an allocator at every entry point,
  allowing for using different allocators in different contexts. This matches
  the Vulkan API more closely.
* VECZ will now produce more efficient memory calculations by using splats
  instead of scalar operations.
* `UnitCL`'s VECZ tests now use more precise device checks to determine whether
  the tests can be expected to vectorize for that target.
* The `CA_PASS_*` environment variables now allows easier per-pass control.
* `CA_PASS_SCEV_*` environment variables have been added to present information
  about the kernels being compiled using LLVM's SCEV analysis.
* Various miscellaneous improvements have been made to `cargo::error_or` and
  `cargo::small_vector`.
* The various similar but slightly different `createLoop` functions have been
  unified.

Bug fixes:

* Alignment of struct members will now follow OpenCL requirements on the ARM
  architecture due to a pass that will insert padding as required.
* A VECZ bug involving writes through function calls to `alloca`s that are not
  uniform across the workgroup has been fixed.
* A VECZ bug where packetising `alloca`s by widening was creating types
  incompatible with the memory operation has been fixed.
* Abacus precision issues on hardware without floating point denormal support
  that affected `cbrt()` and `remquo()` have been fixed.
* The value returned for the OpenCL `CL_DEVICE_MAX_COMPUTE_UNITS` device query
  will now relate to the compute units on the device rather than the host
  processor.
* The values returned for `CL_DEVICE_PREFERRED_VECTOR_WIDTH_*` are now bound by
  the OpenCL type sizes, even if the hardware supports larger vectors. This is
  quite likely for small data types.
* A minor bug where option processing code was inserting an extra space that was
  causing conformance problems is fixed.

## Version 1.8 -- 2017-01-03

Features additions:

* Design documentation for OCL has been greatly expanded.
* OCL now supports programs and kernels for contexts that contain multiple
  devices, thus allowing full support for multiple Core devices within one
  OpenCL platform.
* A first version of an automatic DMA optimization has been added, it can insert
  DMA operations for simple kernels (around vector-add level of complexity) and
  hoist that DMA operation such that it starts before the work group that will
  require the data runs.
* An FTZ reflection function has been added to Abacus/Core so that a Core device
  can report whether or not it is FTZ in a way that LLVM optimization should be
  able to easily exploit through inlining and constant propagation.
* VECZ will now generally not mask the execution of uniform blocks.
* A VECZ 'choice' has been added to allow a Core target to control how
  aggressively masked scatter/gather intrinsics should be used.
* Various `CA_PASS_*` environment variables have been added to allow stronger
  verification of our LLVM passes, and to dump IR after any pass. This provides
  similar functionality to oclc, it allows more precise control, but it cannot
  be integrated with lit testing easily. It is intended to aid day-to-day
  development work on compiler passes.
* Duplicate variations of `FixupClonedDebugInfoPass` have been replaced with a
  single version in `core::utils`.

Bug fixes:

* Fix a bug in the barrier implementation when splitting live ranges but
  dominance information was not previously used.
* Fix a bug where the global constant variables that VECZ can introduce was not
  correctly handled by `ReplaceLocalModuleScopeVariablesPass`.
* Fix a bug with debug info preservation in VECZ scalarization.
* Fix a bug when reporting the length of the `CL_PROGRAM_BUILD_OPTIONS` string.

## Version 1.7 -- 2016-12-06

Feature additions:

* Support multiple devices in a single OCL platform, e.g. a CPU and an
  accelerator, even if they have different pointer width.
* Core now has a mechanism for reporting preferred local work-group sizes, and
  OCL will both communicate that to the programmer through the relevant parts of
  the OpenCL API and use it for the default work-group size.
* Core now has a mechanism for setting preferred and local vector width, and OCL
  will communicate these to the programmer through the relevant OpenCL APIs.
* Core and OCL can now support multiple memory heaps on a device.
* Codeplay OpenCL extension headers have now been unified.
* OCL will now only attempt to vectorize to widths that VECZ was designed to
  support (i.e. natural OpenCL vector widths).
* VECZ is now less aggressive with failures, and now handles more issues by just
  failing vectorization, rather than issuing an error.
* VECZ can now scalarize `printf` function calls, increasing the range of
  kernels that it can vectorize.
* VECZ will now scalarize non-constant indices for extract element, previously
  this blocked vectorization completely.
* Groundwork has been laid for future DMA optimizations.

Bug fixes:

* Fix the use of samplers when OCL is built for LLVM 3.9+.
* Fix bug in barrier pass regarding phi nodes where only some of the inputs
  exist in the post-barrier sub-region.
* Fix bug in the barrier pass regarding storage of live-ranges for 2D and 3D
  work-group iteration spaces.
* Improve alignment with respect to `__local` variables in structs, and when
  compiling for 32-bit ARM. This improves conformance on ARM CPUs.
* Fix linkage issue with LLVM intrinsic functions.
* Fix various tests in UnitCL that we assuming stronger atomic or work-item
  scheduling properties than the OpenCL 1.2 specification describes.

## Version 1.6 -- 2016-11-02

Refactor:

* Remove `CoreBuffer` and `CoreImage`, move usage of `core_buffer_t` and
  `core_image_t` into the `api` module.
* Remove `CoreQueue`, move usage to `core_queue_t` and `core_command_group_t` to
  the `api::Enqueue<Command>` functions.
* Remove `_cl_command_queue::PushBack` interface, move usage into
  `api::Enqueue<Command>` functions.
* Remove `CoreExecutable`, move usage of `core_finalizer_t` and
  `core_executable_t`, creation of `core_kernel_t`, `core_scheduled_kernel_t`,
  and `core_specialized_kernel_t` into the `compiler::Binary` object.
* Move all extensions to the new `extensions` module, rework mechanism for
  checking if an extension is enabled using the preprocessor.
* Cleanup and move utilities in the `source` directory into the `api` module.
* Remove `CODEPLAY_OCL_TARGETS_TO_BUILD` CMake variable and surrounding
  machinery.

## Version 1.5 -- 2016-11-01

Feature additions:

* Move the CPU barrier implementation from `host` to `core::util` to allow
  other implementations to use it.
* The `host` Core implementation now accepts unaligned host pointers by copying
  into an aligned buffer, causing its behaviour to more closely match
  conventional Core implementations.
* VECZ now uses the LLVM remark infrastructure for noting failed attempts to
  vectorize.
* VECZ now more aggressively does recursive inlining, to improve its analysis.
* VECZ now returns a scalar kernel, but with reduced workitem count, for the
  rare kernels that have no leaves, i.e. every workitem does the same
  calculation.
* VECZ now gathers statistics about why a kernel did not vectorize, useful
  for aggregating data on a large set of kernels (a large code base, or a test
  suite).

Bug fixes:

* `CL_DRIVER_VERSION` queries now match the format in the OpenCL specification.
* Eliminate crash on Windows when large stacks were required.
* VECZ now instantiates and masks atomic operations properly.
* Core+VECZ+Barriers interaction issues fixed.

## Version 1.4 -- 2016-10-04

Feature additions:

* Support LLVM (what will become 4.0), in addition to 3.8 and 3.9. See
  DEVELOPERS.md for exact revisions.
* Remove the legacy ARM, MIPS, and PowerPC targets. The ARM and MIPS CPUs are
  supported through the new 'Core' interface.
* Add a new, optional, `configure.py` script that makes build configuration
  simpler. Direct use of CMake is still possible and supported.
* Provide a framework for replacing Abacus builtin functions with LLVM
  intrinsics when it is advantageous to do so on a target. Start with replacing
  `__abacus_clz*` with `llvm::Intrinsic::ctlz`.
* OCL now does builtin-aware constant folding through a variety of mathematical
  builtin functions.
* VECZ can now instantiate atomic instructions, and can thus vectorize kernels
  that use atomics.
* VECZ can now emit shuffle implementations, and can thus vectorize kernels that
  use shuffles.
* VECZ now has better support for structs, and phi nodes operating on vectors,
  allowing a wider range of kernels to be vectorized.
* VECZ can now, optionally, packetize uniform instructions. This can generate
  better code on architectures where it is highly advantageous to avoid scalar
  instructions in vector code.
* VECZ now does light IR-level scheduling when instantiated function calls,
  reducing register pressure.
* VECZ now gathers a variety of statistics regarding vectorization paths taken.
* Add a flag to the Core API to specify when to disable optimizations.

Bug fixes:

* Fix handling of options to `clLinkProgram`, accept previously rejected legal
  combinations of options.
* Correctly use SSE instructions in x86-32 targets, fixing CTS regressions.
* Correct type signatures of `mul24` and `mad24` in the builtins library.
* Core implementations can now control the maximum work item sizes that OCL will
  report per device.

## Version 1.3.1 -- 2016-09-02

Bug fixes:

* Remove `--strip-all` from OCL release mode linker flags.
* Follow Khronos registry instructions on header installation.

## Version 1.3 -- 2016-08-26

Feature additions:

* Core API has been changed to provide the OpenCL builtins library directly to
  Core implementations, allowing them to replace particular builtins with
  optimized or hardware specific ones.
* VECZ now successfully vectorizes a much wider range of kernels.
* VECZ now implements partial scalarization when required.
* VECZ now implements interleaved vector loads/stores using masked vector
  scatter/gathers rather than scalar instructions.
* VECZ now preserves debug info.
* An `cl_codeplay_ocl_debug_info` extension has been provided to allow direct
  source presentation in debuggers, even if the kernel is generated dynamically,
  by using the `-S` flag when building OpenCL C programs.
* Individual passes in VECZ are now lit tested instead of treating it as a
  single black-box.
* Our performance tracking framework, `PerfCL`, now outputs `LNT` compatible
  JSON.
* Support use of Clang/LLVM 3.9.
* UnitCL kernel tests that use floating point now accept answers correct to
  within 4 ULP, so that our tests do not require hardware more accurate than the
  standard demands.

Major bug fixes:

* OCL can now be used through an ICD on 32-bit Windows (in addition to 64-bit
  Windows or 32-/64-bit Linux).
* Attributes are set on barriers so that they are respected by the upstream LLVM
  passes that we use, for both OpenCL C and SPIR.
* Support for 3D image writes is now correctly reported, this extension was
  already implemented but not reported as such.
* Illegal instructions are no longer generated when running on Skylake and using
  LLVM 3.8 (work around an LLVM back-end bug fixed with LLVM 3.9 time frame).

## Version 1.2.2 -- 2016-07-21

Bug fixes:

* Fix logic checking for double support.
* Fix Core targets which don't link LLVM.
  * Link `LLVMCodeGen` into compiler module.
  * Link `pthreads` into `UnitCore`.
* Properly null terminate in `append_null_byte.py`.

## Version 1.2.1 -- 2016-07-12

Bug fixes:

* Use `PROJECT_SOURCE_DIR` when specified directories for `Codeplay_lit`.
* Add calling conventions to API module entry points.
* Fast math test should check for double support (rather than assuming it).
* Assign platform once its been selected by UnitCL.

## Version 1.2 -- 2016-07-05

Feature additions:

* `printf` will now work for target implementations without a native `printf`
  implementation by copying all the parameters back to host and using `printf`
  there.
* VECZ can now vectorize nested-loops in most cases.
* Simplified the handling of memory mapping by adding explicit flushing to Core:
  `coreFlushMappedMemoryToDevice` and `coreFlushMappedMemoryFromDevice`.
* Partial implementation of `-cl-fast-relaxed-math`, now all float operations
  are marked as fast in the IR and calls to many maths builtin functions will be
  replaced with either calls to equivalent but faster functions (based on
  constant propagation), or to fast versions of the original function.
* Add utility code to Core to implement a `core::utils::vector` class, like a
  simple exception-safe `std::vector`.
* The UnitCL, TNEX, and KTS test-suites have now all been merged into one
  binary: UnitCL.

Major bug fixes:

* On Windows systems most of the tear down code is now disabled to prevent
  crashes-on-exit related to trying to use the threading library after it has
  been unloaded.
