// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <extension/khr_extended_async_copies.h>

extension::khr_extended_async_copies::khr_extended_async_copies()
    : extension("cl_khr_extended_async_copies",
#ifdef OCL_EXTENSION_cl_khr_extended_async_copies
                usage_category::DEVICE
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(1, 0, 0)) {
}
