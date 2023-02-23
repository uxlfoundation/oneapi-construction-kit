// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/builtin_info.h>
#include <compiler/utils/device_info.h>
#include <compiler/utils/replace_mux_math_decls_pass.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace llvm;

PreservedAnalyses compiler::utils::ReplaceMuxMathDeclsPass::run(
    Module &M, ModuleAnalysisManager &MAM) {
  bool Changed = false;

  auto &DI = MAM.getResult<DeviceInfoAnalysis>(M);
  // Abacus needs to know if denormal floats are supported, so it can avoid
  // Flush To Zero behaviour in intermediate operations of maths builtins.
  //
  // We make the assumption in abacus that double precision always supports
  // denormals, but if either single and half precision do not we set the FTZ
  // flag.
  bool flush_denorms_to_zero = false;
  bool is_embedded_profile = false;
  if (DI.float_capabilities) {
    flush_denorms_to_zero |= (0 == (DI.float_capabilities &
                                    device_floating_point_capabilities_denorm));

    is_embedded_profile |= (0 == (DI.float_capabilities &
                                  device_floating_point_capabilities_full));
  }

  if (DI.half_capabilities) {
    flush_denorms_to_zero |= (0 == (DI.half_capabilities &
                                    device_floating_point_capabilities_denorm));

    is_embedded_profile |=
        (0 == (DI.half_capabilities & device_floating_point_capabilities_full));
  }

#ifdef CA_CL_FORCE_PROFILE_STRING
  is_embedded_profile =
      cargo::string_view(CA_CL_FORCE_PROFILE_STRING) == "EMBEDDED_PROFILE";
#endif

  const std::array<std::pair<StringRef, const bool>, 3> MuxMathDecls = {
      std::make_pair(MuxBuiltins::isftz, flush_denorms_to_zero),
      std::make_pair(MuxBuiltins ::usefast, UseFast),
      std::make_pair(MuxBuiltins::isembeddedprofile, is_embedded_profile)};

  for (const auto &MathDecl : MuxMathDecls) {
    if (Function *const Fn = M.getFunction(MathDecl.first)) {
      Changed = true;

      // create an IR builder with a single basic block in our function
      IRBuilder<> IR(BasicBlock::Create(Fn->getContext(), "", Fn));

      IR.CreateRet(IR.getInt1(MathDecl.second));
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
