// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "kts/arguments_shared.h"

/// @brief Default global size for 1D kernels.
const size_t kts::N = 256;

/// @brief Default local size for 1D kernels that make use of work-groups.
size_t kts::localN = 16;

kts::BufferDesc::BufferDesc() : size_(0), streamer_(nullptr), streamer2_(nullptr) {}

kts::BufferDesc::BufferDesc(size_t size,
                            std::shared_ptr<BufferStreamer> streamer)
    : size_(size), streamer_(streamer) {}

kts::BufferDesc::BufferDesc(size_t size,
                            std::shared_ptr<BufferStreamer> streamer,
                            std::shared_ptr<BufferStreamer> streamer2)
    : size_(size), streamer_(streamer), streamer2_(streamer2) {}
