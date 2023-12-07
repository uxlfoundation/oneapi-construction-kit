Adapting the Pass Pipeline for the Target
=========================================

``RefSi`` supports calling our kernel with common parameters across multiple
`instance IDs` and `slice IDs`, whereas our standard pass pipeline expects that
all the functions that are executed have the same parameters except for a unique
scheduling struct. 

To map one expectation to the other we add a wrapper pass as a final pass which
takes the *RefSi* defined parameters `instance ID` and `slice ID` to map to to
specific ndrange workgroups.

The default compiler pipeline creates a function which will work on a single
workgroup at a time. This means that it creates work item loops across the whole
workgroup. The *API* of this created function is:

.. code:: cpp

    kernel_name(param_pack, sched_struct)

The `param_pack` is a structure containing all of the parameters packed into a
structure, aligned to each paramater size.

The scheduling struct looks like:

.. code:: cpp

    struct sched_struct {
      size_t   group_id[3];
      size_t   num_groups[3];
      size_t   global_offset[3];
      size_t   local_size[3];
      uint32_t work_dim;
    }

The ``sched_struct`` is assumed to have different values per workgroup for the
`group_id` part of the struct.

*RefSi* only supports the same fixed parameters across all kernels executed, as
well as two 64 bit integer values, ``instance_id`` and ``slice_id`` as
additional parameters as follows:

.. code: cpp

    kernel_name(instance_id, slice_id, param_pack, sched_struct)


We can use these additional values to work out the ``group_id`` values as
follows:

.. code:: cpp

  group_id[0] = instance id
  group_id[1] = slice id % num_groups[1]
  group_id[2] = slice id / num_groups[1]

We thus need to write an additional pass which takes the *RefSi* function
signature, sets the ``group_id`` parts of the ``sched_struct`` and calls the
original kernel produced through the generic pipeline.

To create our pass we need to create a new file ``refsi_wrapper_pass.h`` under
``compiler/refsi_tutorial/include/refsi_tutorial``.

This file needs to look like: 

.. code:: cpp

    #ifndef REFSI_TUTORIAL_REFSI_WRAPPER_PASS_H_INCLUDED
    #define REFSI_TUTORIAL_REFSI_WRAPPER_PASS_H_INCLUDED

    #include <llvm/IR/PassManager.h> 

    namespace refsi_tutorial { 
      class RefSiWrapperPass final 
          : public llvm::PassInfoMixin<RefSiWrapperPass> { 
       public: 
        llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &); 
      }; 
    }

    #endif 

We also need an implementation pass ``RefSiWrapperPass.cpp`` under
``compiler/refsi_tutorial/source/passes``.

This will need the following includes and settings:

.. code:: cpp

    #include <refsi_tutorial/refsi_wrapper_pass.h>
    #include <compiler/utils/metadata.h>
    #include <compiler/utils/pass_functions.h>
    #include <llvm/IR/IRBuilder.h>
    #include <compiler/utils/scheduling.h>
    using namespace llvm;

To begin with we will create an empty function except to print something. 

.. code:: cpp

    namespace refsi_tutorial {
      llvm::PreservedAnalyses RefSiWrapperPass::run(llvm::Module &M, 
                                                    llvm::ModuleAnalysisManager &) { 
        (void) M;
        bool modified = false; 
        llvm::errs() << "Inside RefSiWrapperPass::run\n"; 
        return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
      }
    }

These two files will need to be added to
``compiler/refsi_tutorial/CMakeLists.txt`` under ``set(REFSI_SOURCES``.

We then need to ensure this pass is run. We use a configuration class which
allows the addition of user passes easily, amongst other settings. In this case
we wish to add passes to the end of standard pass pipeline. 

In ``compiler/refsi_tutorial/source/module.cpp``, in ``getLateTargetPasses()``
after the ``Add final passes here`` comment, add:

.. code:: cpp

  // Add final passes here by adding directly to PM as needed
  PM.addPass(refsi_tutorial::RefSiWrapperPass());

Note you will also need to include the header file ``refsi_wrapper_pass.h`` you
just created. 

Now all we need to do is compile. At this point all we need to is build the
standalone compiler, ``clc``, using ``ninja clc``.

Now we can run ``clc`` on a simple kernel, e.g.:

.. code:: c

  __kernel void copy(__global int *in, __global int *out) {
    out[get_global_id(0)] = in[get_global_id(0)];
  }

Save this to ``/tmp/copy.cl``.
Now try ``bin/clc /tmp/copy.cl``. 

You should see: 

.. code:: console

    Inside RefSiWrapperPass::run

To enable additional debug we can also support debugging of the pass by adding
to ``compiler/refsi_tutorial/source/refsi_tutorial_pass_registry.def``: 

.. code:: cpp

  #ifndef MODULE_PASS
  #define MODULE_PASS(NAME, CREATE_PASS)
  #endif
  MODULE_PASS("refsi-wrapper", refsi_tutorial::RefSiWrapperPass())
  #undef MODULE_PASS

Note this also requires adding the header for the pass to
``compiler/refsi_tutorial/source/refsi_tutorial_pass_machinery.cpp``

Running again with the debug environment variable `CA_LLVM_OPTIONS` we can see
the IR after that pass:

.. code:: console

  CA_LLVM_OPTIONS="-print-after=refsi-wrapper" \
  ./build/bin/UnitCL --gtest_filter=Execution/Execution.Task_01_02_Add/OpenCLC

we see the `IR` dumped after that pass, including the unchanged function:

.. code:: console

  ** IR Dump After refsi_tutorial::RefSiWrapperPass on [module] ***
  ; ModuleID = 'kernel.opencl'
  source_filename = "kernel.opencl"
  target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
  target triple = "riscv64-unknown-unknown-elf"

  %0 = type { [3 x i32] }
  %1 = type { [3 x i8] }
  %MuxWorkGroupInfo = type { [3 x i64], [3 x i64], [3 x i64], [3 x i64], i32 }
  %MuxPackedArgs.add = type { ptr addrspace(1), ptr addrspace(1), ptr addrspace(1) }

  @kernel_info_global = local_unnamed_addr global %0 { [3 x i32] [i32 3, i32 1, i32 0] }, section "kernel_info", align 32
  @kernel_names_global = local_unnamed_addr global %1 { [3 x i8] c"add" }, section "kernel_names", align 32

  ; Function Attrs: inaccessiblememonly mustprogress nocallback nofree nosync nounwind willreturn
  declare void @llvm.assume(i1 noundef) #0

  ; Function Attrs: nofree nosync nounwind
  define void @add(ptr nocapture readonly %0, ptr nocapture readonly %1) local_unnamed_addr #1 !codeplay_ca_wrapper !10 !mux_scheduled_fn !11 {

This will be useful as you extend the pass.

We will need to access the scheduling struct as part of this work. The following
functions can be used to load and store from the scheduling struct. 

The ``Element`` value allows us to select one of the elements from the
scheduling struct, there are enums that can be used for this
``compiler::utils::WorkGroupInfoStructField::num_groups`` and
``compiler::utils::WorkGroupInfoStructField::group_id``.

The ``Index`` allows access to the array within that if it is an array.

.. code:: cpp

    namespace {

      /// @brief Store a value to the schedule struct
      /// @param Builder IRBuilder to use
      /// @param MuxWorkGroupStructTy Scheduling structure type
      /// @param Sched Schedule struct
      /// @param Element Top level index into the struct
      /// @param Index Index into the sub array of the element. If this is not an
      /// array element, this value will be ignored.
      /// @param Val Value to be stored
      void storeToSchedStruct(IRBuilder<> &Builder, StructType *MuxWorkGroupStructTy,
                              Value *Sched, uint32_t Element, uint32_t Index,
                              Value *Val) {
        Value *IndicesArray[3] = {Builder.getInt32(0), Builder.getInt32(Element),
                                  Builder.getInt32(Index)};

        Type *ElTy = GetElementPtrInst::getIndexedType(
            MuxWorkGroupStructTy, llvm::ArrayRef<Value *>(IndicesArray, 2));
        ArrayType *ArrayTy = dyn_cast_or_null<ArrayType>(ElTy);

        Value *SchedLookupPtr =
            Builder.CreateGEP(MuxWorkGroupStructTy, Sched,
                              ArrayRef<Value *>(IndicesArray, ArrayTy ? 3 : 2));

        Builder.CreateStore(Val, SchedLookupPtr);
      }

      /// @brief Load a value from the schedule struct
      /// @param Builder IRBuilder to use
      /// @param MuxWorkGroupStructTy Scheduling structure type
      /// @param Sched Schedule struct
      /// @param Element Top level index into the struct
      /// @param Index Index into the sub array of the element. If this is not an
      /// array element, this value will be ignored.
      /// @return The value loaded from the struct
      Value *loadFromSchedStruct(IRBuilder<> &Builder,
                                 StructType *MuxWorkGroupStructTy, Value *Sched,
                                 uint32_t Element, uint32_t Index) {
        Value *IndicesArray[3] = {Builder.getInt32(0), Builder.getInt32(Element),
                                  Builder.getInt32(Index)};
        // Check if it's an array type
        Type *ElTy = GetElementPtrInst::getIndexedType(
            MuxWorkGroupStructTy, llvm::ArrayRef<Value *>(IndicesArray, 2));
        ArrayType *ArrayTy = dyn_cast_or_null<ArrayType>(ElTy);

        Value *SchedLookupPtr =
            Builder.CreateGEP(MuxWorkGroupStructTy, Sched,
                              ArrayRef<Value *>(IndicesArray, ArrayTy ? 3 : 2));
        Type *ValTy = GetElementPtrInst::getIndexedType(
            MuxWorkGroupStructTy, ArrayRef<Value *>(IndicesArray, ArrayTy ? 3 : 2));
        Value *SchedValue = Builder.CreateLoad(ValTy, SchedLookupPtr);

        return SchedValue;
      }
    }  // namespace

We also want to be able to copy the struct so we can write to it. This function
will be useful for this and should be added to the anonymous namespace:

.. code:: cpp

  /// @brief Copy a whole element from one struct to another
  /// @param Builder IRBuilder to use
  /// @param MuxWorkGroupStructTy Scheduling structure type
  /// @param SchedIn Input scheduling struct
  /// @param SchedOut Output scheduling struct
  /// @param Element Element index within scheduling struct
  void CopyElementToNewSchedStruct(IRBuilder<> &Builder,
                                   StructType *MuxWorkGroupStructTy,
                                   Value *SchedIn, Value *SchedOut,
                                   uint32_t Element) {
    Value *IndicesArray[2] = {Builder.getInt32(0), Builder.getInt32(Element)};
    Type *ElTy =
        GetElementPtrInst::getIndexedType(MuxWorkGroupStructTy, IndicesArray);
    ArrayType *ArrayTy = dyn_cast_or_null<ArrayType>(ElTy);

    // If it's an array get the number of elements
    uint32_t Count = ArrayTy ? ArrayTy->getNumElements() : 1;
    for (uint32_t i = 0; i < Count; i++) {
      Value *SchedValue = loadFromSchedStruct(Builder, MuxWorkGroupStructTy,
                                              SchedIn, Element, i);
      storeToSchedStruct(Builder, MuxWorkGroupStructTy, SchedOut, Element, i,
                        SchedValue);
    }
  }

We now want to wrap every kernel. Firstly, replace the ``llvm::errs()`` line
above in ``run()`` with the following: ``RefSiWrapperPass::run()``:

.. code:: cpp

  SmallPtrSet<Function *, 4> NewKernels; 
  for (auto &F : M.functions()) { 
    if (compiler::utils::isKernel(F) && !NewKernels.count(&F)) {
    } 
  } 
 

The ``NewKernels`` ``SmallPtrSet`` is just to ensure we don’t process the
generated new kernel function.


We will do the rest of the code in the namespace ``refsi_tutorial``. We will also
set up some useful constants to refer to the arguments:

.. code:: cpp

  namespace refsi_tutorial {
  /// @brief The index of the scheduling struct in the list of arguments.
  const unsigned int SchedStructArgIndex = 3;
  const unsigned int InstanceArgIndex = 0;
  const unsigned int SliceArgIndex = 1;

We will now write a function to wrap the kernel. We will call it
``addKernelWrapper``:

.. code:: cpp

    llvm::Function *addKernelWrapper(llvm::Module &M, llvm::Function &F) 

To start with we wish to create a bodyless function which basically takes all
of the metadata, name etc from the original function. We do this with a utility
function, ``compiler::utils::createKernelWrapperFunction()``. This utility
function will require the original function and the parameter types for the new
function.

First of all we need to gather together the types of all the new function's
arguments. This function will take the same arguments as the original function,
but with two extra 64 bit int parameters for the ``instance id`` and the ``slice
id``. 

.. code:: cpp

    // Make types for the wrapper pass based on original parameters and 
    // additional instance/slice params. 
    // We add two int64Ty for the Instance Id and Slice Id prior to the kernel
    // arguments.

    SmallVector<Type *, 4> ArgTypes; 
    ArgTypes.push_back(Type::getInt64Ty(M.getContext()));
    ArgTypes.push_back(Type::getInt64Ty(M.getContext()));
    for (auto &Arg : F.getFunctionType()->params()) { 
      ArgTypes.push_back(Arg); 
    } 
    Function *NewFunction = compiler::utils::createKernelWrapperFunction(
        M, F, ArgTypes, ".refsi-wrapper");

    // Copy over the old parameter names and attributes
    for (unsigned i = 0, e = F.arg_size(); i != e; i++) {
      auto *NewArg = NewFunction->getArg(i + 2);
      NewArg->setName(F.getArg(i)->getName());
      NewFunction->addParamAttrs(
          i + 2, AttrBuilder(F.getContext(), F.getAttributes().getParamAttrs(i)));
    }
    NewFunction->getArg(InstanceArgIndex)->setName("instance");
    NewFunction->getArg(SliceArgIndex)->setName("slice");

    if (!NewFunction->hasFnAttribute(Attribute::NoInline)) {
      NewFunction->addFnAttr(Attribute::AlwaysInline);
    }

We want to start creating code now, so create an ``IRBuilder`` for ease of use: 

.. code:: cpp

    IRBuilder<> Builder( 
            BasicBlock::Create(NewFunction->getContext(), "", NewFunction)); 

Set up some variables to refer to the arguments:

.. code:: cpp

  Argument *SchedArg = NewFunction->getArg(SchedStructArgIndex);
  Argument *InstanceArg = NewFunction->getArg(InstanceArgIndex);
  Argument *SliceArg = NewFunction->getArg(SliceArgIndex);

We will be referring to the scheduling struct a lot, so get the type:

.. code:: cpp


    auto *MuxWorkGroupStructTy = compiler::utils::getWorkGroupInfoStructTy(M);

We want to copy the input struct so we can write to it. We need to allocate this
structure on the stack:

.. code:: cpp

    auto *SchedCopyInst = Builder.CreateAlloca(MuxWorkGroupStructTy);

We can now copy the input structure to our copied structure:

.. code:: cpp

    CopyElementToNewSchedStruct(
        Builder, MuxWorkGroupStructTy, SchedArg, SchedCopyInst,
        compiler::utils::WorkGroupInfoStructField::num_groups);
    CopyElementToNewSchedStruct(
        Builder, MuxWorkGroupStructTy, SchedArg, SchedCopyInst,
        compiler::utils::WorkGroupInfoStructField::global_offset);
    CopyElementToNewSchedStruct(
        Builder, MuxWorkGroupStructTy, SchedArg, SchedCopyInst,
        compiler::utils::WorkGroupInfoStructField::local_size);
    CopyElementToNewSchedStruct(
        Builder, MuxWorkGroupStructTy, SchedArg, SchedCopyInst,
        compiler::utils::WorkGroupInfoStructField::work_dim);

In order to work out the ``group ids``, we first need to get the number of
groups in the second dimension.

.. code:: cpp

    Value *NumGroups1 = loadFromSchedStruct(
        Builder, MuxWorkGroupStructTy, SchedArg,
        compiler::utils::WorkGroupInfoStructField::num_groups, 1);


We can now work out the values for ``group id[1]`` and ``group id[2]`` from the
``SliceArg`` and ``NumGroups1``.

.. code:: cpp

    Value *GroupId1 = Builder.CreateURem(SliceArg, NumGroups1);
    Value *GroupId2 = Builder.CreateUDiv(SliceArg, NumGroups1);

We now have all the information we need to set the ``group ids``, so store to
the copied struct:

.. code:: cpp

    storeToSchedStruct(Builder, MuxWorkGroupStructTy, SchedCopyInst,
                       compiler::utils::WorkGroupInfoStructField::group_id, 0,
                       InstanceArg);
    storeToSchedStruct(Builder, MuxWorkGroupStructTy, SchedCopyInst,
                       compiler::utils::WorkGroupInfoStructField::group_id, 1,
                       GroupId1);
    storeToSchedStruct(Builder, MuxWorkGroupStructTy, SchedCopyInst,
                       compiler::utils::WorkGroupInfoStructField::group_id, 2,
                       GroupId2);

We can now just call the original function. First of all set up the arguments.
This will be the same as the original function, but replacing the input
scheduling struct with our copy and dropping the ``instance`` and ``slice`` arguments.

.. code:: cpp

  unsigned int ArgIndex = 0;
  SmallVector<Value *, 8> Args;
  for (auto &Arg : NewFunction->args()) {
    if (ArgIndex > SliceArgIndex) {
      if (ArgIndex == SchedStructArgIndex) {
        Args.push_back(SchedCopyInst);
      } else {
        Args.push_back(&Arg);
      }
    }
    ArgIndex++;
  }

We now call the original function and add a ``ret void``. Our new function is
complete now and we can return this created function.

.. code:: cpp

    compiler::utils::createCallToWrappedFunction(
        F, Args, Builder.GetInsertBlock(), Builder.GetInsertPoint());

    Builder.CreateRetVoid(); 
    return NewFunction; 

Now all we need to do is call ``addKernelWrapper()`` from ``run()``. 

.. code:: cpp

    auto *NewFunction = addKernelWrapper(M, F); 
    modified = true; 
    NewKernels.insert(NewFunction); 

 
We now wish to build *UnitCL*, the oneAPI Construction Kit test suite.

.. code:: console

    $ ninja UnitCL

We will run a single test:

.. code:: console

    $ bin/UnitCL --gtest_filter=Execution/Execution.Task_01_02_Add/OpenCLC

This show should the following:

.. code:: console

    Note: Google Test filter = Execution/Execution.Task_01_02_Add/OpenCLC
    [==========] Running 1 test from 1 test suite.
    [----------] Global test environment set-up.
    [----------] 1 test from Execution/Execution
    [ RUN      ] Execution/Execution.Task_01_02_Add/OpenCLC
    [CMP] Starting.
    [CMP] Starting to execute command buffer at 0x47fff1a0.
    [CMP] CMP_WRITE_REG64(0x1, 0x100d6)
    [CMP] CMP_WRITE_REG64(0x2, 0x2000047fff200)
    [CMP] CMP_WRITE_REG64(0x3, 0x180000000000)
    [CMP] CMP_WRITE_REG64(0x4, 0x1280000200000)
    [CMP] CMP_RUN_KERNEL_SLICE(n=4, slice_id=0, max_harts=4)
    [CMP] CMP_FINISH
    [CMP] Finished executing command buffer.
    [       OK ] Execution/Execution.Task_01_02_Add/OpenCLC (123 ms)
    [----------] 1 test from Execution/Execution (123 ms total)

    [----------] Global test environment tear-down
    [==========] 1 test from 1 test suite ran. (127 ms total)
    [  PASSED  ] 1 test.
    [CMP] Requesting stop.
    [CMP] Stopping.

Dumping the IR of your function should show your changes:

.. code:: console

   $ CA_LLVM_OPTIONS="-print-after=refsi-wrapper" bin/UnitCL \
     --gtest_filter=Execution/Execution.Task_01_02_Add/OpenCLC

  ; Function Attrs: alwaysinline mustprogress nofree norecurse nounwind willreturn memory(read, argmem: readwrite)
  define void @add.refsi-wrapper(i64 %instance, i64 %slice, ptr %packed-args, ptr noalias nonnull align 8 deferenceable(104) %wg-info) #3 !codeplay_ca_wrapper !12 !mux_scheduled_fn !15 {
    %1 = alloca %MuxWorkGroupInfo, align 8
    %2 = getelementptr %MuxWorkGroupInfo, ptr %1, i32 0, i32 1, i32 1
    %3 = load i64, ptr %2, align 8
    %4 = getelementptr %MuxWorkGroupInfo, ptr %1, i32 0, i32 1, i32 0
    %5 = load i64, ptr %4, align 8
    %6 = getelementptr %MuxWorkGroupInfo, ptr %1, i32 0, i32 1, i32 0
    store i64 %5, ptr %6, align 8
  
    ; more load/stores like this to copy the whole struct
  
    %34 = urem i64 %slice, %3
    %35 = udiv i64 %slice, %3
    %36 = getelementptr %MuxWorkGroupInfo, ptr %1, i32 0, i32 0, i32 0
    store i64 %instance, ptr %36, align 8
    %37 = getelementptr %MuxWorkGroupInfo, ptr %1, i32 0, i32 0, i32 1
    store i64 %34, ptr %37, align 8
    %38 = getelementptr %MuxWorkGroupInfo, ptr %1, i32 0, i32 0, i32 2
    store i64 %35, ptr %38, align 8
    call void @add.mux-kernel-wrapper(ptr %packed-args, ptr noalias nonnull align 8 dereferenceable(104) %1)
    ret void
  }
