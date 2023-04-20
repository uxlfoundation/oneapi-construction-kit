// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <base/macros.h>
#include <base/program_metadata.h>
#include <base/spir_fixup_pass.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Analysis/MemoryDependenceAnalysis.h>
#include <llvm/Analysis/ModuleSummaryAnalysis.h>
#include <llvm/Analysis/ScalarEvolutionAliasAnalysis.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/MDBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/PassManager.h>
#include <multi_llvm/multi_llvm.h>

static const llvm::SmallDenseSet<llvm::StringRef, 8> work_item_funcs{{
    "_Z15get_global_sizej",
    "_Z13get_global_idj",
    "_Z17get_global_offsetj",
    "_Z14get_local_sizej",
    "_Z12get_local_idj",
    "_Z14get_num_groupsj",
    "_Z12get_group_idj",
}};

static bool fixupCC(llvm::Function &func) {
  if (!func.isIntrinsic()) {
    return false;
  }
  func.setCallingConv(llvm::CallingConv::C);
  for (auto *user : func.users()) {
    if (auto *callInst = llvm::dyn_cast<llvm::CallInst>(user)) {
      callInst->setCallingConv(llvm::CallingConv::C);
    }
  }
  return true;
}

static bool markNoUnwind(llvm::Function &F) {
  // we don't use exceptions
  if (F.hasFnAttribute(llvm::Attribute::NoUnwind)) {
    return false;
  }
  F.addFnAttr(llvm::Attribute::NoUnwind);
  return true;
}

static bool markReadOnly(llvm::Function &func) {
  // replace readnone with readonly for work_item_funcs
  if (!func.doesNotAccessMemory() ||
      work_item_funcs.count(func.getName()) == 0) {
    return false;
  }
#if LLVM_VERSION_LESS(16, 0)
  func.removeFnAttr(llvm::Attribute::ReadNone);
#else
  func.removeFnAttr(llvm::Attribute::Memory);
#endif
  func.setOnlyReadsMemory();
  for (auto *user : func.users()) {
    if (auto *CI = llvm::dyn_cast<llvm::CallInst>(user)) {
      if (CI->doesNotAccessMemory()) {
#if LLVM_VERSION_LESS(16, 0)
        CI->removeFnAttr(llvm::Attribute::ReadNone);
#else
        CI->removeFnAttr(llvm::Attribute::Memory);
#endif
      }
      CI->setOnlyReadsMemory();
    }
  }
  return true;
}

static bool fixAtomic(llvm::Function &func) {
  // the SPIR kernels we get have the wrong signature for atomics (because
  // of mangling order bug between const & volatile address spaces), so we
  // need to fix them up
  auto name = func.getName().str();

  bool modified = false;
  auto fixup = [&name, &modified](const std::string &pattern, const char c) {
    const size_t i = name.find(pattern);

    // if we found the offending pattern
    if (std::string::npos != i) {
      modified = true;
      // get the address space char
      const char as = name[i + pattern.size()];

      auto replacement = std::string("PU3AS");
      replacement += as;
      replacement += c;

      name.replace(i, replacement.size(), replacement);
    }
  };

  // fixup the broken const and volatile pointer patterns
  fixup("PKU3AS", 'K');
  fixup("PVU3AS", 'V');
  func.setName(name);
  return modified;
}

llvm::PreservedAnalyses compiler::spir::SpirFixupPass::run(
    llvm::Module &module, llvm::ModuleAnalysisManager &) {
  bool modifiedCFG = false, modifiedSCEV = false, modifiedAttrs = false;

  // sometimes LLVM intrinsics will be passed with the incorrect calling
  // convention and SPIR functions may have incorrect attributes.
  for (auto &func : module.functions()) {
    modifiedCFG |= fixupCC(func);
    modifiedAttrs |= markNoUnwind(func);
    modifiedAttrs |= markReadOnly(func);
  }
  for (auto &func : module.functions()) {
    modifiedCFG |= fixAtomic(func);
  }

  // TODO CA-1212: Document why this is necessary and what is going on.
  using MetadataReplacementMap = llvm::DenseMap<llvm::MDNode *, llvm::MDNode *>;
  auto MDB = llvm::MDBuilder(module.getContext());
  MetadataReplacementMap MRM;
  std::function<void(llvm::MDNode *, unsigned)> tbaaOperandPatternMatch =
      [&tbaaOperandPatternMatch, &MDB, &MRM](llvm::MDNode *node, unsigned i) {
        auto operand = llvm::cast<llvm::MDNode>(node->getOperand(i));
        if (MRM.count(operand) > 0) {
          node->replaceOperandWith(i, MRM.lookup(operand));
          return;
        }

        // Note: It is actually possible for the 'old' form TBAA to have three
        // operands here, where the last operand is the value '1' for constant.
        // However, I don't know how to distinguish between that and
        // three-operand form noting a 1 byte offset.  It never happens in the
        // SPIR CTS, and it will never happen with ComputeCpp.
        if (operand->getNumOperands() == 2) {
          // Recurse first so that the memoization works
          tbaaOperandPatternMatch(operand, 1);

          llvm::MDNode *newOperand = MDB.createTBAAScalarTypeNode(
              llvm::cast<llvm::MDString>(operand->getOperand(0))->getString(),
              llvm::cast<llvm::MDNode>(operand->getOperand(1)), 0);
          MRM.try_emplace(operand, newOperand);
          node->replaceOperandWith(i, newOperand);
        }
      };
  auto tbaaNodePatternMatch = [&tbaaOperandPatternMatch](llvm::MDNode *node) {
    bool isStructPathTBAA = llvm::isa<llvm::MDNode>(node->getOperand(0)) &&
                            node->getNumOperands() >= 3;
    if (isStructPathTBAA) {
      tbaaOperandPatternMatch(node, 0);
      tbaaOperandPatternMatch(node, 1);
    }
  };
  auto tbaaPatternMatch = [&tbaaNodePatternMatch](llvm::Instruction &I) {
    if (I.hasMetadata()) {
      if (llvm::MDNode *MD = I.getMetadata(llvm::LLVMContext::MD_tbaa)) {
        tbaaNodePatternMatch(MD);
      }
    }
  };
  for (auto &F : module.functions()) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        tbaaPatternMatch(I);
      }
    }
  }

  // From version 4.0 onwards clang produces a sampler_t pointer parameter
  // for sampler arguments, but spir still mandates that this is an i32.
  // This causes issues from the run-time aspect which expects the
  // pointer. For this reason we create a new function if we have a
  // sampler parameter which takes the sampler_t pointer arguments,
  // converts them to i32 and then calls the original function.

  auto process_kernel = [&module, &modifiedCFG](KernelInfo kernel_info) {
    llvm::PointerType *samplerTypePtr = nullptr;

    // Check to see if any of the arguments are sampler
    bool hasSamplerArg = std::any_of(
        kernel_info.argument_types.cbegin(), kernel_info.argument_types.cend(),
        [&](compiler::ArgumentType type) {
          return type.kind == compiler::ArgumentKind::SAMPLER;
        });

    if (hasSamplerArg) {
      // Get the sampler struct and create a pointer to it if not already
      // done so
      if (nullptr == samplerTypePtr) {
        auto *samplerType =
            multi_llvm::getStructTypeByName(module, "opencl.sampler_t");

        if (nullptr == samplerType) {
          samplerType =
              llvm::StructType::create(module.getContext(), "opencl.sampler_t");
        }
        samplerTypePtr = llvm::PointerType::get(samplerType, 2);
      }

      // Create a duplicate function type, but with sampler struct
      // pointers where we previously had samplers as i32
      if (llvm::Function *func = module.getFunction(kernel_info.name)) {
        modifiedCFG = true;
        auto *funcTy = func->getFunctionType();
        unsigned numParams = funcTy->getNumParams();
        llvm::SmallVector<llvm::Type *, 8> newParamTypes(numParams);
        for (unsigned i = 0; i < numParams; i++) {
          if (kernel_info.argument_types[i].kind ==
              compiler::ArgumentKind::SAMPLER) {
            newParamTypes[i] = samplerTypePtr;
          } else {
            newParamTypes[i] = funcTy->getParamType(i);
          }
        }
        auto newFuncTy = llvm::FunctionType::get(funcTy->getReturnType(),
                                                 newParamTypes, false);

        // create our new function, using the linkage from the old one
        auto newFunc =
            llvm::Function::Create(newFuncTy, func->getLinkage(), "", &module);

        // set the correct calling convention and copy attributes
        newFunc->setCallingConv(func->getCallingConv());
        newFunc->copyAttributesFrom(func);

        // old func has been wrapped and shouldn't be classed as a kernel
        func->setCallingConv(llvm::CallingConv::SPIR_FUNC);

        // propagate the calling conv update to any users
        for (const auto &use : func->users()) {
          if (llvm::CallInst *call = llvm::dyn_cast<llvm::CallInst>(use)) {
            call->setCallingConv(func->getCallingConv());
          }
        }

        // take the name of the old function
        newFunc->takeName(func);

        if (auto entry = llvm::BasicBlock::Create(module.getContext(), "entry",
                                                  newFunc)) {
          llvm::IRBuilder<> ir(entry);
          llvm::SmallVector<llvm::Value *, 8> argsv;
          // set up the arguments for the original function, using the
          // current arguments but casting and truncating any sampler ones
          for (auto &arg : newFunc->args()) {
            if (arg.getType() == samplerTypePtr) {
              const llvm::DataLayout &dataLayout = module.getDataLayout();
              auto ptrToInt = ir.CreatePtrToInt(
                  &arg,
                  llvm::IntegerType::get(module.getContext(),
                                         dataLayout.getPointerSizeInBits(0)));
              auto trunc = ir.CreateTrunc(ptrToInt, ir.getInt32Ty());
              argsv.push_back(trunc);
            } else {
              argsv.push_back(&arg);
            }
          }
          llvm::ArrayRef<llvm::Value *> args(argsv);
          auto call = ir.CreateCall(func, args);
          call->setCallingConv(func->getCallingConv());
          ir.CreateRetVoid();
        }
        if (auto *namedMetaData = module.getNamedMetadata("opencl.kernels")) {
          for (auto *md : namedMetaData->operands()) {
            if (md && (md->getOperand(0) == llvm::ValueAsMetadata::get(func))) {
              md->replaceOperandWith(0, llvm::ValueAsMetadata::get(newFunc));
            }
          }
        }
      }
    } else {
      // In cases where there is no sampler we want to make sure that the
      // calling convention of the call to a spir kernel is set to
      // `SPIR_KERNEL`. Otherwise the calling conventions will mismatch which is
      // considered an undefined behaviour and will be considered an illegal
      // instruction.
      if (module.getFunction(kernel_info.name)->getCallingConv() ==
          llvm::CallingConv::SPIR_KERNEL) {
        for (auto user : module.getFunction(kernel_info.name)->users()) {
          if (auto ci = llvm::dyn_cast<llvm::CallInst>(user)) {
            modifiedCFG = true;
            ci->setCallingConv(llvm::CallingConv::SPIR_KERNEL);
          }
        }
      }
    }
  };

  // ProgramInfo reads the kernel metadata for the module
  (void)moduleToProgramInfo(process_kernel, &module, true);

  // According to the spir spec available_externally is supposed to represent
  // C99 inline semantics. The closest thing llvm has natively is LinkOnce.
  // It doesn't quite give us the same behaviour, but it does assume a more
  // definitive definition might exist outside the module, which is good enough
  // to not go catastrophically awry.
  for (auto &global : module.globals()) {
    if (global.getLinkage() == llvm::GlobalValue::AvailableExternallyLinkage) {
      global.setLinkage(llvm::GlobalValue::LinkOnceAnyLinkage);
    }
  }

  for (auto &func : module.functions()) {
    if (func.getLinkage() == llvm::GlobalValue::AvailableExternallyLinkage) {
      func.setLinkage(llvm::GlobalValue::LinkOnceAnyLinkage);
    }
  }
  llvm::PreservedAnalyses pa;
  if (!modifiedCFG) {
    pa.preserveSet<llvm::CFGAnalyses>();
  }
  if (!modifiedSCEV) {
    pa.preserve<llvm::ScalarEvolutionAnalysis>();
  }
  if (modifiedAttrs) {
    pa.abandon<llvm::BasicAA>();
    pa.abandon<llvm::ModuleSummaryIndexAnalysis>();
    pa.abandon<llvm::MemoryDependenceAnalysis>();
  }
  return pa;
}
