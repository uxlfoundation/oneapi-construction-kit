// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <base/printf_replacement_pass.h>
#include <compiler/utils/builtin_info.h>
#include <compiler/utils/device_info.h>
#include <compiler/utils/pass_functions.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Analysis/OptimizationRemarkEmitter.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DiagnosticInfo.h>
#include <llvm/IR/DiagnosticPrinter.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/MDBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/FormatVariadic.h>
#include <multi_llvm/vector_type_helper.h>
#include <stddef.h>
#include <stdint.h>

#include <iterator>
#include <optional>

#define PASS_NAME "replace-printf"

using namespace llvm;

namespace {

/// @brief Increments the given pointer.
/// @param[in,out] fmt A pointer to the char* to increment.
/// @return True if @p fmt now points to the null-terminator character '\0'.
bool IncrementPtr(const char **fmt) {
  if (*(++(*fmt)) == '\0') {
    return true;
  }
  return false;
}

/// @brief This function transforms an OpenCL printf format string into a
/// C99-conformant one.

/// Its main job is to scalarize vector format specifiers into scalarized form.
/// It does this by taking a vector specifier and determining the specifier
/// corresponding to each vector element. It then emits the element specifier
/// into the new format string for each element in the vector, separated by a
/// comma.
///
/// Special care needs to be taken for modifiers that aren't supported by C99
/// such as the 'hl' length modifier. The new format string will have 'hl'
/// stripped out.
///
/// Examples:
/// @code{.cpp}
/// // vector 2, 8-bit sized hexadecimal integers
/// "%v2hhx"  --> "%hhx,%hhx"
/// // vector 4, 32-bit sized floats
/// "%v4hlf"  --> "%f,%f,%f,%f"
/// @endcode
///
/// It also does some checking to ensure the printf string is conformant to the
/// OpenCL 1.2 specification, and returns an error if it is not.
/// @param[in] str The format string to scalarize and check.
/// @param[out] new_str The new, scalarized, format string.
/// @param[out] num_specifiers The number of specifiers parsed, ignoring %%
/// specifiers.
/// @return The Error::success if successful, else an Error if we detected an
/// illegal OpenCL printf format string.
Error scalarizeAndCheckFormatString(const std::string &str,
                                    std::string &new_str,
                                    size_t &num_specifiers) {
  // Set some sensible defaults in case we return error
  new_str = "";
  constexpr const char *RanOffEnd = "unexpected end of format string";

  num_specifiers = 0;

  const char *fmt = str.c_str();

  while (*fmt != '\0') {
    if (*fmt != '%') {
      new_str += *fmt;
    } else {
      std::string specifier_string(1, *fmt);

      if (IncrementPtr(&fmt)) {
        return createStringError(inconvertibleErrorCode(), RanOffEnd);
      }

      // don't count %% specifiers
      if (*fmt == '%') {
        new_str += "%%";
        ++fmt;
        continue;
      }

      // Parse (zero or more) Flags
      const char *flag_chars = "-+ #0";
      while (strchr(flag_chars, *fmt)) {
        specifier_string += *fmt;
        if (IncrementPtr(&fmt)) {
          return createStringError(inconvertibleErrorCode(), RanOffEnd);
        }
      }

      // Parse (optional) Width
      if (*fmt == '*') {
        // we don't currently support '*'
        return createStringError(
            inconvertibleErrorCode(),
            "the '*' width sub-specifier is not supported");
      }
      if (isdigit(*fmt)) {
        while (isdigit(*fmt)) {
          specifier_string += *fmt;
          if (IncrementPtr(&fmt)) {
            return createStringError(inconvertibleErrorCode(), RanOffEnd);
          }
        }
      }

      // Parse (optional) Precision
      if (*fmt == '.') {
        specifier_string += *fmt;
        if (IncrementPtr(&fmt)) {
          return createStringError(inconvertibleErrorCode(), RanOffEnd);
        }

        if (*fmt == '*') {
          // we don't currently support '*'
          return createStringError(
              inconvertibleErrorCode(),
              "the '*' width sub-specifier is not supported");
        }

        while (isdigit(*fmt)) {
          specifier_string += *fmt;
          if (IncrementPtr(&fmt)) {
            return createStringError(inconvertibleErrorCode(), RanOffEnd);
          }
        }
      }

      uint32_t vector_length = 1u;
      const bool is_vector = *fmt == 'v';
      // Parse (optional) Vector Specifier
      if (is_vector) {
        if (IncrementPtr(&fmt)) {
          return createStringError(inconvertibleErrorCode(), RanOffEnd);
        }
        switch (*fmt) {
          default:
            return createStringError(
                inconvertibleErrorCode(),
                formatv("invalid vector length modifier '{0}'", *fmt));
          case '1':
            // Must be 16, else error
            if (IncrementPtr(&fmt)) {
              return createStringError(inconvertibleErrorCode(), RanOffEnd);
            }
            if (*fmt != '6') {
              return createStringError(
                  inconvertibleErrorCode(),
                  formatv("invalid vector length modifier '1{0}'", *fmt));
            }
            vector_length = 16u;
            break;
          case '2':
            vector_length = 2u;
            break;
          case '3':
            vector_length = 3u;
            break;
          case '4':
            vector_length = 4u;
            break;
          case '8':
            vector_length = 8u;
            break;
        }
        if (IncrementPtr(&fmt)) {
          return createStringError(inconvertibleErrorCode(), RanOffEnd);
        }
      }

      // Parse Length Modifier
      const char *length_modifier_chars = "hljztL";
      // Length Modifier is required with Vector Specifier
      bool has_used_l_length_modifier = false;
      const bool has_supplied_length_modifier =
          strchr(length_modifier_chars, *fmt);
      if (is_vector && !has_supplied_length_modifier) {
        return createStringError(
            inconvertibleErrorCode(),
            "vector specifiers must be supplied length modifiers");
      }

      if (has_supplied_length_modifier) {
        bool consume_next_char = true;
        switch (*fmt) {
          default:
            // The 'j', 'z', 't', and 'L' length modifiers are not supported by
            // OpenCL C.
            return createStringError(
                inconvertibleErrorCode(),
                formatv("invalid length modifier '{0}'", *fmt));
          case 'h':
            if (IncrementPtr(&fmt)) {
              return createStringError(inconvertibleErrorCode(), RanOffEnd);
            }
            if (*fmt == 'h') {
              specifier_string += "hh";
            } else if (*fmt == 'l') {
              // Native printf doesn't recognize 'hl' so we don't
              // add it to the new format string.  Luckily, 'hl'
              // is sizeof(int) - the same as the default on
              // native printf!

              // Additionally, 'hl' modifier may only be used in
              // conjunction with the vector specifier
              if (!is_vector) {
                return createStringError(inconvertibleErrorCode(),
                                         "the 'hl' length modifier may only be "
                                         "used with the vector specifier");
              }
            } else {
              specifier_string += 'h';
              // We've already incremented the ptr and we found nothing; don't
              // do it again
              consume_next_char = false;
            }
            break;
          case 'l':
            specifier_string += *fmt;
            // Check ahead to see if the user is using the invalid 'll' length
            // modifier
            if (IncrementPtr(&fmt)) {
              return createStringError(inconvertibleErrorCode(), RanOffEnd);
            }
            if (*fmt == 'l') {
              return createStringError(inconvertibleErrorCode(),
                                       "the 'll' length modifier is invalid");
            }
            // We've already incremented the ptr; don't do it again

            // The 'l' modifier for the OpenCL printf expects 64 bits
            // integers, check if the system's long are actually 64 bits wide
            // and if not upgrade the format modifier to 'll'.
            //
            // FIXME: This only works for host based devices, which is fine for
            // our current printf implementation, but it should really be
            // removed once we have a proper printf implementation.
            if (sizeof(long) != 8) {
              specifier_string += 'l';
            }

            consume_next_char = false;
            has_used_l_length_modifier = true;
            break;
        }
        if (consume_next_char) {
          if (IncrementPtr(&fmt)) {
            return createStringError(inconvertibleErrorCode(), RanOffEnd);
          }
        }
      }

      // Parse Specifier
      specifier_string += *fmt;

      switch (*fmt) {
        default:
          break;
        case 'n':
          // The 'n' conversion specifier is not supported by OpenCL C.
          return createStringError(inconvertibleErrorCode(),
                                   "the 'n' conversion specifier is not "
                                   "supported by OpenCL C but is reserved");
        case 's':  // Intentional fall-through
        case 'c':
          // The 'l' length modifier followed by the 'c' or 's' conversion
          // specifiers is not supported by OpenCL C.
          if (has_used_l_length_modifier) {
            return createStringError(
                inconvertibleErrorCode(),
                "the 'l' length modifier followed by a 'c' conversion "
                "specifier or 's' conversion specifier is not supported by "
                "OpenCL C");
          }
          break;
      }

      // Output the %specifier for each element of the vector,
      // and for every element but the last, follow it by a "," string.
      for (uint32_t i = 0; i < vector_length; ++i) {
        ++num_specifiers;
        new_str += specifier_string;

        if (i < (vector_length - 1)) {
          new_str += ",";
        }
      }
    }
    ++fmt;
  }

  new_str += '\0';

  return Error::success();
}

std::optional<std::string> getPointerToStringAsString(Value *op) {
  // Check whether the value is being passed directly as the GlobalVariable.
  // This is possible with opaque pointers so will eventually become the
  // default assumption.
  GlobalVariable *var = dyn_cast<GlobalVariable>(op);
  if (!var) {
    if (auto const_string = dyn_cast<ConstantExpr>(op)) {
      switch (const_string->getOpcode()) {
        case Instruction::GetElementPtr:
        case Instruction::AddrSpaceCast:
          var = dyn_cast<GlobalVariable>(const_string->getOperand(0));
          break;
        case Instruction::IntToPtr:
          // Sometimes we see a PtrToInt expression inside an IntToPtr
          // expression, so therefore we need to unwrap it twice.
          const_string = dyn_cast<ConstantExpr>(const_string->getOperand(0));
          if (const_string &&
              const_string->getOpcode() == Instruction::PtrToInt) {
            var = dyn_cast<GlobalVariable>(const_string->getOperand(0));
          }
          break;
        default:
          break;
      }
    } else if (auto gep_string = dyn_cast<GetElementPtrInst>(op)) {
      var = dyn_cast<GlobalVariable>(gep_string->getPointerOperand());
    } else if (auto load_string = dyn_cast<LoadInst>(op)) {
      // If optimizations are off, we might first store the string in an alloca,
      // and then retrieve it in a load.
      if (auto ptr_string =
              dyn_cast<AllocaInst>(load_string->getPointerOperand())) {
        for (User *U : ptr_string->users()) {
          if (auto store_string = dyn_cast<StoreInst>(U)) {
            // We only expect a direct store of a global variable or a GEP of
            // one.
            auto *const store_val = store_string->getValueOperand();
            var = dyn_cast<GlobalVariable>(store_val);
            if (var) {
              break;
            }
            if (auto const_string = dyn_cast<ConstantExpr>(store_val)) {
              if (const_string->getOpcode() == Instruction::GetElementPtr) {
                var = dyn_cast<GlobalVariable>(const_string->getOperand(0));
                if (var) {
                  break;
                }
              }
            }
          }
        }
      }
    }
  }

  if (!var || !var->hasInitializer()) {
    return std::nullopt;
  }

  Constant *const string_const = var->getInitializer();

  if (auto array_string = dyn_cast<ConstantDataSequential>(string_const)) {
    return array_string->getAsString().str();
  }
  if (isa<ConstantAggregateZero>(string_const)) {
    return "";
  }

  return std::nullopt;
}

/// @brief A small wrapper function around IRBuilder<>::CreateCall that
///        sets calling conventions.
/// @param[in] ir The IRBuilder to use when creating the call.
/// @param[in] f The function to call.
/// @param[in] args The arguments to use at the call-site.
/// @return A call instruction that calls @p f with @p args.
CallInst *CreateCall(IRBuilder<> &ir, Function *const f,
                     ArrayRef<Value *> args) {
  auto ci = ir.CreateCall(f, args);
  ci->setCallingConv(f->getCallingConv());
  return ci;
}

/// @brief Recursively search up the call graph and add each function
///        encountered into set_of_callers. Currently not needed in debug
///        builds, since debug passes add printf calls to all functions.
/// @param[in,out] F The function that will be traced
/// @param[in,out] set_of_callers The set of calling functions
void findAndRecurseFunctionUsers(
    Function *F, SmallPtrSetImpl<const Function *> &set_of_callers) {
  // See if the function has already been flagged
  if (set_of_callers.count(F)) {
    return;
  }

  set_of_callers.insert(F);

  // Recurse into any function callers
  for (auto *user : F->users()) {
    if (auto ci = dyn_cast<CallInst>(user)) {
      // Get enclosing function
      Function *parent = ci->getFunction();
      findAndRecurseFunctionUsers(parent, set_of_callers);
    }
  }
}

Type *getBufferEltTy(LLVMContext &c) { return IntegerType::getInt8Ty(c); }

class DiagnosticInfoScrubbedPrintf : public DiagnosticInfoWithLocationBase {
  Twine Msg;

 public:
  static int DK_ScrubbedPrintf;

  DiagnosticInfoScrubbedPrintf(const Instruction &I, const Twine &Msg,
                               DiagnosticSeverity DS)
      : DiagnosticInfoWithLocationBase(
            static_cast<DiagnosticKind>(DK_ScrubbedPrintf), DS,
            *I.getFunction(), I.getDebugLoc()),
        Msg(Msg) {}

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_ScrubbedPrintf;
  }

  const Twine &getMessage() const { return Msg; }

  void print(DiagnosticPrinter &DP) const override {
    DP << getLocationStr() << ": " << getMessage();
  }
};

int DiagnosticInfoScrubbedPrintf::DK_ScrubbedPrintf =
    getNextAvailablePluginDiagnosticKind();
}  // namespace

// 6.15.14.3:
// In OpenCL C, printf returns 0 if it was executed successfully and -1
// otherwise vs. C99 where printf returns the number of characters printed or a
// negative value if an output or encoding error occurred.
constexpr int invalid_printf_ret = -1;

void compiler::PrintfReplacementPass::rewritePrintfCall(
    Module &module, CallInst *ci, Function *printf_func, Function *get_group_id,
    Function *get_num_groups, PrintfDescriptorVecTy &printf_calls) {
  const AtomicOrdering ordering = AtomicOrdering::SequentiallyConsistent;

  auto *const size_t_type = compiler::utils::getSizeType(module);

  // get a printf descriptor for the call site
  printf_calls.resize(printf_calls.size() + 1);
  builtins::printf::descriptor &printf_desc = printf_calls.back();

  IRBuilder<> ir(ci);
  const OptimizationRemarkEmitter ORE(ci->getFunction());

  // get the format string
  std::optional<std::string> format_string =
      getPointerToStringAsString(ci->getArgOperand(0));

  if (!format_string) {
    // if it's invalid replace the call with an error code
    ci->replaceAllUsesWith(ir.getInt32(invalid_printf_ret));

    // FIXME: we need this for now because we are using the list of printf
    // calls to know if we need to add the printf buffer or not
    printf_desc.format_string = std::string("");

    // Emit what went wrong
    module.getContext().diagnose(DiagnosticInfoScrubbedPrintf(
        *ci, "could not retrieve format string", DS_Warning));
    return;
  }

  // validate and scalarize the format string
  std::string scalarized_format_string;
  size_t num_specifiers;
  if (auto err = scalarizeAndCheckFormatString(
          *format_string, scalarized_format_string, num_specifiers)) {
    // if it's invalid replace the call with an error code
    ci->replaceAllUsesWith(ir.getInt32(invalid_printf_ret));

    // FIXME: we need this for now because we are using the list of printf
    // calls to know if we need to add the printf buffer or not
    printf_desc.format_string = std::string("");

    // Emit what went wrong
    module.getContext().diagnose(DiagnosticInfoScrubbedPrintf(
        *ci, toString(std::move(err)), DS_Warning));
    // And give some context
    std::string escaped_format_str;
    raw_string_ostream OS(escaped_format_str);
    OS << "in format string \"";
    printEscapedString(*format_string, OS);
    OS << "\"";
    module.getContext().diagnose(
        DiagnosticInfoScrubbedPrintf(*ci, OS.str(), DS_Note));

    // move on to the next call site
    return;
  }

  printf_desc.format_string = scalarized_format_string;

  // printf buffer is the last argument of the parent function
  Argument *printfBuffer =
      compiler::utils::getLastArgument(ci->getParent()->getParent());

  // Prepare the arguments of the new printf call
  SmallVector<Value *, 32> new_args;
  SmallVector<Type *, 32> new_args_types;

  // the first argument of the call is the printf buffer
  new_args.push_back(printfBuffer);
  new_args_types.push_back(printfBuffer->getType());

  for (unsigned i = 1; i < ci->arg_size(); ++i) {
    // drop any extra arguments
    if (i > num_specifiers) {
      continue;
    }

    auto op = ci->getArgOperand(i);
    auto opType = op->getType();

    // first scalarize vector arguments
    SmallVector<Value *, 16> scalars;
    if (opType->isVectorTy()) {
      const uint32_t numElements = multi_llvm::getVectorNumElements(opType);
      for (unsigned j = 0; j < numElements; ++j) {
        scalars.push_back(ir.CreateExtractElement(op, ir.getInt32(j)));
      }
    } else {
      scalars.push_back(op);
    }

    // then process them
    for (unsigned j = 0; j < scalars.size(); ++j) {
      // drop any extra arguments to the printf call
      if ((i + j) > num_specifiers) {
        continue;
      }

      auto &arg = scalars[j];
      auto type = arg->getType();

      if (type->isPointerTy()) {
        // TODO(8769): what if the user is trying to print the address of a
        // constant string.
        if (auto stringarg = getPointerToStringAsString(arg)) {
          // this is a string argument
          printf_desc.strings.push_back(*stringarg);
          printf_desc.types.push_back(builtins::printf::type::STRING);
        } else {
          // this is a pointer argument
          new_args.push_back(ir.CreatePtrToInt(arg, size_t_type));
          new_args_types.push_back(size_t_type);

          const unsigned size = size_t_type->getPrimitiveSizeInBits();
          printf_desc.types.push_back((size == 32)
                                          ? builtins::printf::type::INT
                                          : builtins::printf::type::LONG);
        }
      } else if (type->isDoubleTy()) {
        if (!double_support) {
          // trunc the double back to float
          new_args.push_back(ir.CreateFPTrunc(arg, ir.getFloatTy()));
          new_args_types.push_back(ir.getFloatTy());
          printf_desc.types.push_back(builtins::printf::type::FLOAT);
        } else {
          new_args.push_back(arg);
          new_args_types.push_back(type);
          printf_desc.types.push_back(builtins::printf::type::DOUBLE);
        }
      } else if (type->isFloatTy()) {
        if (!double_support) {
          new_args.push_back(arg);
          new_args_types.push_back(ir.getFloatTy());
          printf_desc.types.push_back(builtins::printf::type::FLOAT);
        } else {
          // if somehow the float wasn't expanded by clang, expand it
          new_args.push_back(ir.CreateFPExt(arg, ir.getDoubleTy()));
          new_args_types.push_back(ir.getDoubleTy());
          printf_desc.types.push_back(builtins::printf::type::DOUBLE);
        }
      } else if (type->isHalfTy()) {
        if (!double_support) {
          new_args.push_back(ir.CreateFPExt(arg, ir.getFloatTy()));
          new_args_types.push_back(ir.getFloatTy());
          printf_desc.types.push_back(builtins::printf::type::FLOAT);
        } else {
          new_args.push_back(ir.CreateFPExt(arg, ir.getDoubleTy()));
          new_args_types.push_back(ir.getDoubleTy());
          printf_desc.types.push_back(builtins::printf::type::DOUBLE);
        }
      } else {
        new_args.push_back(arg);
        new_args_types.push_back(type);

        switch (type->getPrimitiveSizeInBits()) {
          case 64:
            printf_desc.types.push_back(builtins::printf::type::LONG);
            break;
          case 32:
            printf_desc.types.push_back(builtins::printf::type::INT);
            break;
          case 16:
            printf_desc.types.push_back(builtins::printf::type::SHORT);
            break;
          case 8:
            printf_desc.types.push_back(builtins::printf::type::CHAR);
            break;
          default:
            llvm_unreachable("Unsupported printf argument");
        }
      }
    }
  }

  // now create a new printf function for the call
  auto call_function_type =
      FunctionType::get(ir.getInt32Ty(), new_args_types, false);

  Function *const callFunction = Function::Create(
      call_function_type, GlobalValue::LinkOnceODRLinkage, "", &module);

  callFunction->setCallingConv(printf_func->getCallingConv());
  callFunction->addFnAttr(Attribute::AlwaysInline);
  callFunction->setSubprogram(printf_func->getSubprogram());

  auto entry_block =
      BasicBlock::Create(module.getContext(), "entry", callFunction);
  auto early_return_block =
      BasicBlock::Create(module.getContext(), "early_return", callFunction);
  auto store_block =
      BasicBlock::Create(module.getContext(), "store", callFunction);

  // the buffer is the first argument of the function
  Argument *full_buffer = (&*callFunction->arg_begin());

  auto *buffer_elt_ty = getBufferEltTy(module.getContext());

  // Double-check the buffer is the type we expect, unless it's opaque.
  assert(full_buffer->getType()->isPointerTy() && "Unknown buffer type");

  // entry block
  ir.SetInsertPoint(entry_block);

  // first get the number of work group and our work group id in each dimension
  auto x_group_nums = ir.CreateTrunc(
      CreateCall(ir, get_num_groups, {ir.getInt32(0)}), ir.getInt32Ty());
  auto y_group_nums = ir.CreateTrunc(
      CreateCall(ir, get_num_groups, {ir.getInt32(1)}), ir.getInt32Ty());
  auto z_group_nums = ir.CreateTrunc(
      CreateCall(ir, get_num_groups, {ir.getInt32(2)}), ir.getInt32Ty());

  auto x_group_id = ir.CreateTrunc(
      CreateCall(ir, get_group_id, {ir.getInt32(0)}), ir.getInt32Ty());
  auto y_group_id = ir.CreateTrunc(
      CreateCall(ir, get_group_id, {ir.getInt32(1)}), ir.getInt32Ty());
  auto z_group_id = ir.CreateTrunc(
      CreateCall(ir, get_group_id, {ir.getInt32(2)}), ir.getInt32Ty());

  // compute a unique index for our work group in the printf buffer across all
  // dimensions:
  //   x + y * x_size + z * (x_size * y_size)
  auto group_addr = ir.CreateAdd(
      x_group_id,
      ir.CreateAdd(
          ir.CreateMul(x_group_nums, y_group_id),
          ir.CreateMul(z_group_id, ir.CreateMul(y_group_nums, z_group_nums))));

  // compute the size available to the work group
  // Ensure the size (and therefore the start of buffer for each work item) is
  // aligned to 4 bytes by doing &~3, because the atomic add below assumes
  // alignment to its type (int32).
  auto group_buffer_size = ir.CreateAnd(
      ir.CreateUDiv(
          ir.getInt32(printf_buffer_size),
          ir.CreateMul(x_group_nums, ir.CreateMul(y_group_nums, z_group_nums))),
      ir.getInt32(~3u));

  // get the chunk of buffer that this work group can use
  auto buffer = ir.CreateGEP(buffer_elt_ty, full_buffer,
                             ir.CreateMul(group_addr, group_buffer_size));

  // offset for the printf call, we create this call now but will re-write it
  // later when we know how much we need to add
  auto call_offset = ir.CreateAtomicRMW(
      AtomicRMWInst::Add,
      ir.CreatePointerCast(buffer, ir.getInt32Ty()->getPointerTo(1)),
      ir.getInt32(0), MaybeAlign(), ordering, SyncScope::System);

  // store block
  ir.SetInsertPoint(store_block);

  // store the id of the printf call, for that we use the index of the
  // printf call in the printf descriptors array
  ir.CreateAlignedStore(
      ir.getInt32(printf_calls.size() - 1),
      ir.CreatePointerCast(ir.CreateGEP(buffer_elt_ty, buffer, call_offset),
                           ir.getInt32Ty()->getPointerTo(1)),
      Align(1));
  // argument offset, starts at 4 to account for the printf call's id
  size_t offset = 4;

  // store the arguments in the buffer
  for (Argument &a : callFunction->args()) {
    // skip the printf buffer
    if (0 == a.getArgNo()) {
      continue;
    }

    Argument *arg = &a;
    auto type = arg->getType();

    // offset of the argument
    auto argOffset = ir.CreateAdd(call_offset, ir.getInt32(offset));

    // index into the global array
    auto gep = ir.CreateGEP(buffer_elt_ty, buffer, argOffset);

    // offset by the number of bytes of the type
    const unsigned size = type->getPrimitiveSizeInBits();
    offset += size / 8;

    // cast the pointer to the larger type and store the value
    ir.CreateAlignedStore(arg, ir.CreatePointerCast(gep, type->getPointerTo(1)),
                          Align(1));
  }

  // and return 0;
  ir.CreateRet(ir.getInt32(0));

  // entry block
  ir.SetInsertPoint(entry_block);

  // rewrite the atomic add with the amount of data the store block wants to
  // store
  auto correct_add = ir.CreateAtomicRMW(
      AtomicRMWInst::Add,
      ir.CreatePointerCast(buffer, ir.getInt32Ty()->getPointerTo(1)),
      ir.getInt32(offset), MaybeAlign(), ordering, SyncScope::System);
  call_offset->replaceAllUsesWith(correct_add);

  // delete the old call
  call_offset->dropAllReferences();
  call_offset->eraseFromParent();

  // create the condition to detect overflows, if the printf call doesn't
  // have enough space in the buffer return -1, we weight the branches in
  // favor of the store block as running out of space is unlikely to happen
  MDBuilder md(module.getContext());
  ir.CreateCondBr(
      ir.CreateICmpUGT(ir.CreateAdd(correct_add, ir.getInt32(offset)),
                       group_buffer_size),
      early_return_block, store_block, md.createBranchWeights(0, 1));

  // early return block
  ir.SetInsertPoint(early_return_block);

  // write how much data was not written to the buffer but is accounted for by
  // the length because of the first atomic add
  //
  auto *cast = ir.CreatePointerCast(buffer, ir.getInt32Ty()->getPointerTo(1));

  ir.CreateAtomicRMW(
      AtomicRMWInst::Add, ir.CreateGEP(ir.getInt32Ty(), cast, ir.getInt32(1)),
      ir.getInt32(offset), MaybeAlign(), ordering, SyncScope::System);

  // return -1
  ir.CreateRet(ir.getInt32(-1));

  // finally replace the call instruction with a call to our new function
  const std::string name = ci->getName().str();
  ci->setName("");

  auto newCi = CallInst::Create(callFunction, new_args, name, ci);
  newCi->setDebugLoc(ci->getDebugLoc());
  newCi->setCallingConv(ci->getCallingConv());

  ci->replaceAllUsesWith(newCi);
}

compiler::PrintfReplacementPass::PrintfReplacementPass(PrintfDescriptorVecTy *p,
                                                       size_t s)
    : printf_calls_out_ptr(p), printf_buffer_size(s) {}

PreservedAnalyses compiler::PrintfReplacementPass::run(
    Module &module, ModuleAnalysisManager &AM) {
  Function *printf_func = module.getFunction("printf");
  if (!printf_func) {
    return PreservedAnalyses::all();
  }

  auto &BI = AM.getResult<compiler::utils::BuiltinInfoAnalysis>(module);
  const auto &DI = AM.getResult<compiler::utils::DeviceInfoAnalysis>(module);
  // Set up the double support for this run of the pass
  double_support = DI.double_capabilities != 0;

  Function *get_group_id =
      BI.getOrDeclareMuxBuiltin(compiler::utils::eMuxBuiltinGetGroupId, module);
  assert(get_group_id && "Could not get or insert __mux_get_group_id");
  Function *get_num_groups = BI.getOrDeclareMuxBuiltin(
      compiler::utils::eMuxBuiltinGetNumGroups, module);
  assert(get_num_groups && "Could not get or insert __mux_get_num_groups");

  SmallVector<CallInst *, 32> callsToErase;

  // Clone functions and add extra argument for printf(). Only functions
  // directly or indirectly calling printf are given the extra parameter.
  auto new_param_type =
      PointerType::get(getBufferEltTy(module.getContext()), 1);
  auto param_type_func = [new_param_type](Module &) {
    return compiler::utils::ParamTypeAttrsPair{new_param_type, AttributeSet{}};
  };
  // Set of all functions that directly or indirectly call printf
  SmallPtrSet<const Function *, 16> funcs_calling_printf;
  findAndRecurseFunctionUsers(printf_func, funcs_calling_printf);

  auto to_be_cloned_func = [&funcs_calling_printf](const Function &func,
                                                   bool &ClonedWithBody,
                                                   bool &ClonedNoBody) {
    ClonedWithBody = !func.getName().starts_with("__llvm") &&
                     funcs_calling_printf.count(&func);
    ClonedNoBody = false;
  };

  auto update_md_func = [&module](Function &oldFn, Function &newFn, unsigned) {
    if (auto *namedMetaData = module.getNamedMetadata("opencl.kernels")) {
      for (auto *md : namedMetaData->operands()) {
        if (md && (md->getOperand(0) == llvm::ValueAsMetadata::get(&oldFn))) {
          md->replaceOperandWith(0, llvm::ValueAsMetadata::get(&newFn));
        }
      }
    }
  };
  compiler::utils::cloneFunctionsAddArg(module, param_type_func,
                                        to_be_cloned_func, update_md_func);

  // rewrite printf() calls
  PrintfDescriptorVecTy printf_calls;

  for (auto *user : printf_func->users()) {
    if (auto *ci = dyn_cast<CallInst>(user)) {
      // rewrite the printf calls
      rewritePrintfCall(module, ci, printf_func, get_group_id, get_num_groups,
                        printf_calls);
      callsToErase.push_back(ci);
    }
  }

  // remove all the old instructions as they have been replaced
  for (CallInst *ci : callsToErase) {
    // then destroy the call
    ci->eraseFromParent();
  }

  // destroy the printf function
  printf_func->dropAllReferences();
  printf_func->eraseFromParent();

  // If the user wants the printf calls returned, append to the vector they've
  // provided us.
  if (printf_calls_out_ptr) {
    printf_calls_out_ptr->insert(printf_calls_out_ptr->end(),
                                 std::make_move_iterator(printf_calls.begin()),
                                 std::make_move_iterator(printf_calls.end()));
  }

  return PreservedAnalyses::none();
}
