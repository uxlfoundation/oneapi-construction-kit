// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef ICD_H_INCLUDED
#define ICD_H_INCLUDED

#include <cstdint>

template <typename T>
struct icd_t {
  uintptr_t magic = 0x01CDC0DE;
};

#endif
