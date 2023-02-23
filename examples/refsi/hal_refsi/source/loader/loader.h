// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _HAL_REFSI_LOADER_LOADER_H
#define _HAL_REFSI_LOADER_LOADER_H

#include <stdint.h>
#include "device/device_if.h"

// Retrieve a pointer to the current hart's execution context.
exec_state_t *get_current_context();

#endif
