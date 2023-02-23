// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// This file includes config.h after defining a macro which enables the
// instantiation of the static array of create device hooks so that only one
// exists across all translation units.
#define MUX_CONFIG_INSTANTIATE_IMPLEMENTATION
#include <mux/config.h>
