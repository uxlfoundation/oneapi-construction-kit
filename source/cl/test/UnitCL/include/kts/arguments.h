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
#ifndef UNITCL_KTS_ARGUMENTS_H_INCLUDED
#define UNITCL_KTS_ARGUMENTS_H_INCLUDED

#include <CL/cl.h>
#include <cargo/allocator.h>
#include <kts/arguments_shared.h>
#include <ucl/types.h>

#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace kts {
namespace ucl {

struct PointerPrimitive;

// Describes the settings for a sampler parameter
struct SamplerDesc {
  SamplerDesc(cl_bool normalized_coords, cl_addressing_mode addressing_mode,
              cl_filter_mode filter_mode)
      : normalized_coords_(normalized_coords),
        addressing_mode_(addressing_mode),
        filter_mode_(filter_mode),
        sampler_(nullptr) {}
  SamplerDesc()
      : normalized_coords_(CL_FALSE),
        addressing_mode_(CL_ADDRESS_NONE),
        filter_mode_(CL_FILTER_NEAREST),
        sampler_(nullptr) {}

  ~SamplerDesc() {
    if (sampler_) {
      clReleaseSampler(sampler_);
    }
  }

  cl_bool normalized_coords_;
  cl_addressing_mode addressing_mode_;
  cl_filter_mode filter_mode_;
  cl_sampler sampler_;
};

// Describes the settings for the image parameter
struct ImageDesc {
  ImageDesc(const cl_image_format &format, const cl_image_desc &desc)
      : format_(format), desc_(desc) {}
  ImageDesc() {}

  cl_image_format format_;
  cl_image_desc desc_;
};

// Describes a kernel argument and the values it can take.
class Argument final : public ArgumentBase {
 public:
  Argument(ArgKind kind, size_t index)
      : ArgumentBase(kind, index), buffer_(nullptr) {}

  ~Argument() {
    if (buffer_) {
      clReleaseMemObject(buffer_);
    }
  }

  const BufferDesc &GetBufferDesc() const { return buffer_desc_; }
  void SetBufferDesc(const BufferDesc &new_desc) { buffer_desc_ = new_desc; }

  const cl_mem &GetBuffer() const { return buffer_; }
  void SetBuffer(const cl_mem &new_buffer) { buffer_ = new_buffer; }

  Primitive *GetPrimitive() const { return primitive_.get(); }
  void SetPrimitive(Primitive *new_prim) { primitive_.reset(new_prim); }

  virtual uint8_t *GetBufferStoragePtr() { return storage_.data(); }
  virtual size_t GetBufferStorageSize() { return storage_.size(); }
  virtual void SetBufferStorageSize(size_t size) { storage_.resize(size); }

  const SamplerDesc &GetSamplerDesc() const { return sampler_; }
  void SetSamplerDesc(const SamplerDesc &new_sampler) {
    sampler_ = new_sampler;
  }
  void SetSampler(cl_sampler sampler) { sampler_.sampler_ = sampler; }

  const ImageDesc &GetImageDesc() const { return image_; }
  void SetImageDesc(const ImageDesc &new_image) { image_ = new_image; }

 private:
  // Used to generate the argument's buffer (input) or validate the argument's
  // data.
  BufferDesc buffer_desc_;
  // OpenCL buffer if the argument is a pointer.
  cl_mem buffer_;
  // Primitive value if the argument is a primitive.
  std::unique_ptr<Primitive, cargo::deleter<Primitive>> primitive_;
  // Buffer data if the argument is a buffer.
  std::vector<uint8_t> storage_;
  // Used to generate the argument's sampler input.
  SamplerDesc sampler_;
  // Used to generate the argument's image input combined with buffer_desc_.
  ImageDesc image_;
};

// Describes the arguments passed to a kernel function as well as the
// global work dimensions. Can only be used when the dimension of buffers
// are the same than the N-D range. All buffers must also have the same
// element type since only one reference function is used.
class ArgumentList final {
 public:
  size_t GetCount() const { return args_.size(); }
  Argument *GetArg(unsigned index);
  const BufferDesc &GetBufferDesc() const { return default_desc_; }
  void SetBufferDesc(const BufferDesc &new_desc) { default_desc_ = new_desc; }
  BufferDesc GetBufferDescForArg(unsigned index) const;

  void AddInputBuffer(const BufferDesc &desc);
  void AddOutputBuffer(const BufferDesc &desc);
  void AddInOutBuffer(const BufferDesc &desc);
  // `primitive` must have been allocated with cargo::alloc.
  void AddLocalBuffer(PointerPrimitive *primitive);
  // `primitive` must have been allocated with cargo::alloc.
  void AddPrimitive(Primitive *primitive);
  void AddSampler(cl_bool normalized_coords, cl_addressing_mode addressing_mode,
                  cl_filter_mode filter_mode);
  void AddInputImage(const cl_image_format &format, const cl_image_desc &desc,
                     const BufferDesc &data);

 private:
  BufferDesc default_desc_;
  std::vector<std::unique_ptr<Argument>> args_;
};

struct PointerPrimitive : public Primitive {
  PointerPrimitive(size_t size) : size_(size) {}

  virtual void *GetAddress() { return nullptr; }
  virtual size_t GetSize() { return size_; }

  size_t size_;
};
}  // namespace ucl

template <>
struct Validator<cl_char4> {
  bool validate(cl_char4 expected, cl_char4 actual) {
    Validator<cl_char> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]) &&
           v.validate(expected.s[2], actual.s[2]) &&
           v.validate(expected.s[3], actual.s[3]);
  }

  void print(std::stringstream &s, cl_char4 value) {
    // Use cl_int since we may be printing 0 values which we don't want to
    // treat as a null string terminator.
    Validator<cl_int> v;
    s << "<";
    v.print(s, value.s[0]);
    for (auto i = 1u; i < 4u; ++i) {
      s << ",";
      v.print(s, value.s[i]);
    }
    s << ">";
  }
};

template <>
struct Validator<cl_char8> {
  bool validate(cl_char8 &expected, cl_char8 &actual) {
    Validator<cl_char> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]) &&
           v.validate(expected.s[2], actual.s[2]) &&
           v.validate(expected.s[3], actual.s[3]) &&
           v.validate(expected.s[4], actual.s[4]) &&
           v.validate(expected.s[5], actual.s[5]) &&
           v.validate(expected.s[6], actual.s[6]) &&
           v.validate(expected.s[7], actual.s[7]);
  }

  void print(std::stringstream &s, const cl_char8 &value) {
    // Use cl_int since we may be printing 0 values which we don't want to
    // treat as a null string terminator.
    Validator<cl_int> v;
    s << "<";
    v.print(s, value.s[0]);
    for (auto i = 1u; i < 8u; ++i) {
      s << ",";
      v.print(s, value.s[i]);
    }
    s << ">";
  }
};

template <>
struct Validator<cl_char16> {
  bool validate(cl_char16 &expected, cl_char16 &actual) {
    Validator<cl_char> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]) &&
           v.validate(expected.s[2], actual.s[2]) &&
           v.validate(expected.s[3], actual.s[3]) &&
           v.validate(expected.s[4], actual.s[4]) &&
           v.validate(expected.s[5], actual.s[5]) &&
           v.validate(expected.s[6], actual.s[6]) &&
           v.validate(expected.s[7], actual.s[7]) &&
           v.validate(expected.s[8], actual.s[8]) &&
           v.validate(expected.s[9], actual.s[9]) &&
           v.validate(expected.s[10], actual.s[10]) &&
           v.validate(expected.s[11], actual.s[11]) &&
           v.validate(expected.s[12], actual.s[12]) &&
           v.validate(expected.s[13], actual.s[13]) &&
           v.validate(expected.s[14], actual.s[14]) &&
           v.validate(expected.s[15], actual.s[15]);
  }

  void print(std::stringstream &s, const cl_char16 &value) {
    // Use cl_int since we may be printing 0 values which we don't want to
    // treat as a null string terminator.
    Validator<cl_int> v;
    s << "<";
    v.print(s, value.s[0]);
    for (auto i = 1u; i < 16u; ++i) {
      s << ",";
      v.print(s, value.s[i]);
    }
    s << ">";
  }
};

template <>
struct Validator<cl_int2> {
  bool validate(cl_int2 &expected, cl_int2 &actual) {
    Validator<cl_int> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]);
  }

  void print(std::stringstream &s, const cl_int2 &value) {
    Validator<cl_int> v;
    s << "<";
    v.print(s, value.s[0]);
    s << ",";
    v.print(s, value.s[1]);
    s << ">";
  }
};

template <>
struct Validator<cl_uint2> {
  bool validate(cl_uint2 &expected, cl_uint2 &actual) {
    Validator<cl_uint> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]);
  }

  void print(std::stringstream &s, const cl_uint2 &value) {
    Validator<cl_uint> v;
    s << "<";
    v.print(s, value.s[0]);
    s << ",";
    v.print(s, value.s[1]);
    s << ">";
  }
};

template <>
struct Validator<cl_short4> {
  bool validate(cl_short4 &expected, cl_short4 &actual) {
    Validator<cl_short> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]) &&
           v.validate(expected.s[2], actual.s[2]) &&
           v.validate(expected.s[3], actual.s[3]);
  }

  void print(std::stringstream &s, const cl_short4 &value) {
    Validator<cl_short> v;
    s << "<";
    v.print(s, value.s[0]);
    s << ",";
    v.print(s, value.s[1]);
    s << ",";
    v.print(s, value.s[2]);
    s << ",";
    v.print(s, value.s[3]);
    s << ">";
  }
};

template <>
struct Validator<cl_int4> {
  bool validate(cl_int4 &expected, cl_int4 &actual) {
    Validator<cl_int> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]) &&
           v.validate(expected.s[2], actual.s[2]) &&
           v.validate(expected.s[3], actual.s[3]);
  }

  void print(std::stringstream &s, const cl_int4 &value) {
    Validator<cl_int> v;
    s << "<";
    v.print(s, value.s[0]);
    s << ",";
    v.print(s, value.s[1]);
    s << ",";
    v.print(s, value.s[2]);
    s << ",";
    v.print(s, value.s[3]);
    s << ">";
  }
};

template <>
struct Validator<cl_uint4> {
  bool validate(cl_uint4 &expected, cl_uint4 &actual) {
    Validator<cl_uint> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]) &&
           v.validate(expected.s[2], actual.s[2]) &&
           v.validate(expected.s[3], actual.s[3]);
  }

  void print(std::stringstream &s, const cl_uint4 &value) {
    Validator<cl_uint> v;
    s << "<";
    v.print(s, value.s[0]);
    s << ",";
    v.print(s, value.s[1]);
    s << ",";
    v.print(s, value.s[2]);
    s << ",";
    v.print(s, value.s[3]);
    s << ">";
  }
};

template <>
struct Validator<cl_long2> {
  bool validate(cl_long2 &expected, cl_long2 &actual) {
    Validator<cl_long> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]);
  }

  void print(std::stringstream &s, const cl_long2 &value) {
    Validator<cl_long> v;
    s << "<";
    v.print(s, value.s[0]);
    s << ",";
    v.print(s, value.s[1]);
    s << ">";
  }
};

template <>
struct Validator<cl_ulong2> {
  bool validate(cl_ulong2 &expected, cl_ulong2 &actual) {
    Validator<cl_ulong> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]);
  }

  void print(std::stringstream &s, const cl_ulong2 &value) {
    Validator<cl_ulong> v;
    s << "<";
    v.print(s, value.s[0]);
    s << ",";
    v.print(s, value.s[1]);
    s << ">";
  }
};

template <>
struct Validator<cl_long4> {
  bool validate(cl_long4 &expected, cl_long4 &actual) {
    Validator<cl_long> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]) &&
           v.validate(expected.s[2], actual.s[2]) &&
           v.validate(expected.s[3], actual.s[3]);
  }

  void print(std::stringstream &s, const cl_long4 &value) {
    Validator<cl_long> v;
    s << "<";
    v.print(s, value.s[0]);
    s << ",";
    v.print(s, value.s[1]);
    s << ",";
    v.print(s, value.s[2]);
    s << ",";
    v.print(s, value.s[3]);
    s << ">";
  }
};

template <>
struct Validator<cl_float2> {
  bool validate(cl_float2 &expected, cl_float2 &actual) {
    Validator<cl_float> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]);
  }

  void print(std::stringstream &s, const cl_float2 &value) {
    Validator<cl_float> v;
    s << "<";
    v.print(s, value.s[0]);
    s << ",";
    v.print(s, value.s[1]);
    s << ">";
  }
};

template <>
struct Validator<cl_float4> {
  bool validate(cl_float4 &expected, cl_float4 &actual) {
    Validator<cl_float> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]) &&
           v.validate(expected.s[2], actual.s[2]) &&
           v.validate(expected.s[3], actual.s[3]);
  }

  void print(std::stringstream &s, const cl_float4 &value) {
    Validator<cl_float> v;
    s << "<";
    v.print(s, value.s[0]);
    s << ",";
    v.print(s, value.s[1]);
    s << ",";
    v.print(s, value.s[2]);
    s << ",";
    v.print(s, value.s[3]);
    s << ">";
  }
};

template <>
struct Validator<cl_float16> {
  bool validate(cl_float16 &expected, cl_float16 &actual) {
    Validator<cl_float> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]) &&
           v.validate(expected.s[2], actual.s[2]) &&
           v.validate(expected.s[3], actual.s[3]) &&
           v.validate(expected.s[4], actual.s[4]) &&
           v.validate(expected.s[5], actual.s[5]) &&
           v.validate(expected.s[6], actual.s[6]) &&
           v.validate(expected.s[7], actual.s[7]) &&
           v.validate(expected.s[8], actual.s[8]) &&
           v.validate(expected.s[9], actual.s[9]) &&
           v.validate(expected.s[10], actual.s[10]) &&
           v.validate(expected.s[11], actual.s[11]) &&
           v.validate(expected.s[12], actual.s[12]) &&
           v.validate(expected.s[13], actual.s[13]) &&
           v.validate(expected.s[14], actual.s[14]) &&
           v.validate(expected.s[15], actual.s[15]);
  }

  void print(std::stringstream &s, const cl_float16 &value) {
    Validator<cl_float> v;
    s << "<";
    v.print(s, value.s[0]);
    for (auto i = 1u; i < 16u; ++i) {
      s << ",";
      v.print(s, value.s[i]);
    }
    s << ">";
  }
};

template <>
struct Validator<cl_double4> {
  bool validate(cl_double4 &expected, cl_double4 &actual) {
    Validator<cl_double> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]) &&
           v.validate(expected.s[2], actual.s[2]) &&
           v.validate(expected.s[3], actual.s[3]);
  }

  void print(std::stringstream &s, const cl_double4 &value) {
    Validator<cl_double> v;
    s << "<";
    v.print(s, value.s[0]);
    s << ",";
    v.print(s, value.s[1]);
    s << ",";
    v.print(s, value.s[2]);
    s << ",";
    v.print(s, value.s[3]);
    s << ">";
  }
};

template <class T, class Tag>
struct MemoryAccessor<::ucl::PackedVector3Type<T, Tag>> {
  using vector_type = ::ucl::PackedVector3Type<T, Tag>;
  using value_type = typename vector_type::value_type;

  vector_type LoadFromBuffer(void *Ptr, size_t Offset) {
    void *const PtrPlusOffset =
        static_cast<uint8_t *>(Ptr) + (3 * Offset * sizeof(value_type));
    vector_type Val;
    memcpy(Val.data(), PtrPlusOffset, 3 * sizeof(value_type));
    return Val;
  }

  void StoreToBuffer(const vector_type &Val, void *Ptr, size_t Offset) {
    void *const PtrPlusOffset =
        static_cast<uint8_t *>(Ptr) + (3 * Offset * sizeof(value_type));
    memcpy(PtrPlusOffset, Val.data(), 3 * sizeof(value_type));
  }
};
}  // namespace kts

#endif  // UNITCL_KTS_ARGUMENTS_H_INCLUDED
