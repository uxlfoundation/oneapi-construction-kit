// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_cast.h>

#include <abacus/internal/convert_helper.h>

#ifdef __CA_BUILTINS_HALF_SUPPORT
DEF(half);
#endif  // __CA_BUILTINS_HALF_SUPPORT
DEF(float);
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF(double);
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
