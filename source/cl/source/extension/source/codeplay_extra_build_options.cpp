// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <extension/codeplay_extra_build_options.h>

extension::codeplay_extra_build_options::codeplay_extra_build_options()
    : extension("cl_codeplay_extra_build_options",
#ifdef OCL_EXTENSION_cl_codeplay_extra_build_options
                usage_category::PLATFORM
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(0, 6, 0)) {
}
