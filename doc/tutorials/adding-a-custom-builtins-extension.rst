Adding A Custom Builtins Extension
==================================

This tutorial is intended to be based on the target created in the
:ref:`create new target tutorial<overview/tutorials/creating-a-new-mux-target/running-new-target-script:Running the create_target.py Script>`.

.. note::
   If the user does not want to do the whole tutorial the ``json`` file
   ``refsi_with_wrapper.json`` can be used in the target creation. Also if the user
   wishes to see the result of this tutorial, ";clmul" can be added to the ``feature``
   ``json`` entry.


It is common that a target's hardware has specialized instructions which
accelerate operations suited to the platform and its intended domain.

Often, these instructions can be successfully matched from user code
automatically, through the compiler backend identifying and optimizing
sequences of basic operations.

Occasionally, these instructions are so specialized that compilers are unable
to match them automatically. In this situation it is common that vendors
provide *builtin functions* to users for use in their programs. These builtins
are then caught and matched to specialized instructions by compilers.

This tutorial shows an example of such a scenario using RISC-V's `carry-less
multiplication (Zbc) extension
<https://github.com/riscv/riscv-bitmanip/releases/tag/1.0.0>`_. It will expose
the *carry-less multiplication* (``clmul*``) instructions to users via builtin
functions for use in OpenCL programs.

Registering A New Compiler Extension
------------------------------------

Registering a new compiler extension in ComputeMux requires the following files:

.. code::

    compiler/refsi_tutorial/extension/riscv_clmul/
    ├─ CMakeLists.txt
    ├─ include/
    │  ├─ extension/
    │  │  ├─ riscv_clmul.h
    │  │  ├─ builtins.h
    ├─ source/
    │  ├─ riscv_clmul.cpp
    
Note that this requires the new directory `extension` to have a simple
`CMakeLists.txt` as follows:

.. code:: cmake

  add_subdirectory(riscv_clmul)

The `extension` subdirectory has to be added to
``compiler/refsi_tutorial/CMakeLists.txt`` before ``set(REFSI_TUTORIAL_SOURCES``:

.. code:: cmake

  add_subdirectory(extension)

  # add before this point
  set(REFSI_TUTORIAL_SOURCES

The main header and source files provide the OpenCL runtime with knowledge of
the extension when registered:

``riscv_clmul.h``:

.. code:: cpp

 #ifndef EXTENSION_RISCV_CLMUL_INCLUDED
 #define EXTENSION_RISCV_CLMUL_INCLUDED
 
 #include <extension/extension.h>
 
 namespace extension {
 
 class riscv_clmul final : public extension {
  public:
   riscv_clmul();
 };
 
 }  // namespace extension
 
 #endif  // EXTENSION_RISCV_CLMUL_INCLUDED

``riscv_clmul.cpp``:

.. code:: cpp

 #include <extension/riscv_clmul.h>
 
 extension::riscv_clmul::riscv_clmul()
     : extension("cl_riscv_clmul",
                 usage_category::DEVICE CA_CL_EXT_VERSION(1, 0, 0)) {}


The CMake code is used to register a compiler extension with these source files
and register a *force-include* header file which is implicitly included in all
OpenCL programs when the extension is enabled.

``CMakeLists.txt``:

.. code:: cmake

 add_ca_cl_compiler_extension(riscv-clmul
   EXTENSIONS riscv_clmul
   INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
   SOURCES
   ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/riscv_clmul.h
   ${CMAKE_CURRENT_SOURCE_DIR}/source/riscv_clmul.cpp)
 
 add_ca_force_header(PREFIX "riscv_clmul"
   DEVICE_NAME "${CA_REFSI_TUTORIAL_DEVICE}"
   PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/extension/builtins.h")

The force-include header itself provides declarations of the builtin functions
to expose to users. This tutorial covers adding three builtins which map 1:1
with their corresponding RISC-V instructions, assuming a 64-bit RISC-V platform
(i.e., mapping the 64-bit ``long`` directly to the native 64-bit register
length without type promotion). 

``builtins.h``:

.. code:: c

 // Produce the lower half of the 128-bit carry-less product
 __attribute__((overloadable)) long clmul(long, long);
 
 // Produce the upper half of the 128-bit carry-less product
 __attribute__((overloadable)) long clmulh(long, long);
 
 // Produces bits 126-63 of the 128-bit carry-less product
 __attribute__((overloadable)) long clmulr(long, long);


.. note::

  Builtins must be given the `overloadable attribute
  <https://releases.llvm.org/16.0.0/tools/clang/docs/AttributeReference.html#overloadable>`_
  as OpenCL functions are internally required to be mangled according to their
  type signatures.

With the extension registered, it is now possible for users to use these
builtins in programs via the implicitly-included header:

.. code:: c

 kernel void do_clmul(global long *a, global long *b, global long *z) {
   size_t id = get_global_id(0);
   z[id] = clmul(a[id], b[id]);
 }

However, the builtins have been provided only as *declarations* so the compiler
is required to provide *definitions* for the builtins. Without this step, the
program fails to build as it contains unresolved symbols. The next step is to
modify the compiler to translate the builtin functions to *LLVM intrinsic
functions* which finish the implementation.

.. note::

  Builtins can be given default software implementations, with the compiler
  optimizing to more optimal forms given certain conditions, but that is not
  within the scope of the tutorial.

Builtin Replacement
-------------------

With the ``clmul*`` builtins registered via the extension and successfully
entering the LLVM IR module via the compiler frontend, the next step is to
ensure that the builtins are lowered to the appropriate instructions.

This tutorial lowers builtins to RISC-V instructions via *LLVM intrinsics* as
the open-source LLVM 13+ compiler provides intrinsics which map 1:1 with
``clmul*`` instructions. The task, therefore, is to replace calls to ``clmul*``
builtin functions in LLVM IR with calls to ``llvm.riscv.clmul*`` intrinsic
functions.

ComputeMux makes this task straightforward; targets can hook in custom logic to
replace builtin functions with more optimal sequences of LLVM IR.

The :ref:`OptimalBuiltinReplacementPass
<modules/compiler/utils:OptimalBuiltinReplacementPass>` is a utility pass that
runs over a module, identifying call instructions and replacing them with new
IR sequences. This pass will run by default in the ``refsi_tutorial``.
Alternatively the pass can be run at any point by targets with their own
customized pass pipelines. Part of the information it uses for the replacements
is defined by a ``BuiltinInfo`` class.

The pass needs more information to replace ``clmul*`` instructions. This can be
done by creating a custom inherited class from ``CLBuiltinInfo``. ``CLBuiltinInfo``
is a class that encapsulates information and transformations concerning
compiler OpenCL builtin functions.

First we define the class, which needs to define two methods, ``analyzeBuiltin``
and ``emitBuiltinInline`` as follows after the headers in ``module.cpp``:

.. code:: cpp

  #include <llvm/IR/IntrinsicsRISCV.h>
  #include <compiler/utils/mangling.h>

  namespace refsi_tutorial {
    class CLTargetBuiltinInfo : public compiler::utils::CLBuiltinInfo {
    public:
      // Will be filled out by each example
      CLTargetBuiltinInfo(std::unique_ptr<compiler::utils::CLBuiltinLoader> L)
          : compiler::utils::CLBuiltinInfo(std::move(L)) {}
      compiler::utils::Builtin analyzeBuiltin(
          llvm::Function const &Builtin) const override {
      }
      llvm::Value *emitBuiltinInline(
          llvm::Function *Builtin, llvm::IRBuilder<> &B,
          llvm::ArrayRef<llvm::Value *> Args) override {
      }
    };
  }

We will fill in the bodies of these function later in the tutorial.

Next we need to update ``createPassMachinery`` to register our
``CLTargetBuiltinInfo`` rather than the default one. This is done via setting a
callback for the ``PassMachinery`` class which is used to register an analysis pass
which uses a ``BuiltinInfo`` class as a basis.

In ``createPassMachinery`` we replace the lines:

.. code:: cpp

    auto Callback = [Builtins](const llvm::Module &) {
      return compiler::utils::BuiltinInfo(utils::createSimpleCLBuiltinInfo(Builtins));
    };

with

.. code:: cpp

    auto Callback = [Builtins](const llvm::Module &) {
      return compiler::utils::BuiltinInfo(createSimpleTargetCLBuiltinInfo(Builtins));
    };

This requires ``createSimpleTargetCLBuiltinInfo`` to be implemented. This
calls ``std::make_unique`` to create a unique pointer to a newly created
BILangInfoConcept templated with the new class and passes it a builtin loader.
This can be written as follows:

.. code:: cpp

  std::unique_ptr<compiler::utils::BILangInfoConcept> createSimpleTargetCLBuiltinInfo(
      llvm::Module *Builtins) {
    return std::make_unique<CLTargetBuiltinInfo>(
        std::make_unique<compiler::utils::SimpleCLBuiltinLoader>(Builtins));


At this point everything is plumbed in, but the class will do nothing different
to ``CLBuiltinInfo``. There are two parts to this. Firstly we need to implement
the ``analyzeBuiltin``. This has to return property information about the
builtin. In this case we want to say that it can be emitted inline. We also set
the no ``side effects`` property to assist optimization. This also demangles the
names of the builtins to match. Note that if we are not declaring any additional
BuiltinID values, we can use ``eBuiltinUnknown``. Replace ``analyzeBuiltin``
method with the following:

.. code:: cpp

      compiler::utils::NameMangler mangler(&Builtin.getParent()->getContext());
      llvm::StringRef BaseName = mangler.demangleName(Builtin.getName());

      if ((BaseName == "clmul") || (BaseName == "clmulh") ||
          (BaseName == "clmulr")) {
        unsigned Properties = compiler::utils::eBuiltinPropertyCanEmitInline |
                              compiler::utils::eBuiltinPropertyNoSideEffects;
        return (compiler::utils::BuiltinProperties)Properties;
        return compiler::utils::Builtin{
            Builtin, compiler::utils::eBuiltinUnknown,
            (compiler::utils::BuiltinProperties)Properties};
      }
      compiler::utils::CLBuiltinInfo::analyzeBuiltin(Builtin);

With this in place we update ``emitBuiltinInline`` by identifying calls to
``clmul``, ``clmulh`` and ``clmulr`` and replaces them with the equivalent LLVM
intrinsic.

.. code:: cpp

  if (Builtin) {
    compiler::utils::NameMangler mangler(&Builtin->getParent()->getContext());
    llvm::StringRef BaseName = mangler.demangleName(Builtin->getName()); 
    if (BaseName == "clmul") {
      return B.CreateIntrinsic(llvm::Intrinsic::riscv_clmul,
                              Builtin->getReturnType(), {Args[0], Args[1]});
    } else if (BaseName == "clmulh") {
      return B.CreateIntrinsic(llvm::Intrinsic::riscv_clmulh,
                              Builtin->getReturnType(), {Args[0], Args[1]});
    } else if (BaseName == "clmulr") {
      return B.CreateIntrinsic(llvm::Intrinsic::riscv_clmulr,
                              Builtin->getReturnType(), {Args[0], Args[1]});
    }
  }
  return compiler::utils::CLBuiltinInfo::emitBuiltinInline(Builtin, B, Args);

.. note::

 For simplicity, no validation or type checking is performed here.

Final Result
------------

With the above step completed, users can compile OpenCL programs using
``clmul*`` builtins that are optimally mapped directly to RISC-V instructions.
This can be done with the standalone compiler `clc` (which can be built with
``ninja clc``).

``clmul.cl``:

.. code:: c

 kernel void do_clmul(global long *a, global long *b, global long *z) {
   size_t id = get_global_id(0);
   z[id] = clmul(a[id], b[id]);
 }

.. code:: sh

 clc --strip-binary-header -o clmul.o clmul.cl -cl-wfv=never

 llvm-objdump --disassemble --triple=riscv64 --mattr="v,c,zbc" clmul.o | grep clmul

 0000000000010000 <do_clmul>:
    1003c: 33 96 c6 0a   clmul   a3, a5, a3

Lit Testing
-----------

Finally this can also be tested using a lit test, which is a common way of
testing passes in LLVM. The tool ``muxc`` can be used for this purpose in a similar
way to how the LLVM tool ``opt`` can be used. Create the following file
``compiler/refsi_tutorial/test/clmul_replace.ll``:

.. code::

  ; RUN: %muxc --device "RefSi M1 Tutorial" %s --passes "require<builtin-info>,optimal-builtin-replace,verify" -S | FileCheck %s

  target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
  target triple = "riscv64-unknown-unknown-elf"

  ; CHECK:   call i64 @llvm.riscv.clmul.i64
  ; CHECK:   call i64 @llvm.riscv.clmulh.i64
  ; CHECK:   call i64 @llvm.riscv.clmulr.i64

  ; Function Attrs: norecurse nounwind
  define spir_kernel void @do_clmul(ptr addrspace(1) %a, ptr addrspace(1) %b, ptr addrspace(1) %z) {
  entry:
    %0 = load i64, ptr addrspace(1) %a
    %1 = load i64, ptr addrspace(1) %b
    %call = tail call spir_func i64 @_Z5clmulll(i64 %0, i64 %1)
    %call4 = tail call spir_func i64 @_Z6clmulhll(i64 %0, i64 %1)
    %add = add nsw i64 %call4, %call
    %call7 = tail call spir_func i64 @_Z6clmulrll(i64 %0, i64 %1)
    %add8 = add nsw i64 %add, %call7
    store i64 %add8, ptr addrspace(1) %z
    ret void
  }

  declare spir_func i64 @_Z5clmulll(i64, i64)

  declare spir_func i64 @_Z6clmulhll(i64, i64)

  declare spir_func i64 @_Z6clmulrll(i64, i64)

``muxc`` is used here to run the the ``builtin-info`` analysis pass and the
``optimal-builtin-replace`` pass on the input IR. This results in the output of the
intrinsics ``llvm.riscv.clmul*``.

All of the lit tests for the ``refsi-tutorial`` target can be run with building the
``check-ock-refsi-tutorial-lit`` target. If you use the create script for
building new targets the target name will be used instead of
``refsi-tutorial``.

