// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
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
/// @brief Contains declarations of the kernel image functions.

#ifndef CODEPLAY_IMG_KERNEL_H
#define CODEPLAY_IMG_KERNEL_H

#include <libimg/shared.h>

/// @brief Function definitions encapsulating the kernel API.
///
/// @addtogroup libimg
/// @{

/******************************************************************************/
/******************************IMAGE READ FUNCTIONS****************************/
/******************************************************************************/
/// @brief Built-in Image Read Functions.
///
/// See "The OpenCL Specification Version 1.2, Document Revision 19" section:
/// "6.12.14.2 Built-in image read functions."
/// @weakgroup ReadImageFunctions
/// @{

/// @brief corresponds to OpenCL: float4 read_imagef (image3d_t image, sampler_t
/// sampler, int4 coord )
///
/// Use the coordinate (coord.x, coord.y, coord.z) to do an element lookup in
/// the 3D image object specified by image. coord.w is ignored.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 4 integers.
///
/// @return built-in vector type of 4 floats.
libimg::Float4 __Codeplay_read_imagef_3d(Image *image, Sampler sampler,
                                         libimg::Int4 coord);

/// @brief corresponds to OpenCL: float4 read_imagef (image3d_t image, sampler_t
/// sampler, float4 coord )
///
/// Use the coordinate (coord.x, coord.y, coord.z) to do an element lookup in
/// the 3D image object specified by image. coord.w is ignored.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 4 floats.
///
/// @return built-in vector type of 4 floats.
libimg::Float4 __Codeplay_read_imagef_3d(Image *image, Sampler sampler,
                                         libimg::Float4 coord);

/// @brief corresponds to OpenCL: float4 read_imagef ( image2d_array_t image,
/// sampler_t sampler, int4 coord)
///
/// Use coord.xy to do an element lookup in the 2D image identified by coord.z
/// in the 2D image array specified by image
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 4 integers.
///
/// @return built-in vector type of 4 floats.
libimg::Float4 __Codeplay_read_imagef_2d_array(Image *image, Sampler sampler,
                                               libimg::Int4 coord);

/// @brief corresponds to OpenCL: float4 read_imagef ( image2d_array_t image,
/// sampler_t sampler, float4 coord)
///
/// Use coord.xy to do an element lookup in the 2D image identified by coord.z
/// in the 2D image array specified by image
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 4 floats.
///
/// @return built-in vector type of 4 floats.
libimg::Float4 __Codeplay_read_imagef_2d_array(Image *image, Sampler sampler,
                                               libimg::Float4 coord);

/// @brief corresponds to OpenCL: float4 read_imagef (image2d_t image, sampler_t
/// sampler, int2 coord)
///
/// Use the coordinate (coord.x, coord.y) to do an element lookup in the 2D
/// image object specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 2 integers.
///
/// @return built-in vector type of 4 floats.
libimg::Float4 __Codeplay_read_imagef_2d(Image *image, Sampler sampler,
                                         libimg::Int2 coord);

/// @brief corresponds to OpenCL: float4 read_imagef (image2d_t image,
/// sampler_t sampler, float2 coord)
///
/// Use the coordinate (coord.x, coord.y) to do an element lookup in the 2D
/// image object specified by image.
///
/// @param image pointer to an Image object used by both the host and the
/// kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 2 floats.
///
/// @return built-in vector type of 4 floats.
libimg::Float4 __Codeplay_read_imagef_2d(Image *image, Sampler sampler,
                                         libimg::Float2 coord);

/// @brief corresponds to OpenCL: float4 read_imagef ( image1d_array_t image,
/// sampler_t sampler, int2 coord)
///
/// Use coord.x to do an element lookup in the 1D image identified by coord.y in
/// the 1D image array specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 2 integers.
///
/// @return built-in vector type of 4 floats.
libimg::Float4 __Codeplay_read_imagef_1d_array(Image *image, Sampler sampler,
                                               libimg::Int2 coord);

/// @brief corresponds to OpenCL: float4 read_imagef ( image1d_array_t image,
/// sampler_t sampler, float2 coord)
///
/// Use coord.x to do an element lookup in the 1D image identified by coord.y in
/// the 1D image array specified by image.
///
/// @param image pointer to an Image object used by both the host and the
/// kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 2 floats.
///
/// @return built-in vector type of 4 floats.
libimg::Float4 __Codeplay_read_imagef_1d_array(Image *image, Sampler sampler,
                                               libimg::Float2 coord);

/// @brief corresponds to OpenCL: float4 read_imagef (image1d_t image, sampler_t
/// sampler, int coord)
///
/// Use coord to do an element lookup in the 1D image object specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord integer coordinate.
///
/// @return built-in vector type of 4 floats.
libimg::Float4 __Codeplay_read_imagef_1d(Image *image, Sampler sampler,
                                         libimg::Int coord);

/// @brief corresponds to OpenCL: float4 read_imagef (image1d_t image, sampler_t
/// sampler, float coord)
///
/// Use coord to do an element lookup in the 1D image object specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord float coordinate.
///
/// @return built-in vector type of 4 floats.
libimg::Float4 __Codeplay_read_imagef_1d(Image *image, Sampler sampler,
                                         libimg::Float coord);

/// @brief corresponds to OpenCL: int4 read_imagei (image3d_t image, sampler_t
/// sampler, int4 coord )
///
/// Use the coordinate (coord.x, coord.y, coord.z) to do an element lookup in
/// the 3D image object specified by image. coord.w is ignored.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 4 integers.
///
/// @return built-in vector type of 4 integers.
libimg::Int4 __Codeplay_read_imagei_3d(Image *image, Sampler sampler,
                                       libimg::Int4 coord);

/// @brief corresponds to OpenCL: int4 read_imagei (image3d_t image, sampler_t
/// sampler, float4 coord )
///
/// Use the coordinate (coord.x, coord.y, coord.z) to do an element lookup in
/// the 3D image object specified by image. coord.w is ignored.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 4 floats.
///
/// @return built-in vector type of 4 integers.
libimg::Int4 __Codeplay_read_imagei_3d(Image *image, Sampler sampler,
                                       libimg::Float4 coord);

/// @brief corresponds to OpenCL: int4 read_imagei ( image2d_array_t image,
/// sampler_t sampler, int4 coord)
///
/// Use coord.xy to do an element lookup in the 2D image identified by coord.z
/// in the 2D image array specified by image
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 4 integers.
///
/// @return built-in vector type of 4 integers.
libimg::Int4 __Codeplay_read_imagei_2d_array(Image *image, Sampler sampler,
                                             libimg::Int4 coord);

/// @brief corresponds to OpenCL: int4 read_imagei ( image2d_array_t image,
/// sampler_t sampler, float4 coord)
///
/// Use coord.xy to do an element lookup in the 2D image identified by coord.z
/// in the 2D image array specified by image
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 4 floats.
///
/// @return built-in vector type of 4 integers.
libimg::Int4 __Codeplay_read_imagei_2d_array(Image *image, Sampler sampler,
                                             libimg::Float4 coord);

/// @brief corresponds to OpenCL: int4 read_imagei (image2d_t image, sampler_t
/// sampler, int2 coord)
///
/// Use the coordinate (coord.x, coord.y) to do an element lookup in the 2D
/// image object specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 2 integers.
///
/// @return built-in vector type of 4 integers.
libimg::Int4 __Codeplay_read_imagei_2d(Image *image, Sampler sampler,
                                       libimg::Int2 coord);

/// @brief corresponds to OpenCL: int4 read_imagei (image2d_t image,
/// sampler_t sampler, float2 coord)
///
/// Use the coordinate (coord.x, coord.y) to do an element lookup in the 2D
/// image object specified by image.
///
/// @param image pointer to an Image object used by both the host and the
/// kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 2 floats.
///
/// @return built-in vector type of 4 integers.
libimg::Int4 __Codeplay_read_imagei_2d(Image *image, Sampler sampler,
                                       libimg::Float2 coord);

/// @brief corresponds to OpenCL: int4 read_imagei ( image1d_array_t image,
/// sampler_t sampler, int2 coord)
///
/// Use coord.x to do an element lookup in the 1D image identified by coord.y in
/// the 1D image array specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 2 integers.
///
/// @return built-in vector type of 4 integers.
libimg::Int4 __Codeplay_read_imagei_1d_array(Image *image, Sampler sampler,
                                             libimg::Int2 coord);

/// @brief corresponds to OpenCL: int4 read_imagei ( image1d_array_t image,
/// sampler_t sampler, float2 coord)
///
/// Use coord.x to do an element lookup in the 1D image identified by coord.y in
/// the 1D image array specified by image.
///
/// @param image pointer to an Image object used by both the host and the
/// kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 2 floats.
///
/// @return built-in vector type of 4 integers.
libimg::Int4 __Codeplay_read_imagei_1d_array(Image *image, Sampler sampler,
                                             libimg::Float2 coord);

/// @brief corresponds to OpenCL: int4 read_imagei (image1d_t image, sampler_t
/// sampler, int coord)
///
/// Use coord to do an element lookup in the 1D image object specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord integer coordinate.
///
/// @return built-in vector type of 4 integers.
libimg::Int4 __Codeplay_read_imagei_1d(Image *image, Sampler sampler,
                                       libimg::Int coord);

/// @brief corresponds to OpenCL: int4 read_imagei (image1d_t image, sampler_t
/// sampler, float coord)
///
/// Use coord to do an element lookup in the 1D image object specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord float coordinate.
///
/// @return built-in vector type of 4 integers.
libimg::Int4 __Codeplay_read_imagei_1d(Image *image, Sampler sampler,
                                       libimg::Float coord);

/// @brief corresponds to OpenCL: uint4 read_imageui (image3d_t image, sampler_t
/// sampler, float4 coord )
///
/// Use the coordinate (coord.x, coord.y, coord.z) to do an element lookup in
/// the 3D image object specified by image. coord.w is ignored.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 4 floats.
///
/// @return built-in vector type of 4 unsigned integers.
libimg::UInt4 __Codeplay_read_imageui_3d(Image *image, Sampler sampler,
                                         libimg::Int4 coord);

/// @brief corresponds to OpenCL: uint4 read_imageui (image3d_t image, sampler_t
/// sampler, float4 coord )
///
/// Use the coordinate (coord.x, coord.y, coord.z) to do an element lookup in
/// the 3D image object specified by image. coord.w is ignored.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 4 floats.
///
/// @return built-in vector type of 4 unsigned integers.
libimg::UInt4 __Codeplay_read_imageui_3d(Image *image, Sampler sampler,
                                         libimg::Float4 coord);

/// @brief corresponds to OpenCL: uint4 read_imageui ( image2d_array_t image,
/// sampler_t sampler, int4 coord)
///
/// Use coord.xy to do an element lookup in the 2D image identified by coord.z
/// in the 2D image array specified by image
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 4 integers.
///
/// @return built-in vector type of 4 unsigned integers.
libimg::UInt4 __Codeplay_read_imageui_2d_array(Image *image, Sampler sampler,
                                               libimg::Int4 coord);

/// @brief corresponds to OpenCL: uint4 read_imageui ( image2d_array_t image,
/// sampler_t sampler, float4 coord)
///
/// Use coord.xy to do an element lookup in the 2D image identified by coord.z
/// in the 2D image array specified by image
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 4 floats.
///
/// @return built-in vector type of 4 unsigned integers.
libimg::UInt4 __Codeplay_read_imageui_2d_array(Image *image, Sampler sampler,
                                               libimg::Float4 coord);

/// @brief corresponds to OpenCL: uint4 read_imageui (image2d_t image, sampler_t
/// sampler, int2 coord)
///
/// Use the coordinate (coord.x, coord.y) to do an element lookup in the 2D
/// image object specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 2 integers.
///
/// @return built-in vector type of 4 unsigned integers.
libimg::UInt4 __Codeplay_read_imageui_2d(Image *image, Sampler sampler,
                                         libimg::Int2 coord);

/// @brief corresponds to OpenCL: uint4 read_imageui (image2d_t image,
/// sampler_t sampler, float2 coord)
///
/// Use the coordinate (coord.x, coord.y) to do an element lookup in the 2D
/// image object specified by image.
///
/// @param image pointer to an Image object used by both the host and the
/// kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 2 floats.
///
/// @return built-in vector type of 4 unsigned integers.
libimg::UInt4 __Codeplay_read_imageui_2d(Image *image, Sampler sampler,
                                         libimg::Float2 coord);

/// @brief corresponds to OpenCL: uint4 read_imageui ( image1d_array_t image,
/// sampler_t sampler, int2 coord)
///
/// Use coord.x to do an element lookup in the 1D image identified by coord.y in
/// the 1D image array specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 2 integers.
///
/// @return built-in vector type of 4 unsigned integers.
libimg::UInt4 __Codeplay_read_imageui_1d_array(Image *image, Sampler sampler,
                                               libimg::Int2 coord);

/// @brief corresponds to OpenCL: uint4 read_imageui ( image1d_array_t image,
/// sampler_t sampler, float2 coord)
///
/// Use coord.x to do an element lookup in the 1D image identified by coord.y in
/// the 1D image array specified by image.
///
/// @param image pointer to an Image object used by both the host and the
/// kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord built-in vector type of 2 floats.
///
/// @return built-in vector type of 4 unsigned integers.
libimg::UInt4 __Codeplay_read_imageui_1d_array(Image *image, Sampler sampler,
                                               libimg::Float2 coord);

/// @brief corresponds to OpenCL: uint4 read_imageui (image1d_t image, sampler_t
/// sampler, int coord)
///
/// Use coord to do an element lookup in the 1D image object specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord integer coordinate.
///
/// @return built-in vector type of 4 unsigned integers.
libimg::UInt4 __Codeplay_read_imageui_1d(Image *image, Sampler sampler,
                                         libimg::Int coord);

/// @brief corresponds to OpenCL: uint4 read_imageui (image1d_t image, sampler_t
/// sampler, float coord)
///
/// Use coord to do an element lookup in the 1D image object specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param sampler Sampler value (32-bit integer).
/// @param coord float coordinate.
///
/// @return built-in vector type of 4 unsigned integers.
libimg::UInt4 __Codeplay_read_imageui_1d(Image *image, Sampler sampler,
                                         libimg::Float coord);
/// @}

/******************************************************************************/
/***********************IMAGE SAMPLER-LESS READ FUNCTIONS**********************/
/******************************************************************************/
/// @brief Built-in Image Sampler-less Read Functions.
///
/// See "The OpenCL Specification Version 1.2, Document Revision 19" section:
/// "6.12.14.3 Built-in Image Sampler-less Read Functions."
/// @weakgroup SamplerlessReadImageFunctions
/// @{

/// @brief corresponds to OpenCL: float4 read_imagef (image3d_t image, int4
/// coord )
///
/// Use the coordinate (coord.x, coord.y, coord.z) to do an element lookup in
/// the 3D image object specified by image. coord.w is ignored.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 4 integers.
///
/// @return build-in vector type of 4 floats.
libimg::Float4 __Codeplay_read_imagef_3d(Image *image, libimg::Int4 coord);

/// @brief corresponds to OpenCL: float4 read_imagef ( image2d_array_t image,
/// int4 coord)
///
/// Use coord.xy to do an element lookup in the 2D image identified by coord.z
/// in the 2D image array specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 4 integers.
///
/// @return build-in vector type of 4 floats.
libimg::Float4 __Codeplay_read_imagef_2d_array(Image *image,
                                               libimg::Int4 coord);

/// @brief corresponds to OpenCL: float4 read_imagef ( image1d_array_t image,
/// int2 coord)
///
/// Use coord.x to do an element lookup in the 1D image identified by coord.y in
/// the 1D image array specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 2 integers.
///
/// @return build-in vector type of 4 floats.
libimg::Float4 __Codeplay_read_imagef_1d_array(Image *image,
                                               libimg::Int2 coord);

/// @brief corresponds to OpenCL: float4 read_imagef (image2d_t image, int2
/// coord)
///
/// Use the coordinate (coord.x, coord.y) to do an element lookup in the 2D
/// image object specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 2 integers.
///
/// @return build-in vector type of 4 floats.
libimg::Float4 __Codeplay_read_imagef_2d(Image *image, libimg::Int2 coord);

/// @brief corresponds to OpenCL: float4 read_imagef (image1d_t image, int
/// coord)
///
/// Use coord to do an element lookup in the 1D image or 1D image buffer object
/// specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord integer coordinate.
///
/// @return build-in vector type of 4 floats.
libimg::Float4 __Codeplay_read_imagef_1d(Image *image, libimg::Int coord);

/// @brief corresponds to OpenCL: int4 read_imagei (image3d_t image, int4
/// coord )
///
/// Use the coordinate (coord.x, coord.y, coord.z) to do an element lookup in
/// the 3D image object specified by image. coord.w is ignored.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 4 integers.
///
/// @return build-in vector type of 4 integers.
libimg::Int4 __Codeplay_read_imagei_3d(Image *image, libimg::Int4 coord);

/// @brief corresponds to OpenCL: int4 read_imagei ( image2d_array_t image,
/// int4 coord)
///
/// Use coord.xy to do an element lookup in the 2D image identified by coord.z
/// in the 2D image array specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 4 integers.
///
/// @return build-in vector type of 4 integers.
libimg::Int4 __Codeplay_read_imagei_2d_array(Image *image, libimg::Int4 coord);

/// @brief corresponds to OpenCL: int4 read_imagei (image2d_t image, int2
/// coord)
///
/// Use the coordinate (coord.x, coord.y) to do an element lookup in the 2D
/// image object specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 2 integers.
///
/// @return build-in vector type of 4 integers.
libimg::Int4 __Codeplay_read_imagei_2d(Image *image, libimg::Int2 coord);

/// @brief corresponds to OpenCL: int4 read_imagei ( image1d_array_t image,
/// int2 coord)
///
/// Use coord.x to do an element lookup in the 1D image identified by coord.y in
/// the 1D image array specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 2 integers.
///
/// @return build-in vector type of 4 integers.
libimg::Int4 __Codeplay_read_imagei_1d_array(Image *image, libimg::Int2 coord);

/// @brief corresponds to OpenCL: int4 read_imagei (image1d_t image, int
/// coord)
///
/// Use coord to do an element lookup in the 1D image or 1D image buffer object
/// specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord integer coordinate.
///
/// @return build-in vector type of 4 integers.
libimg::Int4 __Codeplay_read_imagei_1d(Image *image, libimg::Int coord);

/// @brief corresponds to OpenCL: uint4 read_imageui (image3d_t image, int4
/// coord )
///
/// Use the coordinate (coord.x, coord.y, coord.z) to do an element lookup in
/// the 3D image object specified by image. coord.w is ignored.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 4 integers.
///
/// @return build-in vector type of 4 unsigned integers.
libimg::UInt4 __Codeplay_read_imageui_3d(Image *image, libimg::Int4 coord);

/// @brief corresponds to OpenCL: uint4 read_imageui ( image2d_array_t image,
/// int4 coord)
///
/// Use coord.xy to do an element lookup in the 2D image identified by coord.z
/// in the 2D image array specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 4 integers.
///
/// @return build-in vector type of 4 unsigned integers.
libimg::UInt4 __Codeplay_read_imageui_2d_array(Image *image,
                                               libimg::Int4 coord);

/// @brief corresponds to OpenCL: uint4 read_imageui (image2d_t image, int2
/// coord)
///
/// Use the coordinate (coord.x, coord.y) to do an element lookup in the 2D
/// image object specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 2 integers.
///
/// @return build-in vector type of 4 unsigned integers.
libimg::UInt4 __Codeplay_read_imageui_2d(Image *image, libimg::Int2 coord);

/// @brief corresponds to OpenCL: uint4 read_imageui ( image1d_array_t image,
/// int2 coord)
///
/// Use coord.x to do an element lookup in the 1D image identified by coord.y in
/// the 1D image array specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 2 integers.
///
/// @return build-in vector type of 4 unsigned integers.
libimg::UInt4 __Codeplay_read_imageui_1d_array(Image *image,
                                               libimg::Int2 coord);

/// @brief corresponds to OpenCL: uint4 read_imageui (image1d_t image, int
/// coord)
///
/// Use coord to do an element lookup in the 1D image or 1D image buffer object
/// specified by image.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord integer coordinate.
///
/// @return build-in vector type of 4 unsigned integers.
libimg::UInt4 __Codeplay_read_imageui_1d(Image *image, libimg::Int coord);
/// @}

/******************************************************************************/
/****************************IMAGE WRITE FUNCTIONS*****************************/
/******************************************************************************/
/// @brief Built-in Image Write Functions.
///
/// See "The OpenCL Specification Version 1.2, Document Revision 19" section:
/// "6.12.14.4 Built-in Image Write Functions."
/// @weakgroup WriteImageFunctions
/// @{

// 3d writes are OpenCL extension.
void __Codeplay_write_imagef_3d(Image *image, libimg::Int4 coord,
                                libimg::Float4 color);

/// @brief corresponds to OpenCL: void write_imagef ( image2d_array_t image,
/// int4 coord, float4 color)
///
/// Write color value to location specified by coord.xy in the 2D image
/// identified by coord.z in the 2D image array specified by image. Appropriate
/// data format conversion to the specified image format is done before writing
/// the color value. coord.x, coord.y and coord.z are considered to be
/// unnormalized coordinates and must be in the range 0 ... image width – 1, 0 …
/// image height – 1 and 0 … image number of layers – 1.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 4 integers.
/// @param color built-in vector type of 4 floats.
void __Codeplay_write_imagef_2d_array(Image *image, libimg::Int4 coord,
                                      libimg::Float4 color);

/// @brief corresponds to OpenCL: void write_imagef (image2d_t image, int2
/// coord, float4 color)
///
/// Write color value to location specified by coord.xy in the 2D image object
/// specified by image.  Appropriate data format conversion to the specified
/// image format is done before writing the color value.  coord.x and coord.y
/// are considered to be unnormalized coordinates and must be in the range 0 ...
/// image width – 1, and 0 … image height – 1.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 2 integers.
/// @param color built-in vector type of 4 floats.
void __Codeplay_write_imagef_2d(Image *image, libimg::Int2 coord,
                                libimg::Float4 color);

/// @brief corresponds to OpenCL: void write_imagef ( image1d_array_t image,
/// int2 coord, float4 color)
///
/// Write color value to location specified by coord.x in the 1D image
/// identified by coord.y in the 1D image array specified by image. Appropriate
/// data format conversion to the specified image format is done before writing
/// the color value. coord.x and coord.y are considered to be unnormalized
/// coordinates and must be in the range 0 ... image width – 1 and 0 … image
/// number of layers – 1.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 2 integers.
/// @param color built-in vector type of 4 floats.
void __Codeplay_write_imagef_1d_array(Image *image, libimg::Int2 coord,
                                      libimg::Float4 color);

/// @brief corresponds to OpenCL: void write_imagef (image1d_t image, int coord,
/// float4 color)
///
/// Write color value to location specified by coord in the 1D image or 1D image
/// buffer object specified by image. Appropriate data format conversion to the
/// specified image format is done before writing the color value. coord is
/// considered to be unnormalized coordinates and must be in the range 0 ...
/// image width – 1
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord integer coordinate.
/// @param color built-in vector type of 4 floats.
void __Codeplay_write_imagef_1d(Image *image, libimg::Int coord,
                                libimg::Float4 color);

// 3d writes are OpenCL extension.
void __Codeplay_write_imagei_3d(Image *image, libimg::Int4 coord,
                                libimg::Int4 color);

/// @brief corresponds to OpenCL: void write_imagei ( image2d_array_t image,
/// int4 coord, int4 color)
///
/// Write color value to location specified by coord.xy in the 2D image
/// identified by coord.z in the 2D image array specified by image. Appropriate
/// data format conversion to the specified image format is done before writing
/// the color value. coord.x, coord.y and coord.z are considered to be
/// unnormalized coordinates and must be in the range 0 ... image width – 1, 0 …
/// image height – 1 and 0 … image number of layers – 1.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 4 integers.
/// @param color built-in vector type of 4 integers.
void __Codeplay_write_imagei_2d_array(Image *image, libimg::Int4 coord,
                                      libimg::Int4 color);

/// @brief corresponds to OpenCL: void write_imagei (image2d_t image, int2
/// coord, int4 color)
///
/// Write color value to location specified by coord.xy in the 2D image object
/// specified by image.  Appropriate data format conversion to the specified
/// image format is done before writing the color value.  coord.x and coord.y
/// are considered to be unnormalized coordinates and must be in the range 0 ...
/// image width – 1, and 0 … image height – 1.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 2 integers.
/// @param color built-in vector type of 4 integers.
void __Codeplay_write_imagei_2d(Image *image, libimg::Int2 coord,
                                libimg::Int4 color);

/// @brief corresponds to OpenCL: void write_imagei ( image1d_array_t image,
/// int2 coord, int4 color)
///
/// Write color value to location specified by coord.x in the 1D image
/// identified by coord.y in the 1D image array specified by image. Appropriate
/// data format conversion to the specified image format is done before writing
/// the color value. coord.x and coord.y are considered to be unnormalized
/// coordinates and must be in the range 0 ... image width – 1 and 0 … image
/// number of layers – 1.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 2 integers.
/// @param color built-in vector type of 4 integers.
void __Codeplay_write_imagei_1d_array(Image *image, libimg::Int2 coord,
                                      libimg::Int4 color);

/// @brief corresponds to OpenCL: void write_imagei (image1d_t image, int coord,
/// int4 color)
///
/// Write color value to location specified by coord in the 1D image or 1D image
/// buffer object specified by image. Appropriate data format conversion to the
/// specified image format is done before writing the color value. coord is
/// considered to be unnormalized coordinates and must be in the range 0 ...
/// image width – 1
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord integer coordinate.
/// @param color built-in vector type of 4 integer.
void __Codeplay_write_imagei_1d(Image *image, libimg::Int coord,
                                libimg::Int4 color);

// 3d writes are OpenCL extension.
void __Codeplay_write_imageui_3d(Image *image, libimg::Int4 coord,
                                 libimg::UInt4 color);

/// @brief corresponds to OpenCL: void write_imageui ( image2d_array_t image,
/// int4 coord, uint4 color)
///
/// Write color value to location specified by coord.xy in the 2D image
/// identified by coord.z in the 2D image array specified by image. Appropriate
/// data format conversion to the specified image format is done before writing
/// the color value. coord.x, coord.y and coord.z are considered to be
/// unnormalized coordinates and must be in the range 0 ... image width – 1, 0 …
/// image height – 1 and 0 … image number of layers – 1.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 4 integers.
/// @param color built-in vector type of 4 unsigned integers.
void __Codeplay_write_imageui_2d_array(Image *image, libimg::Int4 coord,
                                       libimg::UInt4 color);

/// @brief corresponds to OpenCL: void write_imageui (image2d_t image, int2
/// coord, uint4 color)
///
/// Write color value to location specified by coord.xy in the 2D image object
/// specified by image.  Appropriate data format conversion to the specified
/// image format is done before writing the color value.  coord.x and coord.y
/// are considered to be unnormalized coordinates and must be in the range 0 ...
/// image width – 1, and 0 … image height – 1.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 2 integers.
/// @param color built-in vector type of 4 unsigned integers.
void __Codeplay_write_imageui_2d(Image *image, libimg::Int2 coord,
                                 libimg::UInt4 color);

/// @brief corresponds to OpenCL: void write_imageui ( image1d_array_t image,
/// int2 coord, uint4 color)
///
/// Write color value to location specified by coord.x in the 1D image
/// identified by coord.y in the 1D image array specified by image. Appropriate
/// data format conversion to the specified image format is done before writing
/// the color value. coord.x and coord.y are considered to be unnormalized
/// coordinates and must be in the range 0 ... image width – 1 and 0 … image
/// number of layers – 1.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord built-in vector type of 2 integers.
/// @param color built-in vector type of 4 unsigned integers.
void __Codeplay_write_imageui_1d_array(Image *image, libimg::Int2 coord,
                                       libimg::UInt4 color);

/// @brief corresponds to OpenCL: void write_imageui (image1d_t image, int
/// coord,
/// uint4 color)
///
/// Write color value to location specified by coord in the 1D image or 1D image
/// buffer object specified by image. Appropriate data format conversion to the
/// specified image format is done before writing the color value. coord is
/// considered to be unnormalized coordinates and must be in the range 0 ...
/// image width – 1
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
/// @param coord integer coordinate.
/// @param color built-in vector type of 4 unsigned integers.
void __Codeplay_write_imageui_1d(Image *image, libimg::Int coord,
                                 libimg::UInt4 color);
/// @}

/******************************************************************************/
/****************************IMAGE QUERY FUNCTIONS*****************************/
/******************************************************************************/
/// @brief Built-in Image Query Functions.
///
/// See "The OpenCL Specification Version 1.2, Document Revision 19" section:
/// "6.12.14.5 Built-in Image Query Functions."
/// @weakgroup ImageQueryFunctions
/// @{

/// @brief corresponds to OpenCL: int get_image_width (iamge type)
///
/// Return the image width in pixels.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
///
/// @return integer.
libimg::Int __Codeplay_get_image_width(Image *image);

/// @brief corresponds to OpenCL: int get_image_height (image type)
///
/// Return the image height in pixels.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
///
/// @return integer.
libimg::Int __Codeplay_get_image_height(Image *image);

/// @brief corresponds to OpenCL: int get_image_depth (image3d_t image)
///
/// Return the image depth in pixels.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
///
/// @return integer image.
libimg::Int __Codeplay_get_image_depth(Image *image);

/// @brief corresponds to OpenCL: int get_image_channel_data_type (image type)
///
/// Return the channel data type.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
///
/// @return integer.
libimg::Int __Codeplay_get_image_channel_data_type(Image *image);

/// @brief corresponds to OpenCL: int get_image_channel_order (image type)
///
/// Return the image channel order.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
///
/// @return integer.
libimg::Int __Codeplay_get_image_channel_order(Image *image);

/// @brief corresponds to OpenCL: int2 get_image_dim (image2d_t image)
/// int2 get_image_dim ( image2d_array_t image)
///
/// Return the 2D image width and height as an int2 type. The width is returned
/// in the x component, and the height in the y component.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
///
/// @return integer.
libimg::Int2 __Codeplay_get_image_dim_vec2(Image *image);

/// @brief corresponds to OpenCL: int4 get_image_dim (image3d_t image)
///
/// Return the 3D image width, height, and depth as an int4 type. The width is
/// returned in the x component, height in the y component, depth in the z
/// component and the w component is 0.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
///
/// @return integer.
libimg::Int4 __Codeplay_get_image_dim_vec4(Image *image);

/// @brief corresponds to OpenCL: size_t get_image_array_size( image2d_array_t
/// image), or size_t get_image_array_size( image1d_array_t image)
///
/// Return the number of images in the 1D/2D image array.
///
/// @param image pointer to an Image object used by both the host and the kernel
/// side API's.
///
/// @return size_t
libimg::Size __Codeplay_get_image_array_size(Image *image);
/// @}
/// @}

#endif  // CODEPLAY_IMG_KERNEL_H
