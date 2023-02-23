// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#if defined(__linux__)
#include <host/perf_support.h>
#include <llvm/ExecutionEngine/ObjectCache.h>
#include <llvm/IR/Module.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Object/SymbolSize.h>
#include <llvm/Support/FileSystem.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <utility>
#include <vector>

#define DEFAULT_FILE_NAME_TEMPLATE "/tmp/perf-"
#define PERF_ENV_VARIABLE_ENABLE "CA_ENABLE_PERF_INTERFACE"

namespace host {
using objType = llvm::object::ObjectFile;
using symListType = std::vector<std::pair<llvm::object::SymbolRef, uint64_t>>;
using symListIterType =
    std::vector<std::pair<llvm::object::SymbolRef, uint64_t>>::iterator;
using symRef = llvm::object::SymbolRef;

PerfInterface::PerfInterface(const std::string &obj_name) {
  const char *env = std::getenv(PERF_ENV_VARIABLE_ENABLE);
  if (env && env[0] == '1') {
    enable = true;
  } else {
    enable = false;
    return;
  }

  std::stringstream pid_stream;
  pid_stream << DEFAULT_FILE_NAME_TEMPLATE << getpid() << "_" << obj_name
             << ".o";
  filename = pid_stream.str();

  // On linux, open up a file to hold machine-code. We could have
  // multiple kernels being compiled during a sampling period and we
  // want all of them stored for profiling.
  std::error_code err;
  obj_file = std::unique_ptr<llvm::raw_fd_ostream>(new llvm::raw_fd_ostream(
      filename.c_str(), err, llvm::sys::fs::OpenFlags::OF_Text));

  // On linux, perf looks for files in /tmp with the file format
  // /tmp/perf-${pid}.map. Since we are JIT'ing the kernel and
  // executing immediately in the context of the compiler, we
  // can use our own $PID value
  std::stringstream perf_map_file;
  perf_map_file << DEFAULT_FILE_NAME_TEMPLATE << getpid() << ".map";

  if (enable) {
    // We could have multiple invocations on different kernels
    // for each perf sampling session. Hence open map file in
    // append mode to capture all of them together.
    perf_fstream.open(perf_map_file.str(), std::ofstream::app);
  }
}

PerfInterface::~PerfInterface() {
  if (perf_fstream.is_open()) {
    perf_fstream.close();
  }
}

std::unique_ptr<llvm::MemoryBufferRef> PerfInterface::getObjectFromCache(
    const std::string &key) {
  // Ensure that multiple invocations of MCJIT
  // with same keys won't race with each other
  std::lock_guard<std::mutex> guard(this->lock);
  auto itr = mem_cache.find(key);
  if (itr == mem_cache.end()) {
    // Object not to be found in cache-map
    return nullptr;
  } else {
    // return unique copy of memory
    std::unique_ptr<llvm::MemoryBufferRef> co(new llvm::MemoryBufferRef(
        (itr->second).getBuffer(), (itr->second).getBufferIdentifier()));
    return co;
  }
}

std::unique_ptr<llvm::MemoryBuffer> PerfInterface::getObjectBuffer(
    const std::string &key) {
  std::lock_guard<std::mutex> guard(this->lock);
  auto itr = mem_cache.find(key);
  if (itr == mem_cache.end()) {
    // Object not to be found in cache-map
    return nullptr;
  } else {
    return llvm::MemoryBuffer::getMemBuffer(itr->second, false);
  }
}

// MCJIT will call into this preprocessor prior to compilation.
// Anything other than a NULL ptr will be treated as valid object
// code and will not get compiled
std::unique_ptr<llvm::MemoryBuffer> PerfInterface::getObject(
    const llvm::Module *module) {
  if (module == nullptr) {
    return nullptr;
  }
  const std::string mod_id = module->getModuleIdentifier();
  return getObjectBuffer(mod_id);
}

// Function to write symbols that have just been JIT'ed
// into a perf "map" file which perf can look-up to match
// symbols to which an executed instruction matches against
void PerfInterface::writePerfSymbolFile(const std::string &module_key,
                                        std::string symbol, uint64_t address) {
  if (!is_enabled()) {
    return;
  }

  std::unique_ptr<llvm::MemoryBufferRef> obj = getObjectFromCache(module_key);
  if (!obj) {
    return;
  }

  uint64_t symbol_size = 0;
  llvm::Expected<std::unique_ptr<objType>> compiled_obj =
      objType::createObjectFile(*obj);

  if (!compiled_obj) {
    return;
  }

  symListType sym_list = llvm::object::computeSymbolSizes(**compiled_obj);
  symListIterType symbol_func = std::find_if(
      sym_list.begin(), sym_list.end(),
      [&](std::pair<llvm::object::SymbolRef, uint64_t> &item) -> bool {
        llvm::object::SymbolRef sym = item.first;
        llvm::Expected<llvm::object::SymbolRef::Type> sym_type = sym.getType();
        if (auto err = sym_type.takeError()) {
          return false;
        }

        if (*(sym_type) != llvm::object::SymbolRef::ST_Function) {
          return false;
        }

        // Ensure the symbol matches the symbol we are interested in
        llvm::Expected<llvm::StringRef> name_or_err = sym.getName();
        if (!name_or_err) {
          return false;
        }

        std::string sym_name(*name_or_err);
        if (sym_name != symbol) {
          return false;
        }

        // Obtain size of computed symbol
        symbol_size = item.second;
        return true;
      });

  // Check that while iterating through the object,
  // we actually did come across the symbol
  if (symbol_func == sym_list.end()) {
    return;
  }

  // Perf map files are a space de-limited file with the format
  // <address> <size-of-symbol> <name-of-symbol>
  std::stringstream perf_dso_rec;
  perf_dso_rec << std::hex << address << " " << symbol_size << " " << symbol
               << "\n";

  if (perf_fstream.is_open()) {
    perf_fstream << perf_dso_rec.str();
  }
}

// Function is called as a post-processor by MCJIT
// if MCJIT has compiled the module
void PerfInterface::notifyObjectCompiled(const llvm::Module *module,
                                         const llvm::MemoryBufferRef obj) {
  if (module == nullptr) {
    return;
  }

  // Ensure protection from race conditions
  std::lock_guard<std::mutex> guard(this->lock);

  // Write compiled object code into cache-file
  if (obj_file) {
    (*obj_file) << obj.getBuffer();
  }

  // If the object memory is found in cache, replace old code with new code
  // into correct location. Othewise, add code to map
  std::string moduleID(module->getModuleIdentifier());
  llvm::MemoryBufferRef obj_code(obj.getBuffer(), obj.getBufferIdentifier());
  auto itr = mem_cache.find(moduleID);
  if (itr == mem_cache.end()) {
    mem_cache.insert(
        std::pair<std::string, llvm::MemoryBufferRef>(moduleID, obj_code));
  } else {
    mem_cache[moduleID] = obj_code;
  }
}
}  // namespace host

#endif  //  defined(__linux__)
