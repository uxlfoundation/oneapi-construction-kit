// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <extension/codeplay_soft_math.h>

extension::codeplay_soft_math::codeplay_soft_math()
    : extension("cl_codeplay_soft_math",
#ifdef OCL_EXTENSION_cl_codeplay_soft_math
                usage_category::PLATFORM
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(0, 1, 0)) {
}
