// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <base/image_argument_substitution_pass.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <multi_llvm/multi_llvm.h>

#include <map>

using namespace llvm;

namespace {
const std::map<std::string, std::string> funcToFuncMap = {{
#define PIAS_IMAGE1D "14ocl_image1d"
#define PIAS_IMAGE2D "14ocl_image2d"
#define PIAS_IMAGE3D "14ocl_image3d"
#define PIAS_IMAGE1DARRAY "20ocl_image1d_array"
#define PIAS_IMAGE2DARRAY "20ocl_image2d_array"
#define PIAS_IMAGE1DBUFFER "21ocl_image1d_buffer"
#define PIAS_RO "_ro"
#define PIAS_WO "_wo"
#include "image_argument_substitution_pass.inc"

#define PIAS_IMAGE1D "11ocl_image1d"
#define PIAS_IMAGE2D "11ocl_image2d"
#define PIAS_IMAGE3D "11ocl_image3d"
#define PIAS_IMAGE1DARRAY "16ocl_image1darray"
#define PIAS_IMAGE2DARRAY "16ocl_image2darray"
#define PIAS_IMAGE1DBUFFER "17ocl_image1dbuffer"
#define PIAS_RO ""
#define PIAS_WO ""
#include "image_argument_substitution_pass.inc"
}};
}  // namespace

PreservedAnalyses compiler::ImageArgumentSubstitutionPass::run(
    Module &module, ModuleAnalysisManager &) {
  bool module_modified = false;

  // we need to detect if the new sampler function is there and replace it if so
  {
    auto samplerInitFunc =
        module.getFunction("__translate_sampler_initializer");

    if (nullptr != samplerInitFunc) {
      module_modified = true;

      IRBuilder<> builder(
          BasicBlock::Create(module.getContext(), "entry", samplerInitFunc));

      auto arg = &*samplerInitFunc->arg_begin();

      builder.CreateRet(builder.CreateIntToPtr(
          arg, samplerInitFunc->getFunctionType()->getReturnType()));
    }
  }

  SmallVector<Instruction *, 8> toRemoves;

  // First we lookup the image type we are going to replace the opaque OpenCL
  // image types with
  StructType *imageType = nullptr;

  const char *imageName = "struct.Image";

  for (auto structType : module.getIdentifiedStructTypes()) {
    auto name = structType->getName();

    if (name.rtrim(".0123456789").equals(imageName)) {
      imageType = structType;
      break;
    }
  }

  if (nullptr == imageType) {
    imageType = StructType::create(module.getContext(), imageName);
  }

  for (const auto &pair : funcToFuncMap) {
    auto srcFunc = module.getFunction(pair.first);

    // if we didn't find the image function, we skip this loop iteration (the
    // function simply wasn't used in our user's kernels)
    if (nullptr == srcFunc) {
      continue;
    }

    // we found the function, so we definitely are modifying the module!
    module_modified = true;

    for (auto &use : srcFunc->uses()) {
      auto call = dyn_cast<CallInst>(use.getUser());

      assert(call && "User wasn't a call instruction!");

      auto dstFunc = module.getFunction(pair.second);

      // if we haven't got a declaration for our replacement function, we need
      // to make one!
      if (nullptr == dstFunc) {
        SmallVector<Type *, 8> types;

        types.push_back(PointerType::getUnqual(imageType));

        auto srcFuncType = srcFunc->getFunctionType();

        // by default, we'll just pass through the remaining arguments
        unsigned i = 1;

        // we need to change our approach for handling passing samplers into
        // libimg here too

        // have we got a function that has a sampler in its argument list?
        if (std::string::npos != pair.first.find("sampler")) {
          types.push_back(IntegerType::get(module.getContext(), 32));

          // the sampler will always be the second argument in the list
          i = 2;
        }

        for (unsigned e = srcFuncType->getNumParams(); i < e; i++) {
          types.push_back(srcFuncType->getParamType(i));
        }

        auto dstFuncType =
            FunctionType::get(srcFunc->getReturnType(), types, false);

        dstFunc = Function::Create(dstFuncType, srcFunc->getLinkage(),
                                   pair.second, &module);
        dstFunc->setCallingConv(srcFunc->getCallingConv());
      }

      SmallVector<Value *, 8> args;

      IRBuilder<> Builder(call->getContext());
      Builder.SetInsertPoint(call->getParent(), call->getIterator());

      // we are changing the opaque OpenCL image argument in arg 0 into our
      // own image struct pointer type (the bitcast). Our Image struct pointer
      // type is in the 0th address space, so we need to cast from the global
      // address space the original type had (the addrspacecast)
      args.push_back(Builder.CreateAddrSpaceCast(
          Builder.CreateBitCast(
              call->getArgOperand(0),
              imageType->getPointerTo(
                  call->getArgOperand(0)->getType()->getPointerAddressSpace())),
          PointerType::getUnqual(imageType)));

      // by default, we'll just pass through the remaining arguments
      unsigned i = 1;

      // we need to change our approach for handling samplers into libimg here
      // too

      // have we got a function that has a sampler in its argument list?
      if (std::string::npos != pair.first.find("sampler")) {
        args.push_back(Builder.CreatePtrToInt(
            call->getArgOperand(1),
            dstFunc->getFunctionType()->getParamType(1)));

        // the sampler will always be the second argument in the list
        i = 2;
      }

      for (unsigned e = call->arg_size(); i < e; i++) {
        args.push_back(call->getArgOperand(i));
      }

      auto ci = Builder.CreateCall(dstFunc, args);
      ci->setCallingConv(dstFunc->getCallingConv());
      call->replaceAllUsesWith(ci);
      toRemoves.push_back(call);
    }
  }

  // and remove all the calls that we replaced!
  for (auto remove : toRemoves) {
    remove->eraseFromParent();
  }

  // and finally remove the functions we have replaced
  for (const auto &pair : funcToFuncMap) {
    auto srcFunc = module.getFunction(pair.first);

    // if we didn't find the image function, we skip this loop iteration
    if (nullptr == srcFunc) {
      continue;
    }

    srcFunc->eraseFromParent();
  }

  return module_modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
