// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

/// @file
///
/// @brief Header shared between both the host and kernel API's.

#ifndef CODEPLAY_IMG_SHARED_H
#define CODEPLAY_IMG_SHARED_H

#include <image_library_integration.h>

// When being integrated into the OCL runtime these macro definitions must be
// disabled as they are already defined as part of the OCL runtime in order to
// make them visible to the OpenCL C compiler.
#ifndef __CODEPLAY_OCL_IMAGE_SUPPORT

/// @brief Definitions and types shared by the the host and kernel API's.
///
/// @addtogroup libimg
/// @{

/// @brief Image channel order values.
///
/// Integer values which represent the image channel order.
/// See "The Spir Specification Version 1.2" section:
/// "2.1.3.3 Image channel order values."
/// @weakgroup ChannelOrder
/// @{

/// @brief CLK_R
#define CLK_R 0x10B0
/// @brief CLK_A
#define CLK_A 0x10B1
/// @brief CLK_RG
#define CLK_RG 0x10B2
/// @brief CLK_RA
#define CLK_RA 0x10B3
/// @brief CLK_RGB
#define CLK_RGB 0x10B4
/// @brief CLK_RGBA
#define CLK_RGBA 0x10B5
/// @brief CLK_BGRA
#define CLK_BGRA 0x10B6
/// @brief CLK_ARGB
#define CLK_ARGB 0x10B7
/// @brief CLK_INTENSITY
#define CLK_INTENSITY 0x10B8
/// @brief CLK_LUMINANCE
#define CLK_LUMINANCE 0x10B9
/// @brief CLK_Rx
#define CLK_Rx 0x10BA
/// @brief CLK_RGx
#define CLK_RGx 0x10BB
/// @brief CLK_RGBx
#define CLK_RGBx 0x10BC
/// @brief CLK_DEPTH listed but not supported yet.
#define CLK_DEPTH 0x10BD
/// @brief CLK_DEPTH_STENCIL listed but not supported yet.
#define CLK_DEPTH_STENCIL 0x10BE
/// @}

/// @brief Image channel data type values.
///
/// Integer values which represent the image channel data type.
/// See "The Spir Specification Version 1.2" section:
/// "2.1.3.2 Image channel data type values."
/// @weakgroup ChannelDataType
/// @{

/// @brief CLK_SNORM_INT8 signed, normalized 8-bit integer.
#define CLK_SNORM_INT8 0x10D0
/// @brief CLK_SNORM_INT16 signed, normalized 16-bit integer.
#define CLK_SNORM_INT16 0x10D1
/// @brief CLK_UNORM_INT8 unsigned, normalized 8-bit integer.
#define CLK_UNORM_INT8 0x10D2
/// @brief CLK_UNORM_INT16 unsigned, normalized 16-bit integer.
#define CLK_UNORM_INT16 0x10D3
/// @brief CLK_UNORM_SHORT_565 normalized 5-6-5 3 channel RGB data type.
#define CLK_UNORM_SHORT_565 0x10D4
/// @brief CLK_UNORM_SHORT_555 normalized x-5-5-5 4 channel xRGB data type.
#define CLK_UNORM_SHORT_555 0x10D5
/// @brief CLK_UNORM_INT_101010 normalized x-10-10-10 4-channel xRGB data type.
#define CLK_UNORM_INT_101010 0x10D6
/// @brief CLK_SIGNED_INT8 signed, unnormalized 8-bit integer.
#define CLK_SIGNED_INT8 0x10D7
/// @brief CLK_SIGNED_INT16 signed, unnormalized 16-bit integer.
#define CLK_SIGNED_INT16 0x10D8
/// @brief CLK_SIGNED_INT32 signed, unnormalized 32-bit integer.
#define CLK_SIGNED_INT32 0x10D9
/// @brief CLK_UNSIGNED_INT8 unsigned, unnormalized 8-bit integer.
#define CLK_UNSIGNED_INT8 0x10DA
/// @brief CLK_UNSIGNED_INT16 unsigned, unnormalized 16-bit integer.
#define CLK_UNSIGNED_INT16 0x10DB
/// @brief CLK_UNSIGNED_INT32 unsigned, unnormalized 32-bit integer.
#define CLK_UNSIGNED_INT32 0x10DC
/// @brief CLK_HALF_FLOAT 16-bit half float.
#define CLK_HALF_FLOAT 0x10DD
/// @brief CLK_FLOAT single precision float.
#define CLK_FLOAT 0x10DE
/// @brief CLK_UNORM_INT24 listed but not supported yet.
#define CLK_UNORM_INT24 0x10DF
/// @}

/// @brief Sampler addressing mode.
///
/// Specifies the image addressing-mode i.e. how out-of range image coordinates
/// are handled.
/// See "The Spir Specification Version 1.2" section:
/// "2.1.3.1 Declaring sampler variables."
/// @weakgroup AddressingMode
/// @{

/// @brief CLK_ADDRESS_NONE for this addressing mode the programmer guarantees
/// that the image coordinates used to sample elements of the image refer to a
/// location inside the image; otherwise the results are undefined.
#define CLK_ADDRESS_NONE 0x0000
/// @brief CLK_ADDRESS_CLAMP_TO_EDGE out-of-range image coordinates are clamped
/// to the extent.
#define CLK_ADDRESS_CLAMP_TO_EDGE 0x0002
/// @brief CLK_ADDRESS_CLAMP out-of-range image coordinates will return a border
/// color.
#define CLK_ADDRESS_CLAMP 0x0004
/// @brief CLK_ADDRESS_REPEAT out-of-range image coordinates are wrapped to
/// the valid range. This addressing mode can only be used with normalized
/// coordinates. If normalized coordinates are not used, this addressing mode
/// may generate image coordinates that are undefined.
#define CLK_ADDRESS_REPEAT 0x0006
/// @brief CLK_ADDRESS_MIRRORED_REPEAT flip the image coordinate at every
/// integer junction. This addressing mode can only be used with normalized
/// coordinates. If normalized coordinates are not used, this addressing mode
/// may generate image coordinates that are undefined.
#define CLK_ADDRESS_MIRRORED_REPEAT 0x0008
/// @}

/// @brief Sampler normalized coordinates.
///
/// Specifies whether the x, y and z coordinates are passed in as normalized or
/// unnormalized values. This must be a literal value and can be one of the
/// following predefined enums.
/// See "The Spir Specification Version 1.2" section:
/// "2.1.3.1 Declaring sampler variables."
/// @weakgroup NormalizedCoords
/// @{

/// @brief CLK_NORMALIZED_COORDS_FALSE do not use normalized coordinates.
#define CLK_NORMALIZED_COORDS_FALSE 0x0000
/// @brief CLK_NORMALIZED_COORDS_TRUE use normalized coordinates.
#define CLK_NORMALIZED_COORDS_TRUE 0x0001
/// @}

/// @brief Sampler filtering modes.
///
/// Specifies the filter mode to use. This must be a literal value and can be
/// one of the following predefined enums.
/// See "The Spir Specification Version 1.2" section:
/// "2.1.3.1 Declaring sampler variables."
/// @weakgroup FilterMode
/// @{

/// @brief CLK_FILTER_NEAREST  the image
/// element in the image that is nearest (in Manhattan distance) to that
/// specified by (u,v,w) is obtained
#define CLK_FILTER_NEAREST 0x0010
/// @brief CLK_FILTER_LINEAR  a 2 x 2 square of image elements for a 2D image or
/// a 2 x 2 x 2 cube of image elements for a 3D image is selected.
#define CLK_FILTER_LINEAR 0x0020
/// @}
#endif

/// @brief Masks to retrieve sampler elements.
/// @weakgroup Masks
/// @{

/// @brief NORMALIZED_COORDS_MASK when anded with sampler value, returns
/// normalized coordinates.
#define NORMALIZED_COORDS_MASK 0x1
/// @brief ADDRESSING_MODE_MASK when anded with sampler value, returns
/// addressing mode.
#define ADDRESSING_MODE_MASK 0xE
/// @brief FILTER_MODE_MASK when anded with sampler value, returns
/// filtering mode.
#define FILTER_MODE_MASK 0x30
/// @}

/// @brief Sampler, image_library's representation of sampler.
typedef libimg::UInt Sampler;

/// @brief An image descriptor, used by both the host and kernel API's.
typedef struct {
  /// @brief Description of the order of channels in the pixel.
  libimg::UInt channel_order;
  /// @brief Description of the type of each pixel element.
  libimg::UInt channel_type;
  /// @brief Size in bytes of a single pixel.
  libimg::Size pixel_size;
  /// @brief Image width in pixels.
  libimg::Size width;
  /// @brief Image height in pixels, should be 1 for 1D images.
  libimg::Size height;
  /// @brief Image depth in pixels, should be 1 for 1D and 2D images.
  libimg::Size depth;
  /// @brief Image array size, should be 1 for non-array images.
  libimg::Size array_size;
  /// @brief Size in bytes of a row of the image.
  libimg::Size row_pitch;
  /// @brief Size in bytes of a slice on the image.
  libimg::Size slice_pitch;
} ImageMetaData;

/// @brief An image object, used by both the host and kernel API's.
typedef struct {
  /// @brief Embedded description of the image data.
  ImageMetaData meta_data;
  /// @brief Pointer to the actual image data.
  libimg::UChar* raw_data;
} Image;

/// @}

#endif  // CODEPLAY_IMG_SHARED_H
