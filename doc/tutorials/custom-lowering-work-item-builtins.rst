Custom Lowering Work-Item Builtins
==================================

Work-item builtin functions are core to the correct scheduling of work-groups.
Given the variety of platforms that ComputeMux runs on, it is a common
occurrence that target hardware or execution models could provide more optimal
implementations of these builtins than the "software" versions that ComputeMux
provides by default.

In the compiler, language-level work-item functions are lowered to ComputeMux
:ref:`specifications/mux-compiler-spec:builtins`. For example, OpenCL's
``get_local_id`` is lowered to call ``__mux_get_local_id``. This provides the
compiler with the ability to map multiple higher-level languages to a small
core set of built-in functions whose semantics are well understood. On the
other end, the lowering of these `mux builtins` themselves can be customized by
targets as long as they do not break the semantics of the builtins.

ComputeMux provides a software implementation of mux work-item builtins in two
passes: :ref:`AddSchedulingParametersPass
<modules/compiler/utils:AddSchedulingParametersPass>` and
:ref:`DefineMuxBuiltinsPass <modules/compiler/utils:DefineMuxBuiltinsPass>` .
The behaviour of these passes can be customized by a target's implementation of
``BuiltinInfo``.

Using an example scenario, this tutorial will provide different examples of how
the ``__mux_get_local_id`` builtin could be mapped to hardware functionality,
using a RISC-V example scenario as motivation.

Scenario
--------

If a target's scheduling model :ref:`maps hardware thread IDs to work-item IDs
<overview/example-scenarios/mapping-algorithms-to-vector-hardware:Work-group
Scheduling>`, then ``__mux_get_local_id`` could be lowered to return a hardware
thread ID.

For a RISC-V target executing `one-dimensional` work-groups, this would be
equivalent to the returning value of the ``mhartid`` control and status
register (`CSR`).

1. Override ``BuiltinInfo::defineMuxWorkItemBuiltin`` to customize how
   ``__mux_get_local_id`` is lowered.
2. Override ``BuiltinInfo::initializeSchedulingParamForWrappedKernel`` and
   store ``mhartid`` to ``MuxWorkItemInfo``'s local ID field in the kernel
   wrapper.
3. Override ``BuiltinInfo::getSchedulingParameters`` to add an additional
   thread ID parameter, and implement ``BuiltinInfo::defineMuxWorkItemBuiltin``
   to return that in ``__mux_get_local_id``.

.. note::

   Examples are for illustration purposes only, and only show how the local ID
   in the X dimension could be mapped. Other dimensions are ignored.

Each example requires a custom implementation of ``BIMuxInfoConcept`` as it
overrides the :ref:`default software implementation <modules/compiler/utils:Scheduling
Parameters>` of mux builtins in some way. The skeleton structure for each
example is:

.. code:: cpp

  class MyMuxImpl : public utils::BIMuxInfoConcept {
    // Will be filled out by each example
  };

  // Then, when registering analysis passes or creating the target's
  // PassMachinery...
  auto Callback = [&](const llvm::Module &) {
    return utils::BuiltinInfo(std::make_unique<MyMuxImpl>(),
                              utils::createSimpleCLBuiltinInfo(Builtins));
  };

  MAM.registerPass([&] { return utils::BuiltinInfoAnalysis(Callback); });

This skeleton will provide our target-specific ``BuiltinInfo`` implementation
to the compiler, e.g., in the target's implementation of
:ref:`BaseModule::createPassMachinery
<specifications/mux-compiler-spec:BaseModule::createPassMachinery>`.

We will also be using the following LLVM IR and the
:doc:`/modules/compiler/tools/muxc` compiler testing tool to demonstrate how
the changes made in each example affects code generation.

The example contains a simple kernel function called ``my_kernel`` which calls
``__mux_get_local_id``. The attributes which declare it a kernel function have
been added by a previous pass.

.. code:: llvm

   ; ModuleID = 'thread-id.ll'

   declare i64 @__mux_get_local_id(i32)

   define void @my_kernel() #0 {
     %tid = call i64 @__mux_get_local_id(i32 0)
     ret void
   }

   attributes #0 = { "mux-kernel"="entry-point" }


.. note::

   The above example is compiling for a 64-bit target. To test compilation for
   a 32-bit, change ``i64`` to ``i32`` where appropriate.

Example #1
----------

By customizing how the compiler `defines` (provides the body of the function)
the ``__mux_get_local_id`` function, we can look up and return the ``mhartid``
register. This transformation takes place during the
``DefineMuxBuiltinsPass``, once scheduling parameters have been added.

Reading the ``mhartid`` register must be done with inline assembly.

We defer to the default lowering for all other work-item builtins. This means
that, e.g., ``__mux_get_global_id`` will call ``__mux_get_local_id`` and
benefit from this optimized lowering.

The code for this example is as follows:

.. code:: cpp

  class MyMuxImpl : public utils::BIMuxInfoConcept {
    virtual llvm::Function *defineMuxBuiltin(
        utils::BuiltinID ID, llvm::Module &M,
        llvm::ArrayRef<llvm::Type *> OverloadInfo = {}) override {
      if (ID != utils::eMuxBuiltinGetLocalId) {
        return BIMuxInfoConcept::defineMuxBuiltin(ID, M, OverloadInfo);
      }
      llvm::Function *F =
          M.getFunction(utils::BuiltinInfo::getMuxBuiltinName(ID));
      // Set some useful function attributes
      setDefaultBuiltinAttributes(*F);
      F->setLinkage(llvm::GlobalValue::InternalLinkage);
      // Set up a basic block for our new function body
      auto *BB = llvm::BasicBlock::Create(M.getContext(), "entry", F);

      // Create an inline assembly statement which reads the value of mhartid
      auto *const Asm = llvm::InlineAsm::get(
          llvm::FunctionType::get(F->getReturnType(), /*isVarArg*/ false),
          "csrr\t$0, mhartid", "=r,~{memory}", /*hasSideEffects*/ true);

      llvm::IRBuilder<> IRB(BB);
      // "Call" this inline assembly statement and return it
      IRB.CreateRet(IRB.CreateCall(Asm, {}, "thread-id"));
      return F;
    }
  };


.. code:: llvm

   ; Run this on the command line, you should see the following
   ; muxc --device "RefSi M1" --passes define-mux-builtins -S thread-id.ll

   ; Function Attrs: alwaysinline
   define internal i64 @__mux_get_local_id(i32 %0) #0 {
   entry:
     %thread-id = call i64 asm sideeffect "csrr\09$0, mhartid", "=r,~{memory}"()
     ret i64 %thread-id
   }

   define void @my_kernel() #1 {
     %tid = call i64 @__mux_get_local_id(i32 0)
     ret void
   }

   attributes #0 = { alwaysinline }
   attributes #1 = { "mux-kernel"="entry-point" }

Here, the ``DefineMuxBuiltinsPass`` (``define-mux-builtins``) has picked up the
custom lowering of ``__mux_get_local_id`` to instead return the ``mhartid``
register.

.. note::

   Since other work-item builtins retain their default lowering, they need
   scheduling parameters. As such, the ``AddSchedulingParametersPass``
   (``add-sched-params``) is still required in general. When running this pass
   first, you should see:

  ..
    This should be highlighted as 'llvm' but it can't yet parse opaque pointers

  .. code::

    ; Run this on the command line, you should see the following
    ; muxc --device "RefSi M1" --passes add-sched-params,define-mux-builtins -S thread-id.ll

    declare i64 @__mux_get_local_id.old(i32)

    define internal void @my_kernel() {
      %tid = call i64 @__mux_get_local_id.old(i32 0)
      ret void
    }

    ; Function Attrs: alwaysinline
    define internal i64 @__mux_get_local_id(i32 %0, ptr noalias %wi-info, ptr noalias %wg-info) #0 !mux_scheduled_fn !1 {
    entry:
      %thread-id = call i64 asm sideeffect "csrr\09$0, mhartid", "=r,~{memory}"()
      ret i64 %thread-id
    }

    define void @my_kernel.mux-sched-wrapper(ptr noalias %wi-info, ptr noalias %wg-info) #1 !mux_scheduled_fn !2 {
      %tid = call i64 @__mux_get_local_id(i32 0, ptr noalias %wi-info, ptr noalias %wg-info)
      ret void
    }

    attributes #0 = { alwaysinline }
    attributes #1 = { "mux-base-fn-name"="my_kernel" "mux-kernel"="entry-point" }

    !mux-scheduling-params = !{!0}

    !0 = !{!"MuxWorkItemInfo", !"MuxWorkGroupInfo"}
    !1 = !{i32 1, i32 2}
    !2 = !{i32 0, i32 1}


  Note how ``__mux_get_local_id`` has received scheduling parameters even
  though it doesn't use them. The generated LLVM-IR also contains two dead
  functions. Two key LLVM passes - `dead global elimination` and `dead argument
  elimination` - will usually clean this up later:

  ..
    This should be highlighted as 'llvm' but it can't yet parse opaque pointers

  .. code::

    ; Run this on the command line, you should see the following
    ; muxc --device "RefSi M1" --passes add-sched-params,define-mux-builtins,globaldce,deadargelim -S thread-id.ll

    ; Function Attrs: alwaysinline
    define internal void @__mux_get_local_id() #0 !mux_scheduled_fn !1 {
    entry:
      %thread-id = call i64 asm sideeffect "csrr\09$0, mhartid", "=r,~{memory}"()
      ret void
    }

    define void @my_kernel.mux-sched-wrapper(ptr noalias %wi-info, ptr noalias %wg-info) #1 !mux_scheduled_fn !2 {
      call void @__mux_get_local_id()
      ret void
    }

    attributes #0 = { alwaysinline }
    attributes #1 = { "mux-base-fn-name"="my_kernel" "mux-kernel"="entry-point" }

    !mux-scheduling-params = !{!0}

    !0 = !{!"MuxWorkItemInfo", !"MuxWorkGroupInfo"}
    !1 = !{i32 1, i32 2}
    !2 = !{i32 0, i32 1}


Example #2
----------

Alternatively, it is possible to exploit the default lowering of
``__mux_get_local_id``. The default scheduling parameter ``MuxWorkItemInfo``
has a three-dimensional field to hold local ID values. In the default
compilation pipeline, these values are set by the :ref:`HandleBarriersPass
<modules/compiler/utils:HandleBarriersPass>`. This pass maps all work-items of
a work-group to run on a single hardware thread by making the implicit
parallelism model explicit, inserting three-dimensional loops over a work-group
and calling ``__mux_set_local_id`` in every work-item loop iteration before
calling the original kernel function. If the target does not run this pass and
can guarantee that these local ID values are not otherwise clobbered, it could
store ``mhartid`` to this scheduling parameter once per kernel invocation.

The :ref:`AddKernelWrapperPass <modules/compiler/utils:AddKernelWrapperPass>` is
responsible for initializing any scheduling parameters which are not passed
"externally" by the driver. By overriding
``BuiltinInfo::initializeSchedulingParamForWrappedKernel``, we can customize
the initialization of ``MuxWorkItemInfo`` to store ``mhartid`` to the local ID.

.. code:: cpp

  class MyMuxImpl : public utils::BIMuxInfoConcept {
    virtual llvm::Value *initializeSchedulingParamForWrappedKernel(
        const utils::BuiltinInfo::SchedParamInfo &Info, llvm::IRBuilder<> &B,
        llvm::Function &IntoF, llvm::Function &) override {
      // We only expect to have to initialize the work-item info. The work-group
      // info is passed straight through.
      assert(!Info.PassedExternally && Info.ID == 0 && Info.ParamName == "wi-info");
      llvm::Module &M = *IntoF.getParent();
      // Create an inline assembly statement which reads the value of mhartid
      auto *const Asm = llvm::InlineAsm::get(
          llvm::FunctionType::get(compiler::utils::getSizeType(M), false),
          "csrr\t$0, mhartid", "=r,~{memory}", true);
      // This is known to be the underlying structure type of this scheduling
      // parameter
      auto *Ty = utils::getWorkItemInfoStructTy(M);
      // Allocate MuxWorkItemInfo on the stack
      auto *const Alloca = B.CreateAlloca(Ty, /*ArraySize*/ nullptr, Info.ParamName);
      // Calculate the address of the local ID field in the X dimension
      auto *const FieldAddr =
          B.CreateGEP(Ty, Alloca, {B.getInt32(0), B.getInt32(0), B.getInt32(0)});
      // Store mhartid to this address
      auto *const Call = B.CreateCall(Asm, {}, "thread-id");
      B.CreateStore(Call, FieldAddr, "store");
      // Return the address of the allocation to be passed to the wrapped kernel
      return Alloca;
    }
  };

Running the ``AddKernelWrapperPass`` (``add-kernel-wrapper``) as part of the
pipeline, it is possible to see that the wrapper kernel when initializing
``MuxWorkItemInfo`` also sets up the local ID by storing the value of
``mhartid`` to the scheduling structure. The default lowering of
``__mux_get_local_id`` thus picks this up.

..
  This should be highlighted as 'llvm' but it can't yet parse opaque pointers

.. code::

  ; Run this on the command line, you should see the following
  ; muxc --device "RefSi M1" --passes add-sched-params,define-mux-builtins,add-kernel-wrapper -S thread-id.ll

  %MuxWorkItemInfo = type { [3 x i64], i32, i32, i32, i32 }

  ; Function Attrs: alwaysinline
  define internal i64 @__mux_get_local_id(i32 %0, ptr noalias %wi-info, ptr noalias %wg-info) #0 !mux_scheduled_fn !1 {
    %2 = icmp ult i32 %0, 3
    %3 = select i1 %2, i32 %0, i32 0
    %4 = getelementptr %MuxWorkItemInfo, ptr %wi-info, i32 0, i32 0, i32 %3
    %5 = load i64, ptr %4, align 4
    %6 = select i1 %2, i64 %5, i64 0
    ret i64 %6
  }

  ; Function Attrs: alwaysinline
  define internal void @my_kernel.mux-sched-wrapper(ptr noalias %wi-info, ptr noalias %wg-info) #1 !mux_scheduled_fn !2 {
    %tid = call i64 @__mux_get_local_id(i32 0, ptr noalias %wi-info, ptr noalias %wg-info)
    ret void
  }

  ; Function Attrs: nounwind
  define void @my_kernel.mux-kernel-wrapper(ptr %packed-args, ptr noalias %wg-info) #2 {
    %wi-info = alloca %MuxWorkItemInfo, align 8
    %1 = getelementptr %MuxWorkItemInfo, ptr %wi-info, i32 0, i32 0, i32 0
    %thread-id = call i64 asm sideeffect "csrr\09$0, mhartid", "=r,~{memory}"()
    store volatile i64 %thread-id, ptr %1, align 4
    call void @my_kernel.mux-sched-wrapper(ptr noalias %wi-info, ptr noalias %wg-info) #1
    ret void
  }

  attributes #0 = { alwaysinline }
  attributes #1 = { alwaysinline "mux-base-fn-name"="my_kernel" }
  attributes #2 = { nounwind "mux-base-fn-name"="my_kernel" "mux-kernel"="entry-point" }

  !mux-scheduling-params = !{!0}

  !0 = !{!"MuxWorkItemInfo", !"MuxWorkGroupInfo"}
  !1 = !{i32 1, i32 2}
  !2 = !{i32 0, i32 1}

.. note::

   Note: some dead functions (explained above) have been trimmed for clarity.


Example #3
----------

Instead of using the default scheduling parameters, targets may wish to add
`additional` scheduling parameters. This approach may benefit targets with a
certain kernel ABI, or ones whose work-group scheduling calculates work-item
data beyond the view of ComputeMux, e.g., in the driver or the HAL.

.. code:: cpp

  class MyMuxImpl : public utils::BIMuxInfoConcept {
    virtual llvm::SmallVector<utils::BuiltinInfo::SchedParamInfo, 4>
    getMuxSchedulingParameters(llvm::Module &M) override {
      // Retrieve the default list of scheduling parameters (MuxWorkItemInfo
      // and MuxWorkGroupInfo)
      auto List = BIMuxInfoConcept::getMuxSchedulingParameters(M);
      // Register a third scheduling parameter - a 64-bit integer we'll use to
      // pass through the thread ID.
      utils::BuiltinInfo::SchedParamInfo Extra;
      Extra.ID = 2;
      Extra.ParamTy = llvm::Type::getInt64Ty(M.getContext());
      Extra.ParamName = "thread-id";
      Extra.ParamDebugName = "ThreadID";
      Extra.PassedExternally = true;

      List.push_back(Extra);

      return List;
    }

    virtual llvm::Function *defineMuxBuiltin(
        utils::BuiltinID ID, llvm::Module &M,
        llvm::ArrayRef<llvm::Type *> OverloadInfo = {}) override {
      if (ID == utils::eMuxBuiltinGetLocalId) {
        llvm::Function *F =
            M.getFunction(utils::BuiltinInfo::getMuxBuiltinName(ID));
        // Set some useful function attributes
        setDefaultBuiltinAttributes(*F);
        // We additionally know that our function is readnone
        F->addFnAttr(llvm::Attribute::ReadNone);
        F->setLinkage(llvm::GlobalValue::InternalLinkage);
        auto *BB = llvm::BasicBlock::Create(M.getContext(), "entry", F);
        llvm::IRBuilder<> B(BB);
        // Simply return the last scheduling parameter, which we know is the
        // thread ID.
        B.CreateRet(std::prev(F->arg_end()));
        return F;
      }
      return BIMuxInfoConcept::defineMuxBuiltin(ID, M, OverloadInfo);
    }
  };

Running the ``AddSchedulingParametersPass`` will show that the third scheduling
parameter has been added to ``__mux_get_local_id``, and that
``DefineMuxBuiltinsPass`` then kicks in to simply return that value.

..
  This should be highlighted as 'llvm' but it can't yet parse opaque pointers

.. code::

  ; Run this on the command line, you should see the following
  ; muxc --device "RefSi M1" --passes add-sched-params,define-mux-builtins -S thread-id.ll

  ; Function Attrs: alwaysinline
  define internal i64 @__mux_get_local_id(i32 %0, ptr noalias %wi-info, ptr noalias %wg-info, i64 %thread-id) #0 !mux_scheduled_fn !1 {
  entry:
    ret i64 %thread-id
  }

  define void @my_kernel.mux-sched-wrapper(ptr noalias %wi-info, ptr noalias %wg-info, i64 %thread-id) #1 !mux_scheduled_fn !2 {
    %thread-id = call i64 @__mux_get_local_id(i32 0, ptr noalias %wi-info, ptr noalias %wg-info, i64 %thread-id)
    ret void
  }

  attributes #0 = { alwaysinline readnone }
  attributes #1 = { "mux-base-fn-name"="my_kernel" "mux-kernel"="entry-point" }

  !mux-scheduling-params = !{!0}

  !0 = !{!"MuxWorkItemInfo", !"MuxWorkGroupInfo", !"ThreadID"}
  !1 = !{i32 1, i32 2, i32 3}
  !2 = !{i32 0, i32 1, i32 2}

.. note::

   Note: some dead functions (explained above) have been trimmed for clarity.

We can then see how the ``AddKernelWrapperPass`` respects this scheduling
parameter. Note how ``%thread-id`` now forms part of the kernel ABI:

..
  This should be highlighted as 'llvm' but it can't yet parse opaque pointers

.. code::

  ; Run this on the command line, you should see the following
  ; muxc --device "RefSi M1" --passes add-sched-params,define-mux-builtins,add-kernel-wrapper -S thread-id.ll

  %MuxWorkItemInfo = type { [3 x i64], i32, i32, i32, i32 }

  ; Some functions omitted for clarity

  ; Function Attrs: nounwind
  define void @my_kernel.mux-kernel-wrapper(ptr %packed-args, ptr noalias %wg-info, i64 %thread-id) #2 {
    %wi-info = alloca %MuxWorkItemInfo, align 8
    call void @my_kernel.mux-sched-wrapper(ptr noalias %wi-info, ptr noalias %wg-info, i64 %thread-id) #1
    ret void
  }

In this example, it can be imagined that the code that calls the kernel
``my_kernel`` initializes a parameter register (e.g., ``a2`` on RISC-V) with
the value of ``mhartid``.


Other Approaches
----------------

The set of examples given are not exhaustive: it is possible to combine any of
the above examples:


* Examples #2 and #3 could be combined to result in a third 64-bit integer
  ``ThreadID`` scheduling parameter whose value is initialized by the
  ``AddKernelWrapperPass``, rather than being passed to the kernel.
* Targets using the ``HandleBarriersPass`` could customize the lowering of
  ``__mux_set_local_id`` akin to example #1 to set a target-specific reserved
  register which is then read by ``__mux_get_local_id``.
