// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include "compiler/loader.h"

#include "compiler/context.h"

#if defined(CA_RUNTIME_COMPILER_ENABLED)
#if defined(CA_COMPILER_ENABLE_DYNAMIC_LOADER)
#if defined(_WIN32)
#include <Windows.h>
#define DEFAULT_LIBRARY_NAME "compiler.dll"
#else
#include <dlfcn.h>
#define DEFAULT_LIBRARY_NAME "libcompiler.so"
#endif
#else
#include "compiler/library.h"
#endif
#endif

namespace compiler {
struct Library final {
  ~Library() {
#if defined(CA_RUNTIME_COMPILER_ENABLED) && \
    defined(CA_COMPILER_ENABLE_DYNAMIC_LOADER)
    if (library) {
#if defined(_WIN32)
      FreeLibrary(library);
#else
      dlclose(library);
#endif
    }
#endif
  }

#if defined(CA_RUNTIME_COMPILER_ENABLED) && \
    defined(CA_COMPILER_ENABLE_DYNAMIC_LOADER)
  template <typename T>
  bool loadFunction(T &function_ptr, const char *symbol) {
#if defined(_WIN32)
    function_ptr = reinterpret_cast<T>(GetProcAddress(library, symbol));
#else
    function_ptr = reinterpret_cast<T>(dlsym(library, symbol));
#endif
    return function_ptr != nullptr;
  }

#if defined(_WIN32)
  HMODULE library;
#else
  void *library;
#endif
#endif

  const char *(*llvmVersion)();
  void (*compilers)(cargo::array_view<const compiler::Info *> *out_compilers);
  const compiler::Info *(*getCompilerForDevice)(mux_device_info_t device_info);
  Context *(*createContext)();
};

cargo::expected<std::unique_ptr<Library>, std::string> loadLibrary() {
#if defined(CA_RUNTIME_COMPILER_ENABLED)
  std::unique_ptr<Library> handle{new Library};
#if defined(CA_COMPILER_ENABLE_DYNAMIC_LOADER)
  const char *library_name = std::getenv("CA_COMPILER_PATH");
  library_name = library_name ? library_name : DEFAULT_LIBRARY_NAME;
  if (strlen(library_name) == 0) {
    // If the user has assigned `CA_COMPILER_PATH` to the empty string, we
    // should skip loading the compiler.
    return nullptr;
  }

#if defined(_WIN32)
  handle->library = LoadLibraryA(library_name);
#else
  // Passing RTLD_GLOBAL is required to work around an issue with libstdc++
  // where using std::thread in a library loaded with dlopen() causes segfaults.
  // See
  // https://stackoverflow.com/questions/51209268/using-stdthread-in-a-library-loaded-with-dlopen-leads-to-a-sigsev
  handle->library = dlopen(library_name, RTLD_NOW | RTLD_GLOBAL);
#endif

  if (!handle->library) {
    return nullptr;
  }

  if (!handle->loadFunction(handle->llvmVersion, "caCompilerLLVMVersion")) {
    return cargo::make_unexpected(
        "could not find 'caCompilerLLVMVersion' in '" +
        std::string{library_name} + "'");
  }
  if (!handle->loadFunction(handle->compilers, "caCompilers")) {
    return cargo::make_unexpected("could not find 'caCompilers' in '" +
                                  std::string{library_name} + "'");
  }
  if (!handle->loadFunction(handle->getCompilerForDevice,
                            "caGetCompilerForDevice")) {
    return cargo::make_unexpected(
        "could not find 'caGetCompilerForDevice' in '" +
        std::string{library_name} + "'");
  }
  if (!handle->loadFunction(handle->createContext, "caCompilerCreateContext")) {
    return cargo::make_unexpected(
        "could not find 'caCompilerCreateContext' in '" +
        std::string{library_name} + "'");
  }
#else
  handle->llvmVersion = &compiler::llvmVersion;
  handle->compilers =
      [](cargo::array_view<const compiler::Info *> *out_compilers) {
        *out_compilers = compiler::compilers();
      };
  handle->getCompilerForDevice = &compiler::getCompilerForDevice;
  handle->createContext = []() -> Context * {
    return compiler::createContext().release();
  };
#endif
  return {std::move(handle)};
#else
  return nullptr;
#endif
}

const char *llvmVersion(Library *handle) {
  return handle ? handle->llvmVersion() : nullptr;
}

cargo::array_view<const compiler::Info *> compilers(Library *handle) {
  cargo::array_view<const compiler::Info *> compilers_list;
  if (handle) {
    handle->compilers(&compilers_list);
  }
  return compilers_list;
}

const compiler::Info *getCompilerForDevice(Library *handle,
                                           mux_device_info_t device_info) {
  return handle ? handle->getCompilerForDevice(device_info) : nullptr;
}

std::unique_ptr<Context> createContext(Library *handle) {
  return std::unique_ptr<Context>{handle ? handle->createContext() : nullptr};
}
}  // namespace compiler

void std::default_delete<compiler::Library>::operator()(
    compiler::Library *library) const {
  delete library;
}
