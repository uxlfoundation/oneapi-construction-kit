// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cargo/string_algorithm.h>
#include <host/common/jit_kernel.h>
#include <host/device.h>
#include <host/executable.h>
#include <host/host.h>
#include <host/metadata_hooks.h>
#include <loader/relocations.h>
#include <mux/utils/allocator.h>
#include <utils/system.h>

#if !defined(NDEBUG) && !defined(_MSC_VER)
// 'nix debug builds do extra file checks
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <cassert>
#include <cmath>
#include <memory>
#include <new>
#ifndef NDEBUG
#include <utility>
#endif

// Kernels can call library functions. The platform determines which library
// function calls LLVM might emit. The ELF resolver needs the addresses of these
// functions.
extern "C" {

#if defined(_MSC_VER)
// Windows uses chkstk() to ensure there is enough stack space paged in.
#if defined(UTILS_SYSTEM_64_BIT)
extern void __chkstk();
#else
extern void _chkstk();
#endif
#endif  // _MSC_VER

#if defined(UTILS_SYSTEM_32_BIT)
// On 32-bit (both x86 and Arm) long division is done in software.
extern void __divdi3();
extern void __udivdi3();
extern void __moddi3();
extern void __umoddi3();
#endif

#if defined(UTILS_SYSTEM_ARM) && defined(UTILS_SYSTEM_32_BIT)
// Arm builds use these functions to convert between floats and longs
extern void __fixdfdi();
extern void __floatdidf();
extern void __floatdisf();
#endif
}

namespace {

#ifndef NDEBUG
void *dbg_memcpy(void *dest, const void *src, size_t count) {
  unsigned char *d = reinterpret_cast<unsigned char *>(dest);
  const unsigned char *s = reinterpret_cast<const unsigned char *>(src);

#if !defined(_MSC_VER)
  // On 'nix, check that the source is readable and the destination is writeable
  int null_fd = open("/dev/null", O_WRONLY);
  int zero_fd = open("/dev/zero", O_RDONLY);

  // Only read src if opened "/dev/null" successfully
  if (null_fd >= 0) {
    size_t write_count = static_cast<size_t>(write(null_fd, src, count));
    close(null_fd);
    if (write_count != count) {
      fprintf(stderr, "memcpy (called from kernel) out-of-bounds read\n");
      std::abort();
    }
  }

  // Only write zeros if opening "/dev/zero" was successful
  if (zero_fd >= 0) {
    size_t read_count = static_cast<size_t>(read(zero_fd, dest, count));
    close(zero_fd);
    if (read_count != count) {
      fprintf(stderr, "memcpy (called from kernel) out-of-bounds write\n");
      std::abort();
    }
  }
#endif  // !_MSC_VER

  for (size_t i = 0; i < count; ++i) {
    d[i] = s[i];
  }
  return dest;
}

void *dbg_memset(void *dest, int ch, size_t count) {
  unsigned char c = static_cast<unsigned char>(ch);
  unsigned char *d = reinterpret_cast<unsigned char *>(dest);

#if !defined(_MSC_VER)
  // On 'nix, check that the destination is writeable
  int zero_fd = open("/dev/zero", O_RDONLY);

  // Only write zeros if opening "/dev/zero" was successful
  if (zero_fd >= 0) {
    size_t cnt = static_cast<size_t>(read(zero_fd, dest, count));
    close(zero_fd);
    if (count != cnt) {
      fprintf(stderr, "memset (called from kernel) out-of-bounds write\n");
      std::abort();
    }
  }
#endif  // !_MSC_VER

  for (size_t i = 0; i < count; ++i) {
    d[i] = c;
  }
  return dest;
}
#endif  // NDEBUG

static std::pair<std::string, uint64_t> relocs[] = {
#ifndef NDEBUG
    {"memcpy", reinterpret_cast<uint64_t>(&dbg_memcpy)},
    {"memset", reinterpret_cast<uint64_t>(&dbg_memset)},
#else
    {"memcpy", reinterpret_cast<uint64_t>(&memcpy)},
    {"memset", reinterpret_cast<uint64_t>(&memset)},
#endif  // NDEBUG
    {"memmove", reinterpret_cast<uint64_t>(&memmove)},

#if defined(_MSC_VER)
#if defined(UTILS_SYSTEM_64_BIT)
    {"__chkstk", reinterpret_cast<uint64_t>(&__chkstk)},
#else
    {"_chkstk", reinterpret_cast<uint64_t>(&_chkstk)},
#endif  // UTILS_SYSTEM_64_BIT
#endif  // _MSC_VER

#if defined(UTILS_SYSTEM_32_BIT)
    {"__divdi3", reinterpret_cast<uint64_t>(&__divdi3)},
    {"__udivdi3", reinterpret_cast<uint64_t>(&__udivdi3)},
    {"__moddi3", reinterpret_cast<uint64_t>(&__moddi3)},
    {"__umoddi3", reinterpret_cast<uint64_t>(&__umoddi3)},
#endif  // defined(UTILS_SYSTEM_X86) && defined(UTILS_SYSTEM_32_BIT)

#if defined(UTILS_SYSTEM_ARM) && defined(UTILS_SYSTEM_32_BIT)
    {"__fixdfdi", reinterpret_cast<uint64_t>(&__fixdfdi)},
    {"__floatdidf", reinterpret_cast<uint64_t>(&__floatdidf)},
    {"__floatdisf", reinterpret_cast<uint64_t>(&__floatdisf)},
    // fminf and fmaxf are both used by the Arm32 backend when expanding
    // floating-point min/max reductions.
    {"fminf", reinterpret_cast<uint64_t>(&fminf)},
    {"fmaxf", reinterpret_cast<uint64_t>(&fmaxf)},
#endif  // defined(UTILS_SYSTEM_ARM) && defined(UTILS_SYSTEM_32_BIT)
};
}  // namespace

host::executable_s::executable_s(mux_device_t device, jit_kernel_s kernel,
                                 mux::allocator allocator)
    : jit_kernel_name(kernel.name),
      elf_contents(allocator),
      allocated_pages(allocator) {
  this->device = device;
  kernels.emplace(jit_kernel_name,
                  std::vector<binary_kernel_s>(
                      {{kernel.hook, kernel.name, kernel.local_memory_used,
                        kernel.min_work_width, kernel.pref_work_width,
                        /*sub_group_size*/ 0}}));
}

host::executable_s::executable_s(
    mux_device_t device, mux::dynamic_array<uint64_t> elf_contents,
    mux::small_vector<loader::PageRange, 4> allocated_pages,
    kernel_variant_map binary_kernels)
    : elf_contents(std::move(elf_contents)),
      allocated_pages(std::move(allocated_pages)),
      kernels(std::move(binary_kernels)) {
  this->device = device;
}

mux_result_t hostCreateExecutable(mux_device_t device, const void *binary,
                                  uint64_t binary_length,
                                  mux_allocator_info_t allocator_info,
                                  mux_executable_t *out_executable) {
  mux::allocator allocator(allocator_info);

  // If we're passing through a JIT compiled kernel.
  if (host::isJITKernel(binary, binary_length)) {
    cargo::optional<host::jit_kernel_s> jit_kernel =
        host::deserializeJITKernel(binary, binary_length);
    if (!jit_kernel) {
      return mux_error_invalid_binary;
    }

    auto executable = allocator.create<host::executable_s>(
        device, std::move(*jit_kernel), allocator);
    if (nullptr == executable) {
      return mux_error_out_of_memory;
    }

    *out_executable = executable;
    return mux_success;
  }

  mux::dynamic_array<uint64_t> elf_contents{allocator};
  if (elf_contents.alloc((binary_length / sizeof(uint64_t)) + 1)) {
    return mux_error_out_of_memory;
  }
  cargo::array_view<uint8_t> elf_bytes{
      reinterpret_cast<uint8_t *>(elf_contents.data()),
      static_cast<size_t>(binary_length)};
  std::copy_n(reinterpret_cast<const uint8_t *>(binary),
              static_cast<size_t>(binary_length), elf_bytes.begin());
  host::kernel_variant_map kernels;

  if (!loader::ElfFile::isValidElf(elf_bytes)) {
    return mux_error_invalid_binary;
  }
  std::unique_ptr<loader::ElfFile> elf_file{new loader::ElfFile(elf_bytes)};

  auto parsed_kernels = host::readBinaryMetadata(elf_file.get(), &allocator);
  if (parsed_kernels) {
    kernels = std::move(parsed_kernels.take().value());
  } else {
    return mux_error_invalid_binary;
  }

  mux::small_vector<loader::PageRange, 4> allocated_pages{allocator};
  mux::small_vector<loader::MemoryProtection, 4> page_protection{allocator};
  loader::ElfMap elf_map{elf_file.get()};

  // load
  for (auto &section : elf_file->sections()) {
    if (!(section.flags() & loader::ElfFields::SectionFlags::ALLOC)) {
      continue;
    }
    if (section.name().data() == host::MD_NOTES_SECTION) {
      continue;
    }

    // We map the section whether it has a non-zero size or not, but we only
    // allocate and protect pages if the size is greater than 0.
    if (section.sizeToAlloc() > 0) {
      if (allocated_pages.emplace_back()) {
        return mux_error_out_of_memory;
      }
      if (page_protection.emplace_back()) {
        return mux_error_out_of_memory;
      }
      if (allocated_pages.back().allocate(section.sizeToAlloc())) {
        return mux_error_out_of_memory;
      }
      page_protection.back() = loader::getSectionProtection(section);
      if (section.type() != loader::ElfFields::SectionType::NOBITS) {
        std::copy(section.data().begin(), section.data().end(),
                  allocated_pages.back().data().begin());
      }
      uint8_t *dataptr = allocated_pages.back().data().data();
      if (elf_map.addSectionMapping(section, dataptr,
                                    allocated_pages.back().data().end(),
                                    reinterpret_cast<uint64_t>(dataptr))) {
        return mux_error_out_of_memory;
      }
    } else {
      if (elf_map.addSectionMapping(section, nullptr, nullptr, 0)) {
        return mux_error_out_of_memory;
      }
    }
  }

  // Populate elf_map with all callbacks so the kernel can call those
  // functions
  for (const auto &reloc : relocs) {
    if (elf_map.addCallback(reloc.first, reloc.second)) {
      return mux_error_out_of_memory;
    }
  }

  // relocate
  // If this is failing (especially on Arm32), it may be that a required
  // callback isn't getting added. See the `elf_map.addCallback()`s above.
  // Callbacks are resolved in `loader::ElfMap::getSymbolTargetAddress()`.
  if (!loader::resolveRelocations(*elf_file, elf_map)) {
    return mux_error_internal;
  }

  // protect
  for (size_t i = 0; i < page_protection.size(); i++) {
    allocated_pages[i].protect(page_protection[i]);
  }

  // set hooks
  for (auto &p : kernels) {
    for (auto &variant : p.second) {
      auto hook = elf_map.getSymbolTargetAddress(
          {variant.kernel_name.data(), variant.kernel_name.size()});
      if (!hook) {
        return mux_error_invalid_binary;
      }
      variant.hook = *hook;
    }
  }

  auto executable = allocator.create<host::executable_s>(
      device, std::move(elf_contents), std::move(allocated_pages),
      std::move(kernels));
  if (nullptr == executable) {
    return mux_error_out_of_memory;
  }

  *out_executable = executable;
  return mux_success;
}

void hostDestroyExecutable(mux_device_t device, mux_executable_t executable,
                           mux_allocator_info_t allocator_info) {
  (void)device;
  mux::allocator allocator(allocator_info);
  auto hostExecutable = static_cast<host::executable_s *>(executable);
  allocator.destroy(hostExecutable);
}
