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

// Note - this is essentially a copy of LLVM's
// lib/ExecutionEngine/GDBRegistrationListener.cpp but with the static
// singleton and internal locking removed, as this model isn't safe in a
// library context (the static singleton may be destroyed before we are).
//
// In our version, the external users of GDBJITRegistrationListener must ensure
// the accesses are correctly locked as there may be multiple
// GDBJITRegistrationListeners alive at any one time.

#include <compiler/utils/gdb_registration_listener.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/MemoryBuffer.h>

using namespace llvm;
using namespace llvm::object;

// This must be kept in sync with gdb/gdb/jit.h .
extern "C" {

typedef enum {
  JIT_NOACTION = 0,
  JIT_REGISTER_FN,
  JIT_UNREGISTER_FN
} jit_actions_t;

struct jit_code_entry {
  struct jit_code_entry *next_entry;
  struct jit_code_entry *prev_entry;
  const char *symfile_addr;
  uint64_t symfile_size;
};

struct jit_descriptor {
  uint32_t version;
  // This should be jit_actions_t, but we want to be specific about the
  // bit-width.
  uint32_t action_flag;
  struct jit_code_entry *relevant_entry;
  struct jit_code_entry *first_entry;
};

// We put information about the JITed function in this global, which the
// debugger reads.  Make sure to specify the version statically, because the
// debugger checks the version before we can set it during runtime.
extern struct jit_descriptor __jit_debug_descriptor;

// Debuggers puts a breakpoint in this function.
extern "C" void __jit_debug_register_code();
}

namespace {

// FIXME: lli aims to provide both, RuntimeDyld and JITLink, as the dynamic
// loaders for it's JIT implementations. And they both offer debugging via the
// GDB JIT interface, which builds on the two well-known symbol names below.
// As these symbols must be unique across the linked executable, we can only
// define them in one of the libraries and make the other depend on it.
// OrcTargetProcess is a minimal stub for embedding a JIT client in remote
// executors. For the moment it seems reasonable to have the definition there
// and let ExecutionEngine depend on it, until we find a better solution.
//
LLVM_ATTRIBUTE_USED void requiredSymbolDefinitionsFromOrcTargetProcess() {
  errs() << (void *)&__jit_debug_register_code
         << (void *)&__jit_debug_descriptor;
}

struct RegisteredObjectInfo {
  RegisteredObjectInfo() = default;

  RegisteredObjectInfo(std::size_t Size, jit_code_entry *Entry,
                       OwningBinary<ObjectFile> Obj)
      : Size(Size), Entry(Entry), Obj(std::move(Obj)) {}

  std::size_t Size;
  jit_code_entry *Entry;
  OwningBinary<ObjectFile> Obj;
};

// Buffer for an in-memory object file in executable memory
typedef llvm::DenseMap<JITEventListener::ObjectKey, RegisteredObjectInfo>
    RegisteredObjectBufferMap;

/// Global access point for the JIT debugging interface. Must be locked when
/// calling notifyObjectLoaded or notifyFreeingObject as both methods
/// access/modify global variables.
class GDBJITRegistrationListener : public JITEventListener {
 public:
  /// A map of in-memory object files that have been registered with the
  /// JIT interface.
  RegisteredObjectBufferMap ObjectBufferMap;

  /// Instantiates the JIT service.
  GDBJITRegistrationListener() = default;

  /// Asserts that all resources have already been freed.
  ~GDBJITRegistrationListener() override;

  /// Creates an entry in the JIT registry for the buffer @p Object,
  /// which must contain an object file in executable memory with any
  /// debug information for the debugger.
  void notifyObjectLoaded(ObjectKey K, const ObjectFile &Obj,
                          const RuntimeDyld::LoadedObjectInfo &L) override;

  /// Removes the internal registration of @p Object, and
  /// frees associated resources.
  void notifyFreeingObject(ObjectKey K) override;

 private:
  /// Deregister the debug info for the given object file from the debugger
  /// and delete any temporary copies.  This private method does not remove
  /// the function from Map so that it can be called while iterating over Map.
  void deregisterObjectInternal(RegisteredObjectBufferMap::iterator I);
};

/// Do the registration.
void NotifyDebugger(jit_code_entry *JITCodeEntry) {
  __jit_debug_descriptor.action_flag = JIT_REGISTER_FN;

  // Insert this entry at the head of the list.
  JITCodeEntry->prev_entry = nullptr;
  jit_code_entry *NextEntry = __jit_debug_descriptor.first_entry;
  JITCodeEntry->next_entry = NextEntry;
  if (NextEntry) {
    NextEntry->prev_entry = JITCodeEntry;
  }
  __jit_debug_descriptor.first_entry = JITCodeEntry;
  __jit_debug_descriptor.relevant_entry = JITCodeEntry;
  __jit_debug_register_code();
}

GDBJITRegistrationListener::~GDBJITRegistrationListener() {
  // It is the callers' responsibility to ensure all JIT resources have been
  // manually freed up.
  assert(ObjectBufferMap.empty() && "Not all JIT resources have been cleared!");
}

void GDBJITRegistrationListener::notifyObjectLoaded(
    ObjectKey K, const ObjectFile &Obj,
    const RuntimeDyld::LoadedObjectInfo &L) {
  OwningBinary<ObjectFile> DebugObj = L.getObjectForDebug(Obj);

  // Bail out if debug objects aren't supported.
  if (!DebugObj.getBinary()) {
    return;
  }

  const char *Buffer =
      DebugObj.getBinary()->getMemoryBufferRef().getBufferStart();
  const size_t Size =
      DebugObj.getBinary()->getMemoryBufferRef().getBufferSize();

  assert(ObjectBufferMap.find(K) == ObjectBufferMap.end() &&
         "Second attempt to perform debug registration.");
  jit_code_entry *JITCodeEntry = new jit_code_entry();

  if (!JITCodeEntry) {
    llvm::report_fatal_error(
        "Allocation failed when registering a JIT entry!\n");
  } else {
    JITCodeEntry->symfile_addr = Buffer;
    JITCodeEntry->symfile_size = Size;

    ObjectBufferMap[K] =
        RegisteredObjectInfo(Size, JITCodeEntry, std::move(DebugObj));
    NotifyDebugger(JITCodeEntry);
  }
}

void GDBJITRegistrationListener::notifyFreeingObject(ObjectKey K) {
  const RegisteredObjectBufferMap::iterator I = ObjectBufferMap.find(K);

  if (I != ObjectBufferMap.end()) {
    deregisterObjectInternal(I);
    ObjectBufferMap.erase(I);
  }
}

void GDBJITRegistrationListener::deregisterObjectInternal(
    RegisteredObjectBufferMap::iterator I) {
  jit_code_entry *&JITCodeEntry = I->second.Entry;

  // Do the unregistration.
  {
    __jit_debug_descriptor.action_flag = JIT_UNREGISTER_FN;

    // Remove the jit_code_entry from the linked list.
    jit_code_entry *PrevEntry = JITCodeEntry->prev_entry;
    jit_code_entry *NextEntry = JITCodeEntry->next_entry;

    if (NextEntry) {
      NextEntry->prev_entry = PrevEntry;
    }
    if (PrevEntry) {
      PrevEntry->next_entry = NextEntry;
    } else {
      assert(__jit_debug_descriptor.first_entry == JITCodeEntry);
      __jit_debug_descriptor.first_entry = NextEntry;
    }

    // Tell the debugger which entry we removed, and unregister the code.
    __jit_debug_descriptor.relevant_entry = JITCodeEntry;
    __jit_debug_register_code();
  }

  delete JITCodeEntry;
  JITCodeEntry = nullptr;
}

}  // end namespace

namespace compiler {
namespace utils {

std::unique_ptr<llvm::JITEventListener> createGDBRegistrationListener() {
  return std::make_unique<GDBJITRegistrationListener>();
}

}  // namespace utils
}  // namespace compiler
