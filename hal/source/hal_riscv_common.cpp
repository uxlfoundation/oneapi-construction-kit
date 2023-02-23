// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <assert.h>
#include <hal.h>
#include <hal_riscv_common.h>
#include <hal_types.h>

#include <string>

namespace riscv {
/// @addtogroup riscv
/// @{

/// @brief Helper function to deduce RISCV device info from a risc-v extension
/// string.
///
/// @param str is a null terminated riscv extension string, i.e. ("RV32GVC")
/// @param info the generic device info structure to fill out.
/// @param riscv_info the riscv specific device info structure to fill out.
///
/// @return true on success, false otherwise.
///
/// @note that this does not yet support:
/// - version numbers ("RV64I1p0M1p0A1p0F1p0D1p0") (27.4)
/// - Ignores I
bool update_info_from_riscv_isa_description(
    const char *str, ::hal::hal_device_info_t &info,
    hal_device_info_riscv_t &riscv_info) {
  riscv_info.extensions = 0;
  uint32_t word_size = 0;
  // e.g RV32IMAFD, RV32G
  // skip RV if there
  // read 32 or 64
  if (*str == 'R') str++;
  if (*str == 'V') str++;
  if (isdigit(*str)) {
    word_size = *str - '0';
    str++;
    if (isdigit(*str)) {
      word_size = 10 * word_size + (*str - '0');
      str++;
    }
  }
  if (word_size) {
    info.word_size = word_size;
  }

  while (char c = *str++) {
    switch (c) {
      case 'M':
        riscv_info.extensions |= rv_extension_M;
        break;
      case 'A':
        riscv_info.extensions |= rv_extension_A;
        break;
      case 'D':
        riscv_info.extensions |= rv_extension_D;
        // F is implied
        riscv_info.extensions |= rv_extension_F;
        break;
      case 'F':
        riscv_info.extensions |= rv_extension_F;
        break;
      case 'G':
        riscv_info.extensions |= rv_extension_G;
        break;
      case 'V':
        riscv_info.extensions |= rv_extension_V;
        break;
      case 'Q':
        riscv_info.extensions |= rv_extension_Q;
        // D and F are implied
        riscv_info.extensions |= rv_extension_D;
        riscv_info.extensions |= rv_extension_F;
        break;
      case 'L':
        riscv_info.extensions |= rv_extension_L;
        break;
      case 'C':
        riscv_info.extensions |= rv_extension_C;
        break;
      case 'B':
        riscv_info.extensions |= rv_extension_B;
        break;
      case 'J':
        riscv_info.extensions |= rv_extension_J;
        break;
      case 'T':
        riscv_info.extensions |= rv_extension_T;
        break;
      case 'P':
        riscv_info.extensions |= rv_extension_P;
        break;
      case 'N':
        riscv_info.extensions |= rv_extension_N;
        break;
      case 'H':
        riscv_info.extensions |= rv_extension_H;
        break;
      case 'E':
        riscv_info.extensions |= rv_extension_E;
        break;
      case 'Z': {
        // Start parsing a multi-letter extension.
        auto *start = str;
        while (char zc = *str) {
          // Z extensions should be followed by an underscore, otherwise we'll
          // eat until the end of the string.
          if (zc == '_') {
            break;
          }
          str++;
        }
        auto ext = std::string(start, str);
        if (ext == "ba") {
          riscv_info.extensions |= rv_extension_Zba;
        } else if (ext == "bb") {
          riscv_info.extensions |= rv_extension_Zbb;
        } else if (ext == "bc") {
          riscv_info.extensions |= rv_extension_Zbc;
        } else if (ext == "bs") {
          riscv_info.extensions |= rv_extension_Zbs;
        } else if (ext == "fh") {
          riscv_info.extensions |= rv_extension_Zfh;
        } else if (ext == "bkb") {
          riscv_info.extensions |= rv_extension_Zbkb;
        } else if (ext == "bkc") {
          riscv_info.extensions |= rv_extension_Zbkc;
        } else if (ext == "bkx") {
          riscv_info.extensions |= rv_extension_Zbkx;
        } else if (ext == "knd") {
          riscv_info.extensions |= rv_extension_Zknd;
        } else if (ext == "kne") {
          riscv_info.extensions |= rv_extension_Zkne;
        } else if (ext == "knh") {
          riscv_info.extensions |= rv_extension_Zknh;
        } else if (ext == "ksed") {
          riscv_info.extensions |= rv_extension_Zksed;
        } else if (ext == "ksh") {
          riscv_info.extensions |= rv_extension_Zksh;
        } else if (ext == "kr") {
          riscv_info.extensions |= rv_extension_Zkr;
        } else if (ext == "kt") {
          riscv_info.extensions |= rv_extension_Zkt;
        } else if (ext == "kn") {
          riscv_info.extensions |= rv_extension_Zkn;
        } else if (ext == "ks") {
          riscv_info.extensions |= rv_extension_Zks;
        } else if (ext == "k") {
          riscv_info.extensions |= rv_extension_Zk;
        } else {
          // Unknown standard extension.
          return false;
        }
        break;
      }
      case '_':
      case 'I':
        break;
      default:
        return false;
    }
  }
  return true;
}

/// @}
}  // namespace riscv
