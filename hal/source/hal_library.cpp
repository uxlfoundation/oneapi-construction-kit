// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#if defined(_WIN32)
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

#include <cinttypes>
#include <cstdlib>
#include <cstring>

#include "hal.h"
#include "hal_library.h"

namespace hal {
/// @addtogroup hal
/// @{

using create_hal_fn = hal_t *(*)(uint32_t &);

// Return the file name of the HAL library for the given device.
std::string get_hal_library_path(const char *device_name) {
#if defined(_WIN32)
  return std::string("hal_") + device_name + std::string(".dll");
#else
  return std::string("libhal_") + device_name + std::string(".so");
#endif
}

// Try to load a HAL given a path to a library file. The path can be relative.
// Return a pointer to a HAL object and a library handle on success.
hal_t *load_hal_library(const char *library_path, uint32_t expected_api_version,
                        hal_library_t &handle) {
#if defined(_WIN32)
  const HMODULE hmodule = LoadLibraryA(library_path);
  if (!hmodule) {
    fprintf(stderr, "error: could not load '%s'\n", library_path);
    return nullptr;
  }
  create_hal_fn hal_entry_fn =
      (create_hal_fn)GetProcAddress(hmodule, "get_hal");
  if (!hal_entry_fn) {
    fprintf(stderr, "error: could not find the HAL entry point function\n");
    FreeLibrary(hmodule);
    return nullptr;
  }
  uint32_t reported_api_version = 0;
  hal_t *hal = hal_entry_fn(reported_api_version);
  if (!hal) {
    fprintf(stderr, "error: creating the HAL failed\n");
    FreeLibrary(hmodule);
    return nullptr;
  }
  if (expected_api_version != 0 &&
      reported_api_version != expected_api_version) {
    fprintf(stderr,
            "error: expected HAL API version %" PRIu32 ", but %" PRIu32
            " is the version reported by the loaded HAL\n",
            expected_api_version, reported_api_version);
    FreeLibrary(hmodule);
    return nullptr;
  }
  // an HMODULE is just a pointer type also
  handle = static_cast<hal_library_t>(hmodule);
  return hal;
#else
  // Passing RTLD_GLOBAL is required to work around an issue with libstdc++
  // where using std::thread in a library loaded with dlopen() causes segfaults.
  // See
  // https://stackoverflow.com/questions/51209268/using-stdthread-in-a-library-loaded-with-dlopen-leads-to-a-sigsev
  handle = dlopen(library_path, RTLD_LAZY | RTLD_GLOBAL);
  if (!handle) {
    fprintf(stderr, "error: could not load '%s'\n", library_path);
    return nullptr;
  }
  create_hal_fn hal_entry_fn = (create_hal_fn)dlsym(handle, "get_hal");
  if (!hal_entry_fn) {
    fprintf(stderr, "error: could not find the HAL entry point function\n");
    dlclose(handle);
    return nullptr;
  }
  uint32_t reported_api_version = 0;
  hal_t *hal = hal_entry_fn(reported_api_version);
  if (!hal) {
    fprintf(stderr, "error: creating the HAL failed\n");
    dlclose(handle);
    return nullptr;
  }
  if (expected_api_version != 0 &&
      reported_api_version != expected_api_version) {
    fprintf(stderr,
            "error: expected HAL API version %" PRIu32 ", but %" PRIu32
            " is the version reported by the loaded HAL\n",
            expected_api_version, reported_api_version);
    dlclose(handle);
    return nullptr;
  }
  return hal;
#endif
}

// Try to load a HAL based on the environment and the default device name.
// Return a pointer to a HAL object and a library handle on success.
hal_t *load_hal(const char *default_device, uint32_t expected_api_version,
                hal_library_t &handle) {
  std::string library_path;
  const char *env_hal_device = std::getenv("CA_HAL_DEVICE");
  // First allow the device HAL to be specified through an environment variable.
  if (env_hal_device) {
#if !CA_HAL_LOCK_DEVICE_NAME
    if (env_hal_device[0] == '/') {
      // Load the HAL using an absolute path.
      library_path = std::string(env_hal_device);
    } else {
      // Load the HAL using a device name.
      library_path = get_hal_library_path(env_hal_device);
    }
    if (hal_t *hal = load_hal_library(library_path.c_str(),
                                      expected_api_version, handle)) {
      return hal;
    }
#else
    fprintf(stderr,
            "warning: selecting a HAL device through CA_HAL_DEVICE has "
            "been disabled at build time\n");
#endif
  }

  // Try to load the default device when a device could not be loaded through
  // the environment variable.
  if (strlen(default_device) > 0) {
    library_path = get_hal_library_path(default_device);
    if (hal_t *hal = load_hal_library(library_path.c_str(),
                                      expected_api_version, handle)) {
      return hal;
    } else {
      fprintf(stderr, "error: unable to load hal library from '%s'\n",
              library_path.c_str());
    }
  } else {
    fprintf(stderr,
            "error: no default device was specified and couldn't load "
            "CA_HAL_DEVICE\n");
  }
  return nullptr;
}

void unload_hal(hal_library_t handle) {
  if (handle) {
#if defined(_WIN32)
    const HMODULE hmodule = static_cast<HMODULE>(handle);
    FreeLibrary(hmodule);
#else
    dlclose(handle);
#endif
  }
}

/// @}
}  // namespace hal
