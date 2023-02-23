// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <libimg/kernel.h>

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Debug macros.                                                             */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
#ifdef NDEBUG
#define debug_printf(F, ...)
#else
#define debug_printf(F, ...) __builtin_printf("==> " F, ##__VA_ARGS__)
#endif

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Maths helpers.                                                           */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
inline libimg::Float frac(libimg::Float x) { return x - libimg::floor(x); }

template <typename VecTy, typename VecElemTy, typename ScalarTy>
inline VecTy vec4_times_scalar(VecTy &vec, const ScalarTy scalar) {
  libimg::set_v4(vec,
                 libimg::get_v4<VecElemTy>(vec, libimg::vec_elem::x) * scalar,
                 libimg::vec_elem::x);
  libimg::set_v4(vec,
                 libimg::get_v4<VecElemTy>(vec, libimg::vec_elem::y) * scalar,
                 libimg::vec_elem::y);
  libimg::set_v4(vec,
                 libimg::get_v4<VecElemTy>(vec, libimg::vec_elem::z) * scalar,
                 libimg::vec_elem::z);
  libimg::set_v4(vec,
                 libimg::get_v4<VecElemTy>(vec, libimg::vec_elem::w) * scalar,
                 libimg::vec_elem::w);

  return vec;
}

template <typename VecTy, typename VecElemTy>
inline VecTy vec4_plus_vec4(const VecTy &a, const VecTy &b) {
  VecTy res;
  libimg::set_v4(res,
                 libimg::get_v4<VecElemTy>(a, libimg::vec_elem::x) +
                     libimg::get_v4<VecElemTy>(b, libimg::vec_elem::x),
                 libimg::vec_elem::x);
  libimg::set_v4(res,
                 libimg::get_v4<VecElemTy>(a, libimg::vec_elem::y) +
                     libimg::get_v4<VecElemTy>(b, libimg::vec_elem::y),
                 libimg::vec_elem::y);
  libimg::set_v4(res,
                 libimg::get_v4<VecElemTy>(a, libimg::vec_elem::z) +
                     libimg::get_v4<VecElemTy>(b, libimg::vec_elem::z),
                 libimg::vec_elem::z);
  libimg::set_v4(res,
                 libimg::get_v4<VecElemTy>(a, libimg::vec_elem::w) +
                     libimg::get_v4<VecElemTy>(b, libimg::vec_elem::w),
                 libimg::vec_elem::w);

  return res;
}

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Channel elem access helpers.                                             */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
inline libimg::Char snorm_char_from_float(libimg::Float x) {
  // CL_SNORM_INT8
  return libimg::convert_char_sat_rte(x * 127.0f);
}

inline libimg::Short snorm_short_from_float(libimg::Float x) {
  // CL_SNORM_INT16
  return libimg::convert_short_sat_rte(x * 32767.0f);
}

inline libimg::UChar unorm_char_from_float(libimg::Float x) {
  // CL_UNORM_INT8
  return libimg::convert_uchar_sat_rte(x * 255.0f);
}

inline libimg::UShort unorm_short_from_float(libimg::Float x) {
  // CL_UNORM_INT16
  return libimg::convert_ushort_sat_rte(x * 65535.0f);
}

inline libimg::UShort unorm_5_from_float(libimg::Float x) {
  // CL_UNORM_SHORT_555
  // Returns 5 bits of short from a normalised float
  const libimg::UShort a = libimg::convert_ushort_sat_rte(x * 31.0f);
  const libimg::UShort b = 0x1f;
  return libimg::min(a, b);
}

inline libimg::UShort unorm_6_from_float(libimg::Float x) {
  // CL_UNORM_SHORT_565
  // Returns 6 bits of short from a normalised float
  const libimg::UShort a = libimg::convert_ushort_sat_rte(x * 63.0f);
  const libimg::UShort b = 0x3f;
  return libimg::min(a, b);
}

inline libimg::UInt unorm_int_10_from_float(libimg::Float x) {
  // CL_UNORM_INT_101010
  // Returns 10 bits of int from a normalised float
  // To disambiguate libimg::min, provide it with concrete types.
  const libimg::UInt a = libimg::convert_ushort_sat_rte(x * 1023.0f);
  const libimg::UInt b = 0x3ff;
  return libimg::min(a, b);
}

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Sampler helpers.                                                         */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
inline libimg::Bool get_sampler_normalized_coords(const Sampler sampler) {
  return sampler & NORMALIZED_COORDS_MASK;
}

inline libimg::UInt get_sampler_addressing_mode(const Sampler sampler) {
  return sampler & ADDRESSING_MODE_MASK;
}

inline libimg::UInt get_sampler_filter_mode(const Sampler sampler) {
  return sampler & FILTER_MODE_MASK;
}

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* See 8.3.1.1 "Converting normalized integer channel data types to          */
/*  floating-point values" for detail of how conversion is done.             */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
libimg::Float4 vec_float4_from_unorm_555(const libimg::UShort *pixel_data,
                                         libimg::UInt channel_order) {
  libimg::Float4 res;
  switch (channel_order) {
    default:
      libimg::set_v4(res, 0.0f, libimg::vec_elem::x);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::w);
      return res;
    case CLK_RGB:
    case CLK_RGBx:  // libimg::Intentional fall-through.
      libimg::set_v4(res, ((*pixel_data & 0x7C00) >> 10) / 31.0f,
                     libimg::vec_elem::x);
      libimg::set_v4(res, (((*pixel_data & 0x3E0) >> 5)) / 31.0f,
                     libimg::vec_elem::y);
      libimg::set_v4(res, (((*pixel_data & 0x1F))) / 31.0f,
                     libimg::vec_elem::z);
      libimg::set_v4(res, 1.0f, libimg::vec_elem::w);
      return res;
  }
}

libimg::Float4 vec_float4_from_unorm_565(const libimg::UShort *pixel_data,
                                         libimg::UInt channel_order) {
  libimg::Float4 res;
  switch (channel_order) {
    default:
      libimg::set_v4(res, 0.0f, libimg::vec_elem::x);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::w);
      return res;
    case CLK_RGB:
    case CLK_RGBx:  // libimg::Intentional fall-through.
      libimg::set_v4(res, ((*pixel_data & 0xF800) >> 11) / 31.0f,
                     libimg::vec_elem::x);
      libimg::set_v4(res, (((*pixel_data & 0x7E0) >> 5)) / 63.0f,
                     libimg::vec_elem::y);
      libimg::set_v4(res, (((*pixel_data & 0x1F))) / 31.0f,
                     libimg::vec_elem::z);
      libimg::set_v4(res, 1.0f, libimg::vec_elem::w);
      return res;
  }
}

libimg::Float4 vec_float4_from_unorm_101010(const libimg::UInt *pixel_data,
                                            libimg::UInt channel_order) {
  libimg::Float4 res;
  switch (channel_order) {
    default:
      libimg::set_v4(res, 0.0f, libimg::vec_elem::x);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::w);
      return res;
    case CLK_RGB:
    case CLK_RGBx:  // libimg::Intentional fall-through.
      libimg::set_v4(res, ((*pixel_data & 0x3FF00000) >> 20) / 1023.0f,
                     libimg::vec_elem::x);
      libimg::set_v4(res, (((*pixel_data & 0xFFC00) >> 10)) / 1023.0f,
                     libimg::vec_elem::y);
      libimg::set_v4(res, (((*pixel_data & 0x3FF))) / 1023.0f,
                     libimg::vec_elem::z);
      libimg::set_v4(res, 1.0f, libimg::vec_elem::w);
      return res;
  }
}

libimg::Float4 vec_float4_from_half(const libimg::UShort *pixel_data,
                                    libimg::UInt channel_order) {
  libimg::Float4 res;
  switch (channel_order) {
    default:
      libimg::set_v4(res, 0.0f, libimg::vec_elem::x);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::w);
      return res;
    case CLK_R:  // Fall through
    case CLK_Rx: {
      const libimg::Float res_x = libimg::convert_half_to_float(pixel_data[0]);
      libimg::set_v4(res, res_x, libimg::vec_elem::x);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, 1.0f, libimg::vec_elem::w);
      return res;
    }
    case CLK_A: {
      libimg::set_v4(res, 0.0f, libimg::vec_elem::x);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      const libimg::Float res_w = libimg::convert_half_to_float(pixel_data[0]);
      libimg::set_v4(res, res_w, libimg::vec_elem::w);
      return res;
    }
    case CLK_RG:  // Fall through
    case CLK_RGx: {
      const libimg::Float res_x = libimg::convert_half_to_float(pixel_data[0]);
      const libimg::Float res_y = libimg::convert_half_to_float(pixel_data[1]);
      libimg::set_v4(res, res_x, libimg::vec_elem::x);
      libimg::set_v4(res, res_y, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, 1.0f, libimg::vec_elem::w);
      return res;
    }
    case CLK_RA: {
      const libimg::Float res_x = libimg::convert_half_to_float(pixel_data[0]);
      const libimg::Float res_w = libimg::convert_half_to_float(pixel_data[1]);
      libimg::set_v4(res, res_x, libimg::vec_elem::x);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, res_w, libimg::vec_elem::w);
      return res;
    }
    case CLK_RGBA: {
      const libimg::Float res_x = libimg::convert_half_to_float(pixel_data[0]);
      const libimg::Float res_y = libimg::convert_half_to_float(pixel_data[1]);
      const libimg::Float res_z = libimg::convert_half_to_float(pixel_data[2]);
      const libimg::Float res_w = libimg::convert_half_to_float(pixel_data[3]);
      libimg::set_v4(res, res_x, libimg::vec_elem::x);
      libimg::set_v4(res, res_y, libimg::vec_elem::y);
      libimg::set_v4(res, res_z, libimg::vec_elem::z);
      libimg::set_v4(res, res_w, libimg::vec_elem::w);
      return res;
    }
    case CLK_INTENSITY: {
      const libimg::Float intensity =
          libimg::convert_half_to_float(pixel_data[0]);
      libimg::set_v4(res, intensity, libimg::vec_elem::x);
      libimg::set_v4(res, intensity, libimg::vec_elem::y);
      libimg::set_v4(res, intensity, libimg::vec_elem::z);
      libimg::set_v4(res, intensity, libimg::vec_elem::w);
      return res;
    }
    case CLK_LUMINANCE: {
      const libimg::Float luminance =
          libimg::convert_half_to_float(pixel_data[0]);
      libimg::set_v4(res, luminance, libimg::vec_elem::x);
      libimg::set_v4(res, luminance, libimg::vec_elem::y);
      libimg::set_v4(res, luminance, libimg::vec_elem::z);
      libimg::set_v4(res, 1.0f, libimg::vec_elem::w);
      return res;
    }
  }
}

libimg::Float4 vec_float4_from_float(const libimg::Float *pixel_data,
                                     libimg::UInt channel_order) {
  libimg::Float4 res;
  switch (channel_order) {
    default:
      libimg::set_v4(res, 0.0f, libimg::vec_elem::x);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::w);
      return res;
    case CLK_R:  // libimg::Intentional fall-through.
    case CLK_Rx:
      libimg::set_v4(res, *pixel_data, libimg::vec_elem::x);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, 1.0f, libimg::vec_elem::w);
      return res;
    case CLK_A:
      libimg::set_v4(res, 0.0f, libimg::vec_elem::x);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, *pixel_data, libimg::vec_elem::w);
      return res;
    case CLK_RG:
    case CLK_RGx:
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::x);
      libimg::set_v4(res, pixel_data[1], libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, 1.0f, libimg::vec_elem::w);
      return res;
    case CLK_RA:
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::x);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, pixel_data[1], libimg::vec_elem::w);
      return res;
    case CLK_RGBA:
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::x);
      libimg::set_v4(res, pixel_data[1], libimg::vec_elem::y);
      libimg::set_v4(res, pixel_data[2], libimg::vec_elem::z);
      libimg::set_v4(res, pixel_data[3], libimg::vec_elem::w);
      return res;
    case CLK_INTENSITY: {
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::x);
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::y);
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::z);
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::w);
      return res;
    }
    case CLK_LUMINANCE:
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::x);
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::y);
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::z);
      libimg::set_v4(res, 1.0f, libimg::vec_elem::w);
      return res;
  }
}

template <typename PixelTy>
libimg::Float4 vec_float4_from_norm_8_16_int_types(
    const PixelTy *pixel_data, libimg::UInt channel_order,
    const libimg::Float coefficient) {
  libimg::Float4 res;
  switch (channel_order) {
    default:
      libimg::set_v4(res, 0.0f, libimg::vec_elem::x);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::w);
      return res;
    case CLK_R:  // libimg::Intentional fall-through.
    case CLK_Rx:
      libimg::set_v4(res, *pixel_data * coefficient, libimg::vec_elem::x);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, 1.0f, libimg::vec_elem::w);
      return res;
    case CLK_A:
      libimg::set_v4(res, 0.0f, libimg::vec_elem::x);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, *pixel_data * coefficient, libimg::vec_elem::w);
      return res;
    case CLK_RG:  // libimg::Intentional fall-through.
    case CLK_RGx:
      libimg::set_v4(res, pixel_data[0] * coefficient, libimg::vec_elem::x);
      libimg::set_v4(res, pixel_data[1] * coefficient, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, 1.0f, libimg::vec_elem::w);
      return res;
    case CLK_RA:
      libimg::set_v4(res, pixel_data[0] * coefficient, libimg::vec_elem::x);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::y);
      libimg::set_v4(res, 0.0f, libimg::vec_elem::z);
      libimg::set_v4(res, pixel_data[1] * coefficient, libimg::vec_elem::w);
      return res;
    case CLK_RGBA:
      libimg::set_v4(res, pixel_data[0] * coefficient, libimg::vec_elem::x);
      libimg::set_v4(res, pixel_data[1] * coefficient, libimg::vec_elem::y);
      libimg::set_v4(res, pixel_data[2] * coefficient, libimg::vec_elem::z);
      libimg::set_v4(res, pixel_data[3] * coefficient, libimg::vec_elem::w);
      return res;
    case CLK_BGRA:
      libimg::set_v4(res, pixel_data[2] * coefficient, libimg::vec_elem::x);
      libimg::set_v4(res, pixel_data[1] * coefficient, libimg::vec_elem::y);
      libimg::set_v4(res, pixel_data[0] * coefficient, libimg::vec_elem::z);
      libimg::set_v4(res, pixel_data[3] * coefficient, libimg::vec_elem::w);
      return res;
    case CLK_ARGB:
      libimg::set_v4(res, pixel_data[1] * coefficient, libimg::vec_elem::x);
      libimg::set_v4(res, pixel_data[2] * coefficient, libimg::vec_elem::y);
      libimg::set_v4(res, pixel_data[3] * coefficient, libimg::vec_elem::z);
      libimg::set_v4(res, pixel_data[0] * coefficient, libimg::vec_elem::w);
      return res;
    case CLK_INTENSITY: {
      const libimg::Float intensity =
          static_cast<libimg::Float>(pixel_data[0]) * coefficient;
      libimg::set_v4(res, intensity, libimg::vec_elem::x);
      libimg::set_v4(res, intensity, libimg::vec_elem::y);
      libimg::set_v4(res, intensity, libimg::vec_elem::z);
      libimg::set_v4(res, intensity, libimg::vec_elem::w);
      return res;
    }
    case CLK_LUMINANCE: {
      const libimg::Float luminance =
          static_cast<libimg::Float>(pixel_data[0]) * coefficient;
      libimg::set_v4(res, luminance, libimg::vec_elem::x);
      libimg::set_v4(res, luminance, libimg::vec_elem::y);
      libimg::set_v4(res, luminance, libimg::vec_elem::z);
      libimg::set_v4(res, 1.0f, libimg::vec_elem::w);
      return res;
    }
  }
}

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* libimg::Float4 helpers. */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
class float4_reader {
 public:
  inline libimg::Float4 operator()(const void *data,
                                   const libimg::UInt channel_order,
                                   const libimg::UInt channel_type) const {
    return read(data, channel_order, channel_type);
  }
  static inline libimg::Float4 read(const void *data,
                                    const libimg::UInt channel_order,
                                    const libimg::UInt channel_type) {
    switch (channel_type) {
      default:
        return libimg::make<libimg::Float4>(0.0f, 0.0f, 0.0f, 0.0f);
      case CLK_HALF_FLOAT: {
        const libimg::UShort *pixel_data =
            static_cast<const libimg::UShort *>(data);
        return vec_float4_from_half(pixel_data, channel_order);
      }
      case CLK_FLOAT: {
        const libimg::Float *pixel_data =
            static_cast<const libimg::Float *>(data);
        return vec_float4_from_float(pixel_data, channel_order);
      }
      case CLK_UNORM_INT8: {
        const libimg::UChar *pixel_data =
            static_cast<const libimg::UChar *>(data);
        const libimg::Float coefficient = 0.0039215686f;
        return vec_float4_from_norm_8_16_int_types(pixel_data, channel_order,
                                                   coefficient);
      }
      case CLK_UNORM_INT16: {
        const libimg::UShort *pixel_data =
            static_cast<const libimg::UShort *>(data);
        const libimg::Float coefficient = 0.00001525902189f;
        return vec_float4_from_norm_8_16_int_types(pixel_data, channel_order,
                                                   coefficient);
      }
      case CLK_SNORM_INT8: {
        const libimg::Char *pixel_data =
            static_cast<const libimg::Char *>(data);
        const libimg::Float coefficient = 0.0078740157f;
        libimg::Float4 ret = vec_float4_from_norm_8_16_int_types(
            pixel_data, channel_order, coefficient);
        return libimg::make<libimg::Float4>(
            libimg::max(
                -1.0f, libimg::get_v4<libimg::Float>(ret, libimg::vec_elem::x)),
            libimg::max(
                -1.0f, libimg::get_v4<libimg::Float>(ret, libimg::vec_elem::y)),
            libimg::max(
                -1.0f, libimg::get_v4<libimg::Float>(ret, libimg::vec_elem::z)),
            libimg::max(-1.0f, libimg::get_v4<libimg::Float>(
                                   ret, libimg::vec_elem::w)));
      }
      case CLK_SNORM_INT16: {
        const libimg::Short *pixel_data =
            static_cast<const libimg::Short *>(data);
        const libimg::Float coefficient = 0.0000305185f;
        libimg::Float4 ret = vec_float4_from_norm_8_16_int_types(
            pixel_data, channel_order, coefficient);
        return libimg::make<libimg::Float4>(
            libimg::max(
                -1.0f, libimg::get_v4<libimg::Float>(ret, libimg::vec_elem::x)),
            libimg::max(
                -1.0f, libimg::get_v4<libimg::Float>(ret, libimg::vec_elem::y)),
            libimg::max(
                -1.0f, libimg::get_v4<libimg::Float>(ret, libimg::vec_elem::z)),
            libimg::max(-1.0f, libimg::get_v4<libimg::Float>(
                                   ret, libimg::vec_elem::w)));
        return ret;
      }
      case CLK_UNORM_SHORT_555:
        return vec_float4_from_unorm_555(
            static_cast<const libimg::UShort *>(data), channel_order);
      case CLK_UNORM_SHORT_565: {
        return vec_float4_from_unorm_565(
            static_cast<const libimg::UShort *>(data), channel_order);
      }
      case CLK_UNORM_INT_101010:
        return vec_float4_from_unorm_101010(
            static_cast<const libimg::UInt *>(data), channel_order);
    }
  }
};

class float4_writer {
 public:
  static inline void write(void *data, const libimg::Float4 &color,
                           const libimg::UInt channel_order,
                           const libimg::UInt channel_type) {
    switch (channel_type) {
      default:
        return;
      case CLK_FLOAT: {
        switch (channel_order) {
          default:
            return;
          case CLK_R:          // libimg::Intentional fall-through.
          case CLK_Rx:         // libimg::Intentional fall-through.
          case CLK_INTENSITY:  // libimg::Intentional fall-through.
          case CLK_LUMINANCE: {
            libimg::Float *pixel_data = reinterpret_cast<libimg::Float *>(data);
            *pixel_data =
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::x);
            return;
          }
          case CLK_A: {
            libimg::Float *pixel_data = reinterpret_cast<libimg::Float *>(data);
            *pixel_data =
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::w);
            return;
          }
          case CLK_RA: {
            libimg::Float2 *pixel_data =
                reinterpret_cast<libimg::Float2 *>(data);
            libimg::set_v2(
                *pixel_data,
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::x),
                libimg::vec_elem::x);
            libimg::set_v2(
                *pixel_data,
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::w),
                libimg::vec_elem::y);
            return;
          }
          case CLK_RG:
          case CLK_RGx: {  // libimg::Intentional fall-through.
            libimg::Float2 *pixel_data =
                reinterpret_cast<libimg::Float2 *>(data);
            libimg::set_v2(
                *pixel_data,
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::x),
                libimg::vec_elem::x);
            libimg::set_v2(
                *pixel_data,
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::y),
                libimg::vec_elem::y);
            return;
          }
          case CLK_RGBA: {
            libimg::Float4 *pixel_data =
                reinterpret_cast<libimg::Float4 *>(data);
            libimg::set_v4(
                *pixel_data,
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::x),
                libimg::vec_elem::x);
            libimg::set_v4(
                *pixel_data,
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::y),
                libimg::vec_elem::y);
            libimg::set_v4(
                *pixel_data,
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::z),
                libimg::vec_elem::z);
            libimg::set_v4(
                *pixel_data,
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::w),
                libimg::vec_elem::w);
            return;
          }
        }
      }
      case CLK_HALF_FLOAT: {
        switch (channel_order) {
          default:
            return;
          case CLK_R:          // libimg::Intentional fall-through.
          case CLK_Rx:         // libimg::Intentional fall-through.
          case CLK_INTENSITY:  // libimg::Intentional fall-through.
          case CLK_LUMINANCE: {
            libimg::UShort *pixel_data =
                reinterpret_cast<libimg::UShort *>(data);
            libimg::Float elem_x =
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::x);
            *pixel_data = libimg::convert_float_to_half(elem_x);
            return;
          }
          case CLK_A: {
            libimg::UShort *pixel_data =
                reinterpret_cast<libimg::UShort *>(data);
            libimg::Float elem_x =
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::w);
            *pixel_data = libimg::convert_float_to_half(elem_x);
            return;
          }
          case CLK_RA: {
            libimg::UShort2 *pixel_data =
                reinterpret_cast<libimg::UShort2 *>(data);
            libimg::Float elem_x =
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::x);
            libimg::set_v2(*pixel_data, libimg::convert_float_to_half(elem_x),
                           libimg::vec_elem::x);
            libimg::Float elem_y =
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::w);
            libimg::set_v2(*pixel_data, libimg::convert_float_to_half(elem_y),
                           libimg::vec_elem::y);
            return;
          }
          case CLK_RG:
          case CLK_RGx: {  // libimg::Intentional fall-through.
            libimg::UShort2 *pixel_data =
                reinterpret_cast<libimg::UShort2 *>(data);
            libimg::Float elem_x =
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::x);
            libimg::set_v2(*pixel_data, libimg::convert_float_to_half(elem_x),
                           libimg::vec_elem::x);
            libimg::Float elem_y =
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::y);
            libimg::set_v2(*pixel_data, libimg::convert_float_to_half(elem_y),
                           libimg::vec_elem::y);
            return;
          }
          case CLK_RGBA: {
            libimg::UShort4 *pixel_data =
                reinterpret_cast<libimg::UShort4 *>(data);
            libimg::Float elem_x =
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::x);
            libimg::set_v4(*pixel_data, libimg::convert_float_to_half(elem_x),
                           libimg::vec_elem::x);
            libimg::Float elem_y =
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::y);
            libimg::set_v4(*pixel_data, libimg::convert_float_to_half(elem_y),
                           libimg::vec_elem::y);
            libimg::Float elem_z =
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::z);
            libimg::set_v4(*pixel_data, libimg::convert_float_to_half(elem_z),
                           libimg::vec_elem::z);
            libimg::Float elem_w =
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::w);
            libimg::set_v4(*pixel_data, libimg::convert_float_to_half(elem_w),
                           libimg::vec_elem::w);
            return;
          }
        }
      }
      case CLK_SNORM_INT8: {
        switch (channel_order) {
          default:
            return;
          case CLK_R:          // libimg::Intentional fall-through.
          case CLK_Rx:         // libimg::Intentional fall-through.
          case CLK_INTENSITY:  // libimg::Intentional fall-through.
          case CLK_LUMINANCE: {
            libimg::Char *pixel_data = reinterpret_cast<libimg::Char *>(data);
            *pixel_data = snorm_char_from_float(
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::x));
            return;
          }
          case CLK_A: {
            libimg::Char *pixel_data = reinterpret_cast<libimg::Char *>(data);
            *pixel_data = snorm_char_from_float(
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::w));
            return;
          }
          case CLK_RG:
          case CLK_RGx: {  // libimg::Intentional fall-through;
            libimg::Char2 *pixel_data = reinterpret_cast<libimg::Char2 *>(data);
            libimg::set_v2(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::x);
            libimg::set_v2(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::y);
            return;
          }
          case CLK_RA: {
            libimg::Char2 *pixel_data = reinterpret_cast<libimg::Char2 *>(data);
            libimg::set_v2(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::x);
            libimg::set_v2(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::y);
            return;
          }
          case CLK_RGBA: {
            libimg::Char4 *pixel_data = reinterpret_cast<libimg::Char4 *>(data);
            libimg::set_v4(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::x);
            libimg::set_v4(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::y);
            libimg::set_v4(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::z)),
                           libimg::vec_elem::z);
            libimg::set_v4(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::w);
            return;
          }
          case CLK_ARGB: {
            libimg::Char4 *pixel_data = reinterpret_cast<libimg::Char4 *>(data);
            libimg::set_v4(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::x);
            libimg::set_v4(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::y);
            libimg::set_v4(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::z);
            libimg::set_v4(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::z)),
                           libimg::vec_elem::w);
            return;
          }
          case CLK_BGRA: {
            libimg::Char4 *pixel_data = reinterpret_cast<libimg::Char4 *>(data);
            libimg::set_v4(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::z)),
                           libimg::vec_elem::x);
            libimg::set_v4(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::y);
            libimg::set_v4(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::z);
            libimg::set_v4(*pixel_data,
                           snorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::w);
            return;
          }
        }
      }
      case CLK_SNORM_INT16: {
        switch (channel_order) {
          default:
            return;
          case CLK_R:          // libimg::Intentional fall-through.
          case CLK_Rx:         // libimg::Intentional fall-through.
          case CLK_INTENSITY:  // libimg::Intentional fall-through.
          case CLK_LUMINANCE: {
            libimg::Short *pixel_data = reinterpret_cast<libimg::Short *>(data);
            *pixel_data = snorm_short_from_float(
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::x));
            return;
          }
          case CLK_A: {
            libimg::Short *pixel_data = reinterpret_cast<libimg::Short *>(data);
            *pixel_data = snorm_short_from_float(
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::w));
            return;
          }
          case CLK_RG:
          case CLK_RGx: {  // libimg::Intentional fall-through;
            libimg::Short2 *pixel_data =
                reinterpret_cast<libimg::Short2 *>(data);
            libimg::set_v2(*pixel_data,
                           snorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::x);
            libimg::set_v2(*pixel_data,
                           snorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::y);
            return;
          }
          case CLK_RA: {
            libimg::Short2 *pixel_data =
                reinterpret_cast<libimg::Short2 *>(data);
            libimg::set_v2(*pixel_data,
                           snorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::x);
            libimg::set_v2(*pixel_data,
                           snorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::y);
            return;
          }
          case CLK_RGBA: {
            libimg::Short4 *pixel_data =
                reinterpret_cast<libimg::Short4 *>(data);
            libimg::set_v4(*pixel_data,
                           snorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::x);
            libimg::set_v4(*pixel_data,
                           snorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::y);
            libimg::set_v4(*pixel_data,
                           snorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::z)),
                           libimg::vec_elem::z);
            libimg::set_v4(*pixel_data,
                           snorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::w);
            return;
          }
        }
      }
      case CLK_UNORM_INT8: {
        switch (channel_order) {
          default:
            return;
          case CLK_R:          // libimg::Intentional fall-through.
          case CLK_Rx:         // libimg::Intentional fall-through.
          case CLK_INTENSITY:  // libimg::Intentional fall-through.
          case CLK_LUMINANCE: {
            libimg::Char *pixel_data = reinterpret_cast<libimg::Char *>(data);
            *pixel_data = unorm_char_from_float(
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::x));
            return;
          }
          case CLK_A: {
            libimg::Char *pixel_data = reinterpret_cast<libimg::Char *>(data);
            *pixel_data = unorm_char_from_float(
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::w));
            return;
          }
          case CLK_RG:
          case CLK_RGx: {  // libimg::Intentional fall-through;
            libimg::Char2 *pixel_data = reinterpret_cast<libimg::Char2 *>(data);
            libimg::set_v2(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::x);
            libimg::set_v2(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::y);
            return;
          }
          case CLK_RA: {
            libimg::Char2 *pixel_data = reinterpret_cast<libimg::Char2 *>(data);
            libimg::set_v2(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::x);
            libimg::set_v2(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::y);
            return;
          }
          case CLK_RGBA: {
            libimg::Char4 *pixel_data = reinterpret_cast<libimg::Char4 *>(data);
            libimg::set_v4(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::x);
            libimg::set_v4(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::y);
            libimg::set_v4(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::z)),
                           libimg::vec_elem::z);
            libimg::set_v4(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::w);
            return;
          }
          case CLK_ARGB: {
            libimg::Char4 *pixel_data = reinterpret_cast<libimg::Char4 *>(data);
            libimg::set_v4(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::x);
            libimg::set_v4(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::y);
            libimg::set_v4(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::z);
            libimg::set_v4(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::z)),
                           libimg::vec_elem::w);
            return;
          }
          case CLK_BGRA: {
            libimg::Char4 *pixel_data = reinterpret_cast<libimg::Char4 *>(data);
            libimg::set_v4(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::z)),
                           libimg::vec_elem::x);
            libimg::set_v4(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::y);
            libimg::set_v4(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::z);
            libimg::set_v4(*pixel_data,
                           unorm_char_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::w);
            return;
          }
        }
      }
      case CLK_UNORM_INT16: {
        switch (channel_order) {
          default:
            return;
          case CLK_R:          // libimg::Intentional fall-through.
          case CLK_Rx:         // libimg::Intentional fall-through.
          case CLK_INTENSITY:  // libimg::Intentional fall-through.
          case CLK_LUMINANCE: {
            libimg::Short *pixel_data = reinterpret_cast<libimg::Short *>(data);
            *pixel_data = unorm_short_from_float(
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::x));
            return;
          }
          case CLK_A: {
            libimg::Short *pixel_data = reinterpret_cast<libimg::Short *>(data);
            *pixel_data = unorm_short_from_float(
                libimg::get_v4<libimg::Float>(color, libimg::vec_elem::w));
            return;
          }
          case CLK_RG:
          case CLK_RGx: {  // libimg::Intentional fall-through;
            libimg::Short2 *pixel_data =
                reinterpret_cast<libimg::Short2 *>(data);
            libimg::set_v2(*pixel_data,
                           unorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::x);
            libimg::set_v2(*pixel_data,
                           unorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::y);
            return;
          }
          case CLK_RA: {
            libimg::Short2 *pixel_data =
                reinterpret_cast<libimg::Short2 *>(data);
            libimg::set_v2(*pixel_data,
                           unorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::x);
            libimg::set_v2(*pixel_data,
                           unorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::y);
            return;
          }
          case CLK_RGBA: {
            libimg::Short4 *pixel_data =
                reinterpret_cast<libimg::Short4 *>(data);
            libimg::set_v4(*pixel_data,
                           unorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::x);
            libimg::set_v4(*pixel_data,
                           unorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::y);
            libimg::set_v4(*pixel_data,
                           unorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::z)),
                           libimg::vec_elem::z);
            libimg::set_v4(*pixel_data,
                           unorm_short_from_float(libimg::get_v4<libimg::Float>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::w);
            return;
          }
        }
      }
      case CLK_UNORM_SHORT_555: {
        libimg::UShort *pixel_data = reinterpret_cast<libimg::UShort *>(data);
        switch (channel_order) {
          default:
            return;
          case CLK_RGB:
          case CLK_RGBx: {
            *pixel_data = unorm_5_from_float(libimg::get_v4<libimg::Float>(
                              color, libimg::vec_elem::x))
                              << 10 |
                          unorm_5_from_float(libimg::get_v4<libimg::Float>(
                              color, libimg::vec_elem::y))
                              << 5 |
                          unorm_5_from_float(libimg::get_v4<libimg::Float>(
                              color, libimg::vec_elem::z));
            return;
          }
        }
      }
      case CLK_UNORM_SHORT_565: {
        libimg::UShort *pixel_data = reinterpret_cast<libimg::UShort *>(data);
        switch (channel_order) {
          default:
            return;
          case CLK_RGB:
          case CLK_RGBx: {
            *pixel_data = unorm_5_from_float(libimg::get_v4<libimg::Float>(
                              color, libimg::vec_elem::x))
                              << 11 |
                          unorm_6_from_float(libimg::get_v4<libimg::Float>(
                              color, libimg::vec_elem::y))
                              << 5 |
                          unorm_5_from_float(libimg::get_v4<libimg::Float>(
                              color, libimg::vec_elem::z));
            return;
          }
        }
      }
      case CLK_UNORM_INT_101010: {
        libimg::UInt *pixel_data = reinterpret_cast<libimg::UInt *>(data);
        switch (channel_order) {
          default:
            return;
          case CLK_RGB:
          case CLK_RGBx: {
            *pixel_data = unorm_int_10_from_float(libimg::get_v4<libimg::Float>(
                              color, libimg::vec_elem::x))
                              << 20 |
                          unorm_int_10_from_float(libimg::get_v4<libimg::Float>(
                              color, libimg::vec_elem::y))
                              << 10 |
                          unorm_int_10_from_float(libimg::get_v4<libimg::Float>(
                              color, libimg::vec_elem::z));
            return;
          }
        }
      }
    }
  }
};

class int4_writer {
 public:
  static inline void write(void *data, const libimg::Int4 &color,
                           const libimg::UInt channel_order,
                           const libimg::UInt channel_type) {
    switch (channel_type) {
      default:
        return;
      case CLK_SIGNED_INT8: {
        switch (channel_order) {
          default:
            return;
          case CLK_R:
          case CLK_Rx: {  // libimg::Intentional fall-through.
            libimg::Char *pixel_data = reinterpret_cast<libimg::Char *>(data);
            *pixel_data = libimg::convert_char_sat(
                libimg::get_v4<libimg::Int>(color, libimg::vec_elem::x));
            return;
          }
          case CLK_A: {
            libimg::Char *pixel_data = reinterpret_cast<libimg::Char *>(data);
            *pixel_data = libimg::convert_char_sat(
                libimg::get_v4<libimg::Int>(color, libimg::vec_elem::w));
            return;
          }
          case CLK_RG:
          case CLK_RGx: {  // libimg::Intentional fall-through.
            libimg::Char2 *pixel_data = reinterpret_cast<libimg::Char2 *>(data);
            libimg::set_v2(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::x);
            libimg::set_v2(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::y);
            return;
          }
          case CLK_RA: {
            libimg::Char2 *pixel_data = reinterpret_cast<libimg::Char2 *>(data);
            libimg::set_v2(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::x);
            libimg::set_v2(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::y);
            return;
          }
          case CLK_RGBA: {
            libimg::Char4 *pixel_data = reinterpret_cast<libimg::Char4 *>(data);
            libimg::set_v4(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::x);
            libimg::set_v4(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::y);
            libimg::set_v4(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::z)),
                           libimg::vec_elem::z);
            libimg::set_v4(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::w);
            return;
          }
          case CLK_ARGB: {
            libimg::Char4 *pixel_data = reinterpret_cast<libimg::Char4 *>(data);
            libimg::set_v4(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::x);
            libimg::set_v4(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::y);
            libimg::set_v4(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::z);
            libimg::set_v4(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::z)),
                           libimg::vec_elem::w);
            return;
          }
          case CLK_BGRA: {
            libimg::Char4 *pixel_data = reinterpret_cast<libimg::Char4 *>(data);
            libimg::set_v4(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::z)),
                           libimg::vec_elem::x);
            libimg::set_v4(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::y)),
                           libimg::vec_elem::y);
            libimg::set_v4(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::x)),
                           libimg::vec_elem::z);
            libimg::set_v4(*pixel_data,
                           libimg::convert_char_sat(libimg::get_v4<libimg::Int>(
                               color, libimg::vec_elem::w)),
                           libimg::vec_elem::w);
            return;
          }
        }
        return;
      }
      case CLK_SIGNED_INT16: {
        switch (channel_order) {
          default:
            return;
          case CLK_R:
          case CLK_Rx: {  // libimg::Intentional fall-through.
            libimg::Short *pixel_data = reinterpret_cast<libimg::Short *>(data);
            *pixel_data = libimg::convert_short_sat(
                libimg::get_v4<libimg::Int>(color, libimg::vec_elem::x));
            return;
          }
          case CLK_A: {
            libimg::Short *pixel_data = reinterpret_cast<libimg::Short *>(data);
            *pixel_data = libimg::convert_short_sat(
                libimg::get_v4<libimg::Int>(color, libimg::vec_elem::w));
            return;
          }
          case CLK_RG:
          case CLK_RGx: {  // libimg::Intentional fall-through.
            libimg::Short2 *pixel_data =
                reinterpret_cast<libimg::Short2 *>(data);
            libimg::set_v2(
                *pixel_data,
                libimg::convert_short_sat(
                    libimg::get_v4<libimg::Int>(color, libimg::vec_elem::x)),
                libimg::vec_elem::x);
            libimg::set_v2(
                *pixel_data,
                libimg::convert_short_sat(
                    libimg::get_v4<libimg::Int>(color, libimg::vec_elem::y)),
                libimg::vec_elem::y);
            return;
          }
          case CLK_RA: {
            libimg::Short2 *pixel_data =
                reinterpret_cast<libimg::Short2 *>(data);
            libimg::set_v2(
                *pixel_data,
                libimg::convert_short_sat(
                    libimg::get_v4<libimg::Int>(color, libimg::vec_elem::x)),
                libimg::vec_elem::x);
            libimg::set_v2(
                *pixel_data,
                libimg::convert_short_sat(
                    libimg::get_v4<libimg::Int>(color, libimg::vec_elem::w)),
                libimg::vec_elem::y);
            return;
          }
          case CLK_RGBA: {
            libimg::Short4 *pixel_data =
                reinterpret_cast<libimg::Short4 *>(data);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_short_sat(
                    libimg::get_v4<libimg::Int>(color, libimg::vec_elem::x)),
                libimg::vec_elem::x);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_short_sat(
                    libimg::get_v4<libimg::Int>(color, libimg::vec_elem::y)),
                libimg::vec_elem::y);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_short_sat(
                    libimg::get_v4<libimg::Int>(color, libimg::vec_elem::z)),
                libimg::vec_elem::z);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_short_sat(
                    libimg::get_v4<libimg::Int>(color, libimg::vec_elem::w)),
                libimg::vec_elem::w);
            return;
          }
        }
      }
      case CLK_SIGNED_INT32: {
        switch (channel_order) {
          default:
            return;
          case CLK_R:
          case CLK_Rx: {  // libimg::Intentional fall-through.
            libimg::Int *pixel_data = reinterpret_cast<libimg::Int *>(data);
            *pixel_data =
                libimg::get_v4<libimg::Int>(color, libimg::vec_elem::x);
            return;
          }
          case CLK_A: {
            libimg::Int *pixel_data = reinterpret_cast<libimg::Int *>(data);
            *pixel_data =
                libimg::get_v4<libimg::Int>(color, libimg::vec_elem::w);
            return;
          }
          case CLK_RG:
          case CLK_RGx: {  // libimg::Intentional fall-through.
            libimg::Int2 *pixel_data = reinterpret_cast<libimg::Int2 *>(data);
            libimg::set_v2(
                *pixel_data,
                libimg::get_v4<libimg::Int>(color, libimg::vec_elem::x),
                libimg::vec_elem::x);
            libimg::set_v2(
                *pixel_data,
                libimg::get_v4<libimg::Int>(color, libimg::vec_elem::y),
                libimg::vec_elem::y);
            return;
          }
          case CLK_RA: {
            libimg::Int2 *pixel_data = reinterpret_cast<libimg::Int2 *>(data);
            libimg::set_v2(
                *pixel_data,
                libimg::get_v4<libimg::Int>(color, libimg::vec_elem::x),
                libimg::vec_elem::x);
            libimg::set_v2(
                *pixel_data,
                libimg::get_v4<libimg::Int>(color, libimg::vec_elem::w),
                libimg::vec_elem::y);
            return;
          }
          case CLK_RGBA: {
            libimg::Int4 *pixel_data = reinterpret_cast<libimg::Int4 *>(data);
            libimg::set_v4(
                *pixel_data,
                libimg::get_v4<libimg::Int>(color, libimg::vec_elem::x),
                libimg::vec_elem::x);
            libimg::set_v4(
                *pixel_data,
                libimg::get_v4<libimg::Int>(color, libimg::vec_elem::y),
                libimg::vec_elem::y);
            libimg::set_v4(
                *pixel_data,
                libimg::get_v4<libimg::Int>(color, libimg::vec_elem::z),
                libimg::vec_elem::z);
            libimg::set_v4(
                *pixel_data,
                libimg::get_v4<libimg::Int>(color, libimg::vec_elem::w),
                libimg::vec_elem::w);
            return;
          }
        }
      }
    }
  }
};

class uint4_writer {
 public:
  static inline void write(void *data, const libimg::UInt4 &color,
                           const libimg::UInt channel_order,
                           const libimg::UInt channel_type) {
    switch (channel_type) {
      default:
        return;
      case CLK_UNSIGNED_INT8: {
        switch (channel_order) {
          default:
            return;
          case CLK_R:
          case CLK_Rx: {  // libimg::Intentional fall-through.
            libimg::UChar *pixel_data = reinterpret_cast<libimg::UChar *>(data);
            *pixel_data = libimg::convert_uchar_sat(
                libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::x));
            return;
          }
          case CLK_A: {
            libimg::UChar *pixel_data = reinterpret_cast<libimg::UChar *>(data);
            *pixel_data = libimg::convert_uchar_sat(
                libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::w));
            return;
          }
          case CLK_RG:
          case CLK_RGx: {  // libimg::Intentional fall-through.
            libimg::UChar2 *pixel_data =
                reinterpret_cast<libimg::UChar2 *>(data);
            libimg::set_v2(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::x)),
                libimg::vec_elem::x);
            libimg::set_v2(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::y)),
                libimg::vec_elem::y);
            return;
          }
          case CLK_RA: {
            libimg::UChar2 *pixel_data =
                reinterpret_cast<libimg::UChar2 *>(data);
            libimg::set_v2(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::x)),
                libimg::vec_elem::x);
            libimg::set_v2(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::w)),
                libimg::vec_elem::y);
            return;
          }
          case CLK_RGBA: {
            libimg::UChar4 *pixel_data =
                reinterpret_cast<libimg::UChar4 *>(data);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::x)),
                libimg::vec_elem::x);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::y)),
                libimg::vec_elem::y);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::z)),
                libimg::vec_elem::z);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::w)),
                libimg::vec_elem::w);
            return;
          }
          case CLK_ARGB: {
            libimg::UChar4 *pixel_data =
                reinterpret_cast<libimg::UChar4 *>(data);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::w)),
                libimg::vec_elem::x);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::x)),
                libimg::vec_elem::y);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::y)),
                libimg::vec_elem::z);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::z)),
                libimg::vec_elem::w);
            return;
          }
          case CLK_BGRA: {
            libimg::UChar4 *pixel_data =
                reinterpret_cast<libimg::UChar4 *>(data);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::z)),
                libimg::vec_elem::x);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::y)),
                libimg::vec_elem::y);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::x)),
                libimg::vec_elem::z);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_uchar_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::w)),
                libimg::vec_elem::w);
            return;
          }
        }
        return;
      }
      case CLK_UNSIGNED_INT16: {
        switch (channel_order) {
          default:
            return;
          case CLK_R:
          case CLK_Rx: {  // libimg::Intentional fall-through.
            libimg::UShort *pixel_data =
                reinterpret_cast<libimg::UShort *>(data);
            *pixel_data = libimg::convert_ushort_sat(
                libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::x));
            return;
          }
          case CLK_A: {
            libimg::UShort *pixel_data =
                reinterpret_cast<libimg::UShort *>(data);
            *pixel_data = libimg::convert_ushort_sat(
                libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::w));
            return;
          }
          case CLK_RG:
          case CLK_RGx: {  // libimg::Intentional fall-through.
            libimg::UShort2 *pixel_data =
                reinterpret_cast<libimg::UShort2 *>(data);
            libimg::set_v2(
                *pixel_data,
                libimg::convert_ushort_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::x)),
                libimg::vec_elem::x);
            libimg::set_v2(
                *pixel_data,
                libimg::convert_ushort_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::y)),
                libimg::vec_elem::y);
            return;
          }
          case CLK_RA: {
            libimg::UShort2 *pixel_data =
                reinterpret_cast<libimg::UShort2 *>(data);
            libimg::set_v2(
                *pixel_data,
                libimg::convert_ushort_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::x)),
                libimg::vec_elem::x);
            libimg::set_v2(
                *pixel_data,
                libimg::convert_ushort_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::w)),
                libimg::vec_elem::y);
            return;
          }
          case CLK_RGBA: {
            libimg::UShort4 *pixel_data =
                reinterpret_cast<libimg::UShort4 *>(data);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_ushort_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::x)),
                libimg::vec_elem::x);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_ushort_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::y)),
                libimg::vec_elem::y);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_ushort_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::z)),
                libimg::vec_elem::z);
            libimg::set_v4(
                *pixel_data,
                libimg::convert_ushort_sat(
                    libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::w)),
                libimg::vec_elem::w);
            return;
          }
        }
      }
      case CLK_UNSIGNED_INT32: {
        switch (channel_order) {
          default:
            return;
          case CLK_R:
          case CLK_Rx: {  // libimg::Intentional fall-through.
            libimg::UInt *pixel_data = reinterpret_cast<libimg::UInt *>(data);
            *pixel_data =
                libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::x);
            return;
          }
          case CLK_A: {
            libimg::UInt *pixel_data = reinterpret_cast<libimg::UInt *>(data);
            *pixel_data =
                libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::w);
            return;
          }
          case CLK_RG:
          case CLK_RGx: {  // libimg::Intentional fall-through.
            libimg::UInt2 *pixel_data = reinterpret_cast<libimg::UInt2 *>(data);
            libimg::set_v2(
                *pixel_data,
                libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::x),
                libimg::vec_elem::x);
            libimg::set_v2(
                *pixel_data,
                libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::y),
                libimg::vec_elem::y);
            return;
          }
          case CLK_RA: {
            libimg::UInt2 *pixel_data = reinterpret_cast<libimg::UInt2 *>(data);
            libimg::set_v2(
                *pixel_data,
                libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::x),
                libimg::vec_elem::x);
            libimg::set_v2(
                *pixel_data,
                libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::w),
                libimg::vec_elem::y);
            return;
          }
          case CLK_RGBA: {
            libimg::UInt4 *pixel_data = reinterpret_cast<libimg::UInt4 *>(data);
            libimg::set_v4(
                *pixel_data,
                libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::x),
                libimg::vec_elem::x);
            libimg::set_v4(
                *pixel_data,
                libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::y),
                libimg::vec_elem::y);
            libimg::set_v4(
                *pixel_data,
                libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::z),
                libimg::vec_elem::z);
            libimg::set_v4(
                *pixel_data,
                libimg::get_v4<libimg::UInt>(color, libimg::vec_elem::w),
                libimg::vec_elem::w);
            return;
          }
        }
      }
    }
  }
};

template <typename VecTy, typename ElemTy>
inline VecTy vec_int4_from_int16_32_signed_unsigned(
    const ElemTy *pixel_data, libimg::UInt channel_order) {
  VecTy res;
  switch (channel_order) {
    default:
      libimg::set_v4(res, 0, libimg::vec_elem::x);
      libimg::set_v4(res, 0, libimg::vec_elem::y);
      libimg::set_v4(res, 0, libimg::vec_elem::z);
      libimg::set_v4(res, 0, libimg::vec_elem::w);
      return res;
    case CLK_R:  // libimg::Intentional fall-through
    case CLK_Rx: {
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::x);
      libimg::set_v4(res, 0, libimg::vec_elem::y);
      libimg::set_v4(res, 0, libimg::vec_elem::z);
      libimg::set_v4(res, 1, libimg::vec_elem::w);
      return res;
    }
    case CLK_A: {
      libimg::set_v4(res, 0, libimg::vec_elem::x);
      libimg::set_v4(res, 0, libimg::vec_elem::y);
      libimg::set_v4(res, 0, libimg::vec_elem::z);
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::w);
      return res;
    }
    case CLK_RG:  // libimg::Intentional fall-through
    case CLK_RGx: {
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::x);
      libimg::set_v4(res, pixel_data[1], libimg::vec_elem::y);
      libimg::set_v4(res, 0, libimg::vec_elem::z);
      libimg::set_v4(res, 1, libimg::vec_elem::w);
      return res;
    }
    case CLK_RA: {
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::x);
      libimg::set_v4(res, 0, libimg::vec_elem::y);
      libimg::set_v4(res, 0, libimg::vec_elem::z);
      libimg::set_v4(res, pixel_data[1], libimg::vec_elem::w);
      return res;
    }
    case CLK_RGBA: {
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::x);
      libimg::set_v4(res, pixel_data[1], libimg::vec_elem::y);
      libimg::set_v4(res, pixel_data[2], libimg::vec_elem::z);
      libimg::set_v4(res, pixel_data[3], libimg::vec_elem::w);
      return res;
    }
  }
}

template <typename VecTy, typename ElemTy>
inline VecTy vec_int4_from_int8_signed_unsigned(const ElemTy *pixel_data,
                                                libimg::UInt channel_order) {
  VecTy res;
  switch (channel_order) {
    default:
      libimg::set_v4(res, 0, libimg::vec_elem::x);
      libimg::set_v4(res, 0, libimg::vec_elem::y);
      libimg::set_v4(res, 0, libimg::vec_elem::z);
      libimg::set_v4(res, 0, libimg::vec_elem::w);
      return res;
    case CLK_R:  // libimg::Intentional fall-through
    case CLK_Rx: {
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::x);
      libimg::set_v4(res, 0, libimg::vec_elem::y);
      libimg::set_v4(res, 0, libimg::vec_elem::z);
      libimg::set_v4(res, 1, libimg::vec_elem::w);
      return res;
    }
    case CLK_A: {
      libimg::set_v4(res, 0, libimg::vec_elem::x);
      libimg::set_v4(res, 0, libimg::vec_elem::y);
      libimg::set_v4(res, 0, libimg::vec_elem::z);
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::w);
      return res;
    }
    case CLK_RG:  // libimg::Intentional fall-through
    case CLK_RGx: {
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::x);
      libimg::set_v4(res, pixel_data[1], libimg::vec_elem::y);
      libimg::set_v4(res, 0, libimg::vec_elem::z);
      libimg::set_v4(res, 1, libimg::vec_elem::w);
      return res;
    }
    case CLK_RA: {
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::x);
      libimg::set_v4(res, 0, libimg::vec_elem::y);
      libimg::set_v4(res, 0, libimg::vec_elem::z);
      libimg::set_v4(res, pixel_data[1], libimg::vec_elem::w);
      return res;
    }
    case CLK_RGBA: {
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::x);
      libimg::set_v4(res, pixel_data[1], libimg::vec_elem::y);
      libimg::set_v4(res, pixel_data[2], libimg::vec_elem::z);
      libimg::set_v4(res, pixel_data[3], libimg::vec_elem::w);
      return res;
    }
    case CLK_BGRA: {
      libimg::set_v4(res, pixel_data[2], libimg::vec_elem::x);
      libimg::set_v4(res, pixel_data[1], libimg::vec_elem::y);
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::z);
      libimg::set_v4(res, pixel_data[3], libimg::vec_elem::w);
      return res;
    }
    case CLK_ARGB: {
      libimg::set_v4(res, pixel_data[1], libimg::vec_elem::x);
      libimg::set_v4(res, pixel_data[2], libimg::vec_elem::y);
      libimg::set_v4(res, pixel_data[3], libimg::vec_elem::z);
      libimg::set_v4(res, pixel_data[0], libimg::vec_elem::w);
      return res;
    }
  }
}

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* libimg::Int4 / libimg::UInt4 helpers. */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
struct int4_reader {
  inline libimg::Int4 operator()(const void *data,
                                 const libimg::UInt channel_order,
                                 const libimg::UInt channel_type) const {
    return read(data, channel_order, channel_type);
  }
  static inline libimg::Int4 read(const void *data,
                                  const libimg::UInt channel_order,
                                  const libimg::UInt channel_type) {
    switch (channel_type) {
      default:
        return libimg::make<libimg::Int4>(0, 0, 0, 0);
      case CLK_SIGNED_INT8: {
        const libimg::Char *pixel_data =
            static_cast<const libimg::Char *>(data);
        return vec_int4_from_int8_signed_unsigned<libimg::Int4, libimg::Char>(
            pixel_data, channel_order);
      }
      case CLK_SIGNED_INT16: {
        const libimg::Short *pixel_data =
            static_cast<const libimg::Short *>(data);
        return vec_int4_from_int16_32_signed_unsigned<libimg::Int4,
                                                      libimg::Short>(
            pixel_data, channel_order);
      }
      case CLK_SIGNED_INT32: {
        const libimg::Int *pixel_data = static_cast<const libimg::Int *>(data);
        return vec_int4_from_int16_32_signed_unsigned<libimg::Int4,
                                                      libimg::Int>(
            pixel_data, channel_order);
      }
    }
  }
};

struct uint4_reader {
  inline libimg::UInt4 operator()(const void *data,
                                  const libimg::UInt channel_order,
                                  const libimg::UInt channel_type) const {
    return read(data, channel_order, channel_type);
  }
  static inline libimg::UInt4 read(const void *data,
                                   const libimg::UInt channel_order,
                                   const libimg::UInt channel_type) {
    switch (channel_type) {
      default:
        return libimg::make<libimg::UInt4>(0u, 0u, 0u, 0u);
      case CLK_UNSIGNED_INT8: {
        const libimg::UChar *pixel_data =
            static_cast<const libimg::UChar *>(data);
        return vec_int4_from_int8_signed_unsigned<libimg::UInt4, libimg::UChar>(
            pixel_data, channel_order);
      }
      case CLK_UNSIGNED_INT16: {
        const libimg::UShort *pixel_data =
            static_cast<const libimg::UShort *>(data);
        return vec_int4_from_int16_32_signed_unsigned<libimg::UInt4,
                                                      libimg::UShort>(
            pixel_data, channel_order);
      }
      case CLK_UNSIGNED_INT32: {
        const libimg::UInt *pixel_data =
            static_cast<const libimg::UInt *>(data);
        return vec_int4_from_int16_32_signed_unsigned<libimg::UInt4,
                                                      libimg::UInt>(
            pixel_data, channel_order);
      }
    }
  }
};

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Address mode helpers.                                                     */
/* Takes an libimg::Int (libimg::floor'ed libimg::Float). */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
inline libimg::Int addressing_mode_CLAMP_TO_EDGE(libimg::Int coord,
                                                 libimg::Int size) {
  return libimg::clamp(coord, 0, size - 1);
}

inline libimg::Int addressing_mode_CLAMP(libimg::Int coord, libimg::Int size) {
  return libimg::clamp(coord, -1, size);
}

inline libimg::Int addressing_mode_NONE(libimg::Int coord) { return coord; }

template <typename VecTy>
VecTy border_color(const libimg::UInt channel_order) {
  switch (channel_order) {
    case CLK_R:
    case CLK_RG:
    case CLK_RGB:
    case CLK_LUMINANCE:
      return libimg::make<VecTy>(0, 0, 0, 1);
    default:
      break;
  }
  return libimg::make<VecTy>(0, 0, 0, 0);
}

template <typename VecTy, typename VecElemTy, typename VecAccessTy>
inline VecTy image_1d_sampler_read_helper(const libimg::Float coord,
                                          const Sampler sampler,
                                          const ImageMetaData &desc,
                                          const libimg::UChar *raw_image_data,
                                          const VecTy border_res,
                                          VecAccessTy read_vec4) {
  const libimg::UInt filter_mode = get_sampler_filter_mode(sampler);
  const libimg::UInt addressing_mode = get_sampler_addressing_mode(sampler);
  const libimg::UInt normalized_coords = get_sampler_normalized_coords(sampler);
  const libimg::UInt width = desc.width;
  const libimg::Float u = normalized_coords ? coord * width : coord;
  // NaN
  if (u != u) {
    return border_res;
  }
  // INF
  if (libimg::isinf(u)) {
    return border_res;
  }

  switch (filter_mode) {
    default:
      return border_res;
    case CLK_FILTER_NEAREST: {
      libimg::Int i = 0;
      switch (addressing_mode) {
        default:
          return border_res;
        case CLK_ADDRESS_CLAMP_TO_EDGE: {
          i = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(u)), width);
          // Read outside of the image, return res, that has already been set
          // with border color.
          if ((i >= static_cast<libimg::Int>(width)) || (i < 0)) {
            return border_res;
          }
          break;
        }
        case CLK_ADDRESS_CLAMP: {
          i = addressing_mode_CLAMP(static_cast<libimg::Int>(libimg::floor(u)),
                                    width);
          // Read outside of the image, return res, that has already been set
          // with border color.
          if ((i >= static_cast<libimg::Int>(width)) || (i < 0)) {
            return border_res;
          }

          const void *data = &raw_image_data[desc.pixel_size * i];
          return read_vec4(data, desc.channel_order, desc.channel_type);
          break;
        }
        case CLK_ADDRESS_NONE: {
          i = addressing_mode_NONE(static_cast<libimg::Int>(libimg::floor(u)));
          // Read outside of the image, return res, that has already been set
          // with border color.
          if ((i >= static_cast<libimg::Int>(width)) || (i < 0)) {
            return border_res;
          }
          break;
        }
        case CLK_ADDRESS_REPEAT: {
          const libimg::Float s = coord;
          // NaN
          if (s != s) {
            return border_res;
          }
          // INF
          if (libimg::isinf(s)) {
            return border_res;
          }

          const libimg::Float u = (s - libimg::floor(s)) * width;
          i = libimg::floor(u);
          if (i > static_cast<libimg::Int>(width) - 1) {
            i = i - width;
          }
          break;
        }
        case CLK_ADDRESS_MIRRORED_REPEAT: {
          const libimg::Float s = coord;
          // NaN
          if (s != s) {
            return border_res;
          }
          // INF
          if (libimg::isinf(s)) {
            return border_res;
          }

          libimg::Float s_prim = 2.0f * libimg::rint(0.5f * s);
          s_prim = libimg::fabs(s - s_prim);
          const libimg::Float u = s_prim * width;
          i = libimg::floor(u);
          i = libimg::min(i, static_cast<libimg::Int>(width - 1));
          break;
        }
      }
      const void *data = &raw_image_data[desc.pixel_size * i];
      return read_vec4(data, desc.channel_order, desc.channel_type);
    }
    case CLK_FILTER_LINEAR: {
      libimg::Int i0 = 0;
      libimg::Int i1 = 0;
      libimg::Float a = 0.0f;
      VecTy t_i0;
      VecTy t_i1;
      switch (addressing_mode) {
        default:
          return border_res;
        case CLK_ADDRESS_CLAMP_TO_EDGE: {
          a = frac(u - 0.5f);
          i0 = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f)), width);
          i1 = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f) + 1), width);

          const bool i0_outside =
              (i0 >= static_cast<libimg::Int>(width) || i0 < 0);
          const bool i1_outside =
              (i1 >= static_cast<libimg::Int>(width) || i1 < 0);

          t_i0 = i0_outside ? border_res
                            : read_vec4(&raw_image_data[desc.pixel_size * i0],
                                        desc.channel_order, desc.channel_type);
          t_i1 = i1_outside ? border_res
                            : read_vec4(&raw_image_data[desc.pixel_size * i1],
                                        desc.channel_order, desc.channel_type);
          break;
        }
        case CLK_ADDRESS_CLAMP: {
          a = frac(u - 0.5f);
          i0 = addressing_mode_CLAMP(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f)), width);
          i1 = addressing_mode_CLAMP(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f) + 1), width);

          const bool i0_outside =
              (i0 >= static_cast<libimg::Int>(width) || i0 < 0);
          const bool i1_outside =
              (i1 >= static_cast<libimg::Int>(width) || i1 < 0);

          t_i0 = i0_outside ? border_res
                            : read_vec4(&raw_image_data[desc.pixel_size * i0],
                                        desc.channel_order, desc.channel_type);
          t_i1 = i1_outside ? border_res
                            : read_vec4(&raw_image_data[desc.pixel_size * i1],
                                        desc.channel_order, desc.channel_type);
          break;
        }
        case CLK_ADDRESS_NONE: {  // libimg::Intentional fall-through.
          a = frac(u - 0.5f);
          i0 = addressing_mode_NONE(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f)));
          i1 = addressing_mode_NONE(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f) + 1));

          const bool i0_outside =
              (i0 >= static_cast<libimg::Int>(width) || i0 < 0);
          const bool i1_outside =
              (i1 >= static_cast<libimg::Int>(width) || i1 < 0);

          t_i0 = i0_outside ? border_res
                            : read_vec4(&raw_image_data[desc.pixel_size * i0],
                                        desc.channel_order, desc.channel_type);
          t_i1 = i1_outside ? border_res
                            : read_vec4(&raw_image_data[desc.pixel_size * i1],
                                        desc.channel_order, desc.channel_type);
          break;
        }

        case CLK_ADDRESS_REPEAT: {
          const libimg::Float s = coord;
          // NaN
          if (s != s) {
            return border_res;
          }
          // INF
          if (libimg::isinf(s)) {
            return border_res;
          }

          const libimg::Float u = (s - libimg::floor(s)) * width;
          i0 = libimg::floor(u - 0.5f);
          i1 = i0 + 1;
          if (i0 < 0) {
            i0 = width + i0;
          }
          if (i1 > static_cast<libimg::Int>(width) - 1) {
            i1 = i1 - width;
          }

          a = frac(u - 0.5f);

          t_i0 = read_vec4(&raw_image_data[desc.pixel_size * i0],
                           desc.channel_order, desc.channel_type);
          t_i1 = read_vec4(&raw_image_data[desc.pixel_size * i1],
                           desc.channel_order, desc.channel_type);
          break;
        }
        case CLK_ADDRESS_MIRRORED_REPEAT: {
          const libimg::Float s = coord;
          // NaN
          if (s != s) {
            return border_res;
          }
          // INF
          if (libimg::isinf(s)) {
            return border_res;
          }

          libimg::Float s_prim = 2.0f * libimg::rint(0.5f * s);
          s_prim = libimg::fabs(s - s_prim);
          const libimg::Float u = s_prim * width;
          i0 = libimg::floor(u - 0.5f);
          i1 = i0 + 1;
          i0 = libimg::max(i0, 0);
          i1 = libimg::min(i1, static_cast<libimg::Int>(width - 1));

          a = frac(u - 0.5f);

          t_i0 = read_vec4(&raw_image_data[desc.pixel_size * i0],
                           desc.channel_order, desc.channel_type);
          t_i1 = read_vec4(&raw_image_data[desc.pixel_size * i1],
                           desc.channel_order, desc.channel_type);
          break;
        }
      }

      return vec4_plus_vec4<VecTy, VecElemTy>(
          vec4_times_scalar<VecTy, libimg::Float>(t_i0, (1 - a)),
          vec4_times_scalar<VecTy, libimg::Float>(t_i1, a));
    }
  }

  return border_res;
}

template <typename VecTy, typename VecElemTy, typename VecAccessTy>
inline VecTy image_2d_sampler_read_helper(const libimg::Float2 &coord,
                                          const Sampler sampler,
                                          const ImageMetaData &desc,
                                          const libimg::UChar *raw_image_data,
                                          const VecTy border_res,
                                          VecAccessTy read_vec4) {
  const libimg::UInt filter_mode = get_sampler_filter_mode(sampler);
  const libimg::UInt addressing_mode = get_sampler_addressing_mode(sampler);
  const libimg::UInt normalized_coords = get_sampler_normalized_coords(sampler);
  const libimg::UInt width = desc.width;
  const libimg::UInt height = desc.height;
  const libimg::Float u =
      normalized_coords
          ? libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::x) * width
          : libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::x);
  const libimg::Float v =
      normalized_coords
          ? libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::y) * height
          : libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::y);

  // NaN
  if ((u != u) || (v != v)) {
    return border_res;
  }
  // INF
  if (libimg::isinf(u) || libimg::isinf(v)) {
    return border_res;
  }

  switch (filter_mode) {
    default:
      return border_res;
    case CLK_FILTER_NEAREST: {
      libimg::Int i = 0;
      libimg::Int j = 0;
      switch (addressing_mode) {
        default:
          return border_res;
        case CLK_ADDRESS_CLAMP_TO_EDGE: {
          i = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(u)), width);
          j = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(v)), height);
          // Read outside of the image, return res, that has already been set
          // with border color.
          if ((i >= static_cast<libimg::Int>(width)) ||
              (j >= static_cast<libimg::Int>(height)) || (i < 0) || (j < 0)) {
            return border_res;
          }
          break;
        }
        case CLK_ADDRESS_CLAMP: {
          i = addressing_mode_CLAMP(static_cast<libimg::Int>(libimg::floor(u)),
                                    width);
          j = addressing_mode_CLAMP(static_cast<libimg::Int>(libimg::floor(v)),
                                    height);
          // Read outside of the image, return res, that has already been set
          // with border color.
          if ((i >= static_cast<libimg::Int>(width)) ||
              (j >= static_cast<libimg::Int>(height)) || (i < 0) || (j < 0)) {
            return border_res;
          }

          const void *data =
              &raw_image_data[desc.pixel_size * i + desc.row_pitch * j];
          return read_vec4(data, desc.channel_order, desc.channel_type);
          break;
        }
        case CLK_ADDRESS_NONE: {
          i = addressing_mode_NONE(static_cast<libimg::Int>(libimg::floor(u)));
          j = addressing_mode_NONE(static_cast<libimg::Int>(libimg::floor(v)));
          // Read outside of the image, return res, that has already been set
          // with border color.
          if ((i >= static_cast<libimg::Int>(width)) ||
              (j >= static_cast<libimg::Int>(height)) || (i < 0) || (j < 0)) {
            return border_res;
          }
          break;
        }
        case CLK_ADDRESS_REPEAT: {
          const libimg::Float s =
              libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::x);
          const libimg::Float t =
              libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::y);
          // NaN
          if ((s != s) || (t != t)) {
            return border_res;
          }
          // INF
          if (libimg::isinf(s) || libimg::isinf(t)) {
            return border_res;
          }

          const libimg::Float u = (s - libimg::floor(s)) * width;
          i = libimg::floor(u);
          if (i > static_cast<libimg::Int>(width) - 1) {
            i = i - width;
          }

          const libimg::Float v = (t - libimg::floor(t)) * height;
          j = libimg::floor(v);
          if (j > static_cast<libimg::Int>(height) - 1) {
            j = j - height;
          }
          break;
        }
        case CLK_ADDRESS_MIRRORED_REPEAT: {
          const libimg::Float s =
              libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::x);
          const libimg::Float t =
              libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::y);
          // NaN
          if ((s != s) || (t != t)) {
            return border_res;
          }
          // INF
          if (libimg::isinf(s) || libimg::isinf(t)) {
            return border_res;
          }

          libimg::Float s_prim = 2.0f * libimg::rint(0.5f * s);
          s_prim = libimg::fabs(s - s_prim);
          const libimg::Float u = s_prim * width;
          i = libimg::floor(u);
          i = libimg::min(i, static_cast<libimg::Int>(width - 1));

          libimg::Float t_prim = 2.0f * libimg::rint(0.5f * t);
          t_prim = libimg::fabs(t - t_prim);
          const libimg::Float v = t_prim * height;
          j = libimg::floor(v);
          j = libimg::min(j, static_cast<libimg::Int>(height - 1));
          break;
        }
      }
      const void *data =
          &raw_image_data[desc.pixel_size * i + desc.row_pitch * j];
      return read_vec4(data, desc.channel_order, desc.channel_type);
    }
    case CLK_FILTER_LINEAR: {
      libimg::Int i0 = 0;
      libimg::Int j0 = 0;
      libimg::Int i1 = 0;
      libimg::Int j1 = 0;
      libimg::Float a = 0.0f;
      libimg::Float b = 0.0f;
      VecTy t_i0j0;
      VecTy t_i1j0;
      VecTy t_i0j1;
      VecTy t_i1j1;
      switch (addressing_mode) {
        default:
          return border_res;
        case CLK_ADDRESS_CLAMP_TO_EDGE: {
          a = frac(u - 0.5f);
          b = frac(v - 0.5f);
          i0 = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f)), width);
          j0 = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(v - 0.5f)), height);
          i1 = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f) + 1), width);
          j1 = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(v - 0.5f) + 1), height);

          const bool i0_outside =
              (i0 >= static_cast<libimg::Int>(width) || i0 < 0);
          const bool i1_outside =
              (i1 >= static_cast<libimg::Int>(width) || i1 < 0);
          const bool j0_outside =
              (j0 >= static_cast<libimg::Int>(height) || j0 < 0);
          const bool j1_outside =
              (j1 >= static_cast<libimg::Int>(height) || j1 < 0);

          t_i0j0 = (i0_outside || j0_outside)
                       ? border_res
                       : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                   desc.row_pitch * j0],
                                   desc.channel_order, desc.channel_type);
          t_i1j0 = (i1_outside || j0_outside)
                       ? border_res
                       : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                   desc.row_pitch * j0],
                                   desc.channel_order, desc.channel_type);
          t_i0j1 = (i0_outside || j1_outside)
                       ? border_res
                       : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                   desc.row_pitch * j1],
                                   desc.channel_order, desc.channel_type);
          t_i1j1 = (i1_outside || j1_outside)
                       ? border_res
                       : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                   desc.row_pitch * j1],
                                   desc.channel_order, desc.channel_type);

          break;
        }
        case CLK_ADDRESS_CLAMP: {
          a = frac(u - 0.5f);
          b = frac(v - 0.5f);
          i0 = addressing_mode_CLAMP(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f)), width);
          j0 = addressing_mode_CLAMP(
              static_cast<libimg::Int>(libimg::floor(v - 0.5f)), height);
          i1 = addressing_mode_CLAMP(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f) + 1), width);
          j1 = addressing_mode_CLAMP(
              static_cast<libimg::Int>(libimg::floor(v - 0.5f) + 1), height);

          const bool i0_outside =
              (i0 >= static_cast<libimg::Int>(width) || i0 < 0);
          const bool i1_outside =
              (i1 >= static_cast<libimg::Int>(width) || i1 < 0);
          const bool j0_outside =
              (j0 >= static_cast<libimg::Int>(height) || j0 < 0);
          const bool j1_outside =
              (j1 >= static_cast<libimg::Int>(height) || j1 < 0);

          t_i0j0 = (i0_outside || j0_outside)
                       ? border_res
                       : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                   desc.row_pitch * j0],
                                   desc.channel_order, desc.channel_type);
          t_i1j0 = (i1_outside || j0_outside)
                       ? border_res
                       : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                   desc.row_pitch * j0],
                                   desc.channel_order, desc.channel_type);
          t_i0j1 = (i0_outside || j1_outside)
                       ? border_res
                       : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                   desc.row_pitch * j1],
                                   desc.channel_order, desc.channel_type);
          t_i1j1 = (i1_outside || j1_outside)
                       ? border_res
                       : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                   desc.row_pitch * j1],
                                   desc.channel_order, desc.channel_type);
          break;
        }
        case CLK_ADDRESS_NONE: {  // libimg::Intentional fall-through.
          a = frac(u - 0.5f);
          b = frac(v - 0.5f);
          i0 = addressing_mode_NONE(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f)));
          j0 = addressing_mode_NONE(
              static_cast<libimg::Int>(libimg::floor(v - 0.5f)));
          i1 = addressing_mode_NONE(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f) + 1));
          j1 = addressing_mode_NONE(
              static_cast<libimg::Int>(libimg::floor(v - 0.5f) + 1));

          const bool i0_outside =
              (i0 >= static_cast<libimg::Int>(width) || i0 < 0);
          const bool i1_outside =
              (i1 >= static_cast<libimg::Int>(width) || i1 < 0);
          const bool j0_outside =
              (j0 >= static_cast<libimg::Int>(height) || j0 < 0);
          const bool j1_outside =
              (j1 >= static_cast<libimg::Int>(height) || j1 < 0);

          t_i0j0 = (i0_outside || j0_outside)
                       ? border_res
                       : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                   desc.row_pitch * j0],
                                   desc.channel_order, desc.channel_type);
          t_i1j0 = (i1_outside || j0_outside)
                       ? border_res
                       : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                   desc.row_pitch * j0],
                                   desc.channel_order, desc.channel_type);
          t_i0j1 = (i0_outside || j1_outside)
                       ? border_res
                       : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                   desc.row_pitch * j1],
                                   desc.channel_order, desc.channel_type);
          t_i1j1 = (i1_outside || j1_outside)
                       ? border_res
                       : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                   desc.row_pitch * j1],
                                   desc.channel_order, desc.channel_type);
          break;
        }

        case CLK_ADDRESS_REPEAT: {
          const libimg::Float s =
              libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::x);
          const libimg::Float t =
              libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::y);
          // NaN
          if ((s != s) || (t != t)) {
            return border_res;
          }
          // INF
          if (libimg::isinf(s) || libimg::isinf(t)) {
            return border_res;
          }

          const libimg::Float u = (s - libimg::floor(s)) * width;
          i0 = libimg::floor(u - 0.5f);
          i1 = i0 + 1;
          if (i0 < 0) {
            i0 = width + i0;
          }
          if (i1 > static_cast<libimg::Int>(width) - 1) {
            i1 = i1 - width;
          }

          const libimg::Float v = (t - libimg::floor(t)) * height;
          j0 = libimg::floor(v - 0.5f);
          j1 = j0 + 1;
          if (j0 < 0) {
            j0 = height + j0;
          }
          if (j1 > static_cast<libimg::Int>(height) - 1) {
            j1 = j1 - height;
          }

          a = frac(u - 0.5f);
          b = frac(v - 0.5f);

          t_i0j0 = read_vec4(
              &raw_image_data[desc.pixel_size * i0 + desc.row_pitch * j0],
              desc.channel_order, desc.channel_type);
          t_i1j0 = read_vec4(
              &raw_image_data[desc.pixel_size * i1 + desc.row_pitch * j0],
              desc.channel_order, desc.channel_type);
          t_i0j1 = read_vec4(
              &raw_image_data[desc.pixel_size * i0 + desc.row_pitch * j1],
              desc.channel_order, desc.channel_type);
          t_i1j1 = read_vec4(
              &raw_image_data[desc.pixel_size * i1 + desc.row_pitch * j1],
              desc.channel_order, desc.channel_type);
          break;
        }
        case CLK_ADDRESS_MIRRORED_REPEAT: {
          const libimg::Float s =
              libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::x);
          const libimg::Float t =
              libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::y);
          // NaN
          if ((s != s) || (t != t)) {
            return border_res;
          }
          // INF
          if (libimg::isinf(s) || libimg::isinf(t)) {
            return border_res;
          }

          libimg::Float s_prim = 2.0f * libimg::rint(0.5f * s);
          s_prim = libimg::fabs(s - s_prim);
          const libimg::Float u = s_prim * width;
          i0 = libimg::floor(u - 0.5f);
          i1 = i0 + 1;
          i0 = libimg::max(i0, 0);
          i1 = libimg::min(i1, static_cast<libimg::Int>(width - 1));

          libimg::Float t_prim = 2.0f * libimg::rint(0.5f * t);
          t_prim = libimg::fabs(t - t_prim);
          const libimg::Float v = t_prim * height;
          j0 = libimg::floor(v - 0.5f);
          j1 = j0 + 1;
          j0 = libimg::max(j0, 0);
          j1 = libimg::min(j1, static_cast<libimg::Int>(height - 1));

          a = frac(u - 0.5f);
          b = frac(v - 0.5f);

          t_i0j0 = read_vec4(
              &raw_image_data[desc.pixel_size * i0 + desc.row_pitch * j0],
              desc.channel_order, desc.channel_type);
          t_i1j0 = read_vec4(
              &raw_image_data[desc.pixel_size * i1 + desc.row_pitch * j0],
              desc.channel_order, desc.channel_type);
          t_i0j1 = read_vec4(
              &raw_image_data[desc.pixel_size * i0 + desc.row_pitch * j1],
              desc.channel_order, desc.channel_type);
          t_i1j1 = read_vec4(
              &raw_image_data[desc.pixel_size * i1 + desc.row_pitch * j1],
              desc.channel_order, desc.channel_type);
          break;
        }
      }

      return vec4_plus_vec4<VecTy, VecElemTy>(
          vec4_plus_vec4<VecTy, VecElemTy>(
              vec4_times_scalar<VecTy, libimg::Float>(t_i0j0,
                                                      (1 - a) * (1 - b)),
              vec4_times_scalar<VecTy, libimg::Float>(t_i1j0, a * (1 - b))),
          vec4_plus_vec4<VecTy, VecElemTy>(
              vec4_times_scalar<VecTy, libimg::Float>(t_i0j1, (1 - a) * b),
              vec4_times_scalar<VecTy, libimg::Float>(t_i1j1, a * b)));
    }
  }

  return border_res;
}

template <typename VecTy, typename VecElemTy, typename VecAccessTy>
inline VecTy image_3d_sampler_read_helper(const libimg::Float4 &coord,
                                          const Sampler sampler,
                                          const ImageMetaData &desc,
                                          const libimg::UChar *raw_image_data,
                                          const VecTy border_res,
                                          VecAccessTy read_vec4) {
  const libimg::UInt filter_mode = get_sampler_filter_mode(sampler);
  const libimg::UInt addressing_mode = get_sampler_addressing_mode(sampler);
  const libimg::UInt normalized_coords = get_sampler_normalized_coords(sampler);
  const libimg::UInt width = desc.width;
  const libimg::UInt height = desc.height;
  const libimg::UInt depth = desc.depth;
  const libimg::Float u =
      normalized_coords
          ? libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::x) * width
          : libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::x);
  const libimg::Float v =
      normalized_coords
          ? libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::y) * height
          : libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::y);
  const libimg::Float w =
      normalized_coords
          ? libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::z) * depth
          : libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::z);
  if (normalized_coords) {
  } else {
  }
  // NaN
  if ((u != u) || (v != v) || (w != w)) {
    return border_res;
  }
  // INF
  if (libimg::isinf(u) || libimg::isinf(v) || libimg::isinf(w)) {
    return border_res;
  }

  switch (filter_mode) {
    default:
      return border_res;
    case CLK_FILTER_NEAREST: {
      switch (addressing_mode) {
        default:
          return border_res;
        case CLK_ADDRESS_CLAMP_TO_EDGE: {
          const libimg::Int i = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(u)), width);
          const libimg::Int j = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(v)), height);
          const libimg::Int k = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(w)), depth);
          // Read outside of the image, return res, that has already been set
          // with border color.
          if ((i >= static_cast<libimg::Int>(width)) ||
              (j >= static_cast<libimg::Int>(height)) ||
              (k >= static_cast<libimg::Int>(depth)) || (i < 0) || (j < 0) ||
              (k < 0)) {
            return border_res;
          }

          const void *data =
              &raw_image_data[desc.pixel_size * i + desc.row_pitch * j +
                              desc.slice_pitch * k];
          return read_vec4(data, desc.channel_order, desc.channel_type);
        }
        case CLK_ADDRESS_CLAMP: {
          const libimg::Int i = addressing_mode_CLAMP(
              static_cast<libimg::Int>(libimg::floor(u)), width);
          const libimg::Int j = addressing_mode_CLAMP(
              static_cast<libimg::Int>(libimg::floor(v)), height);
          const libimg::Int k = addressing_mode_CLAMP(
              static_cast<libimg::Int>(libimg::floor(w)), depth);
          // Read outside of the image, return res, that has already been set
          // with border color.
          if ((i >= static_cast<libimg::Int>(width)) ||
              (j >= static_cast<libimg::Int>(height)) ||
              (k >= static_cast<libimg::Int>(depth)) || (i < 0) || (j < 0) ||
              (k < 0)) {
            return border_res;
          }

          const void *data =
              &raw_image_data[desc.pixel_size * i + desc.row_pitch * j +
                              desc.slice_pitch * k];
          return read_vec4(data, desc.channel_order, desc.channel_type);
        }
        case CLK_ADDRESS_NONE: {
          const libimg::Int i =
              addressing_mode_NONE(static_cast<libimg::Int>(libimg::floor(u)));
          const libimg::Int j =
              addressing_mode_NONE(static_cast<libimg::Int>(libimg::floor(v)));
          const libimg::Int k =
              addressing_mode_NONE(static_cast<libimg::Int>(libimg::floor(w)));
          // Read outside of the image, return res, that has already been set
          // with border color.
          if ((i >= static_cast<libimg::Int>(width)) ||
              (j >= static_cast<libimg::Int>(height)) ||
              (k >= static_cast<libimg::Int>(depth)) || (i < 0) || (j < 0) ||
              (k < 0)) {
            return border_res;
          }

          const void *data =
              &raw_image_data[desc.pixel_size * i + desc.row_pitch * j +
                              desc.slice_pitch * k];
          return read_vec4(data, desc.channel_order, desc.channel_type);
        }
        case CLK_ADDRESS_REPEAT: {
          const libimg::Float s =
              libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::x);
          const libimg::Float t =
              libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::y);
          const libimg::Float r =
              libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::z);
          // NaN
          if ((s != s) || (t != t) || (r != r)) {
            return border_res;
          }
          // INF
          if (libimg::isinf(s) || libimg::isinf(t) || libimg::isinf(r)) {
            return border_res;
          }

          const libimg::Float u = (s - libimg::floor(s)) * width;
          libimg::UInt i = libimg::floor(u);
          if (i > width - 1) {
            i = i - width;
          }

          const libimg::Float v = (t - libimg::floor(t)) * height;
          libimg::UInt j = libimg::floor(v);
          if (j > height - 1) {
            j = j - height;
          }

          const libimg::Float w = (r - libimg::floor(r)) * depth;
          libimg::UInt k = libimg::floor(w);
          if (k > depth - 1) {
            k = k - depth;
          }
          const void *data =
              &raw_image_data[desc.pixel_size * i + desc.row_pitch * j +
                              desc.slice_pitch * k];
          return read_vec4(data, desc.channel_order, desc.channel_type);
        }
        case CLK_ADDRESS_MIRRORED_REPEAT: {
          const libimg::Float s =
              libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::x);
          const libimg::Float t =
              libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::y);
          const libimg::Float r =
              libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::z);
          // NaN
          if ((s != s) || (t != t) || (r != r)) {
            return border_res;
          }
          // INF
          if (libimg::isinf(s) || libimg::isinf(t) || libimg::isinf(r)) {
            return border_res;
          }

          libimg::Float s_prim = 2.0f * libimg::rint(0.5f * s);
          s_prim = libimg::fabs(s - s_prim);
          const libimg::Float u = s_prim * width;
          libimg::Int i = libimg::floor(u);
          i = libimg::min(i, static_cast<libimg::Int>(width - 1));

          libimg::Float t_prim = 2.0f * libimg::rint(0.5f * t);
          t_prim = libimg::fabs(t - t_prim);
          const libimg::Float v = t_prim * height;
          libimg::Int j = libimg::floor(v);
          j = libimg::min(j, static_cast<libimg::Int>(height - 1));

          libimg::Float r_prim = 2.0f * libimg::rint(0.5f * r);
          r_prim = libimg::fabs(r - r_prim);
          const libimg::Float w = r_prim * depth;
          libimg::Int k = libimg::floor(w);
          k = libimg::min(k, static_cast<libimg::Int>(depth - 1));

          const void *data =
              &raw_image_data[desc.pixel_size * i + desc.row_pitch * j +
                              desc.slice_pitch * k];
          return read_vec4(data, desc.channel_order, desc.channel_type);
        }
      }
    }
    case CLK_FILTER_LINEAR: {
      libimg::Int i0 = 0;
      libimg::Int j0 = 0;
      libimg::Int k0 = 0;
      libimg::Int i1 = 0;
      libimg::Int j1 = 0;
      libimg::Int k1 = 0;
      libimg::Float a = 0.0f;
      libimg::Float b = 0.0f;
      libimg::Float c = 0.0f;
      VecTy t_i0j0k0;
      VecTy t_i1j0k0;
      VecTy t_i0j1k0;
      VecTy t_i1j1k0;
      VecTy t_i0j0k1;
      VecTy t_i1j0k1;
      VecTy t_i0j1k1;
      VecTy t_i1j1k1;
      switch (addressing_mode) {
        default:
          return border_res;
        case CLK_ADDRESS_CLAMP_TO_EDGE: {
          i0 = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f)), width);
          j0 = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(v - 0.5f)), height);
          k0 = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(w - 0.5f)), depth);
          i1 = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f) + 1), width);
          j1 = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(v - 0.5f) + 1), height);
          k1 = addressing_mode_CLAMP_TO_EDGE(
              static_cast<libimg::Int>(libimg::floor(w - 0.5f) + 1), depth);

          const bool i0_outside =
              (i0 >= static_cast<libimg::Int>(width) || i0 < 0);
          const bool i1_outside =
              (i1 >= static_cast<libimg::Int>(width) || i1 < 0);
          const bool j0_outside =
              (j0 >= static_cast<libimg::Int>(height) || j0 < 0);
          const bool j1_outside =
              (j1 >= static_cast<libimg::Int>(height) || j1 < 0);
          const bool k0_outside =
              (k0 >= static_cast<libimg::Int>(depth) || k0 < 0);
          const bool k1_outside =
              (k1 >= static_cast<libimg::Int>(depth) || k1 < 0);

          t_i0j0k0 = (i0_outside || j0_outside || k0_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                     desc.row_pitch * j0 +
                                                     desc.slice_pitch * k0],
                                     desc.channel_order, desc.channel_type);
          t_i1j0k0 = (i1_outside || j0_outside || k0_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                     desc.row_pitch * j0 +
                                                     desc.slice_pitch * k0],
                                     desc.channel_order, desc.channel_type);
          t_i0j1k0 = (i0_outside || j1_outside || k0_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                     desc.row_pitch * j1 +
                                                     desc.slice_pitch * k0],
                                     desc.channel_order, desc.channel_type);
          t_i1j1k0 = (i1_outside || j1_outside || k0_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                     desc.row_pitch * j1 +
                                                     desc.slice_pitch * k0],
                                     desc.channel_order, desc.channel_type);
          t_i0j0k1 = (i0_outside || j0_outside || k1_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                     desc.row_pitch * j0 +
                                                     desc.slice_pitch * k1],
                                     desc.channel_order, desc.channel_type);
          t_i1j0k1 = (i1_outside || j0_outside || k1_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                     desc.row_pitch * j0 +
                                                     desc.slice_pitch * k1],
                                     desc.channel_order, desc.channel_type);
          t_i0j1k1 = (i0_outside || j1_outside || k1_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                     desc.row_pitch * j1 +
                                                     desc.slice_pitch * k1],
                                     desc.channel_order, desc.channel_type);
          t_i1j1k1 = (i1_outside || j1_outside || k1_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                     desc.row_pitch * j1 +
                                                     desc.slice_pitch * k1],
                                     desc.channel_order, desc.channel_type);

          a = frac(u - 0.5f);
          b = frac(v - 0.5f);
          c = frac(w - 0.5f);

          break;
        }
        case CLK_ADDRESS_CLAMP: {
          i0 = addressing_mode_CLAMP(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f)), width);
          j0 = addressing_mode_CLAMP(
              static_cast<libimg::Int>(libimg::floor(v - 0.5f)), height);
          k0 = addressing_mode_CLAMP(
              static_cast<libimg::Int>(libimg::floor(w - 0.5f)), depth);
          i1 = addressing_mode_CLAMP(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f) + 1), width);
          j1 = addressing_mode_CLAMP(
              static_cast<libimg::Int>(libimg::floor(v - 0.5f) + 1), height);
          k1 = addressing_mode_CLAMP(
              static_cast<libimg::Int>(libimg::floor(w - 0.5f) + 1), depth);

          const bool i0_outside =
              (i0 >= static_cast<libimg::Int>(width) || i0 < 0);
          const bool i1_outside =
              (i1 >= static_cast<libimg::Int>(width) || i1 < 0);
          const bool j0_outside =
              (j0 >= static_cast<libimg::Int>(height) || j0 < 0);
          const bool j1_outside =
              (j1 >= static_cast<libimg::Int>(height) || j1 < 0);
          const bool k0_outside =
              (k0 >= static_cast<libimg::Int>(depth) || k0 < 0);
          const bool k1_outside =
              (k1 >= static_cast<libimg::Int>(depth) || k1 < 0);

          t_i0j0k0 = (i0_outside || j0_outside || k0_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                     desc.row_pitch * j0 +
                                                     desc.slice_pitch * k0],
                                     desc.channel_order, desc.channel_type);
          t_i1j0k0 = (i1_outside || j0_outside || k0_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                     desc.row_pitch * j0 +
                                                     desc.slice_pitch * k0],
                                     desc.channel_order, desc.channel_type);
          t_i0j1k0 = (i0_outside || j1_outside || k0_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                     desc.row_pitch * j1 +
                                                     desc.slice_pitch * k0],
                                     desc.channel_order, desc.channel_type);
          t_i1j1k0 = (i1_outside || j1_outside || k0_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                     desc.row_pitch * j1 +
                                                     desc.slice_pitch * k0],
                                     desc.channel_order, desc.channel_type);
          t_i0j0k1 = (i0_outside || j0_outside || k1_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                     desc.row_pitch * j0 +
                                                     desc.slice_pitch * k1],
                                     desc.channel_order, desc.channel_type);
          t_i1j0k1 = (i1_outside || j0_outside || k1_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                     desc.row_pitch * j0 +
                                                     desc.slice_pitch * k1],
                                     desc.channel_order, desc.channel_type);
          t_i0j1k1 = (i0_outside || j1_outside || k1_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                     desc.row_pitch * j1 +
                                                     desc.slice_pitch * k1],
                                     desc.channel_order, desc.channel_type);
          t_i1j1k1 = (i1_outside || j1_outside || k1_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                     desc.row_pitch * j1 +
                                                     desc.slice_pitch * k1],
                                     desc.channel_order, desc.channel_type);

          a = frac(u - 0.5f);
          b = frac(v - 0.5f);
          c = frac(w - 0.5f);

          break;
        }
        case CLK_ADDRESS_NONE: {
          i0 = addressing_mode_NONE(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f)));
          j0 = addressing_mode_NONE(
              static_cast<libimg::Int>(libimg::floor(v - 0.5f)));
          k0 = addressing_mode_NONE(
              static_cast<libimg::Int>(libimg::floor(w - 0.5f)));
          i1 = addressing_mode_NONE(
              static_cast<libimg::Int>(libimg::floor(u - 0.5f) + 1));
          j1 = addressing_mode_NONE(
              static_cast<libimg::Int>(libimg::floor(v - 0.5f) + 1));
          k1 = addressing_mode_NONE(
              static_cast<libimg::Int>(libimg::floor(w - 0.5f) + 1));

          const bool i0_outside =
              (i0 >= static_cast<libimg::Int>(width) || i0 < 0);
          const bool i1_outside =
              (i1 >= static_cast<libimg::Int>(width) || i1 < 0);
          const bool j0_outside =
              (j0 >= static_cast<libimg::Int>(height) || j0 < 0);
          const bool j1_outside =
              (j1 >= static_cast<libimg::Int>(height) || j1 < 0);
          const bool k0_outside =
              (k0 >= static_cast<libimg::Int>(depth) || k0 < 0);
          const bool k1_outside =
              (k1 >= static_cast<libimg::Int>(depth) || k1 < 0);

          t_i0j0k0 = (i0_outside || j0_outside || k0_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                     desc.row_pitch * j0 +
                                                     desc.slice_pitch * k0],
                                     desc.channel_order, desc.channel_type);
          t_i1j0k0 = (i1_outside || j0_outside || k0_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                     desc.row_pitch * j0 +
                                                     desc.slice_pitch * k0],
                                     desc.channel_order, desc.channel_type);
          t_i0j1k0 = (i0_outside || j1_outside || k0_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                     desc.row_pitch * j1 +
                                                     desc.slice_pitch * k0],
                                     desc.channel_order, desc.channel_type);
          t_i1j1k0 = (i1_outside || j1_outside || k0_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                     desc.row_pitch * j1 +
                                                     desc.slice_pitch * k0],
                                     desc.channel_order, desc.channel_type);
          t_i0j0k1 = (i0_outside || j0_outside || k1_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                     desc.row_pitch * j0 +
                                                     desc.slice_pitch * k1],
                                     desc.channel_order, desc.channel_type);
          t_i1j0k1 = (i1_outside || j0_outside || k1_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                     desc.row_pitch * j0 +
                                                     desc.slice_pitch * k1],
                                     desc.channel_order, desc.channel_type);
          t_i0j1k1 = (i0_outside || j1_outside || k1_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i0 +
                                                     desc.row_pitch * j1 +
                                                     desc.slice_pitch * k1],
                                     desc.channel_order, desc.channel_type);
          t_i1j1k1 = (i1_outside || j1_outside || k1_outside)
                         ? border_res
                         : read_vec4(&raw_image_data[desc.pixel_size * i1 +
                                                     desc.row_pitch * j1 +
                                                     desc.slice_pitch * k1],
                                     desc.channel_order, desc.channel_type);

          a = frac(u - 0.5f);
          b = frac(v - 0.5f);
          c = frac(w - 0.5f);

          break;
        }
        case CLK_ADDRESS_REPEAT: {
          const libimg::Float s =
              libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::x);
          const libimg::Float t =
              libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::y);
          const libimg::Float r =
              libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::z);
          // NaN
          if ((s != s) || (t != t) || (r != r)) {
            return border_res;
          }
          // INF
          if (libimg::isinf(s) || libimg::isinf(t) || libimg::isinf(r)) {
            return border_res;
          }

          const libimg::Float u = (s - libimg::floor(s)) * width;
          i0 = libimg::floor(u - 0.5f);
          i1 = i0 + 1;
          if (i0 < 0) {
            i0 = width + i0;
          }
          if (i1 > static_cast<libimg::Int>(width) - 1) {
            i1 = i1 - width;
          }

          const libimg::Float v = (t - libimg::floor(t)) * height;
          j0 = libimg::floor(v - 0.5f);
          j1 = j0 + 1;
          if (j0 < 0) {
            j0 = height + j0;
          }
          if (j1 > static_cast<libimg::Int>(height) - 1) {
            j1 = j1 - height;
          }

          const libimg::Float w = (r - libimg::floor(r)) * depth;
          k0 = libimg::floor(w - 0.5f);
          k1 = k0 + 1;
          if (k0 < 0) {
            k0 = depth + k0;
          }
          if (k1 > static_cast<libimg::Int>(depth) - 1) {
            k1 = k1 - depth;
          }

          a = frac(u - 0.5f);
          b = frac(v - 0.5f);
          c = frac(w - 0.5f);

          t_i0j0k0 = read_vec4(
              &raw_image_data[desc.pixel_size * i0 + desc.row_pitch * j0 +
                              desc.slice_pitch * k0],
              desc.channel_order, desc.channel_type);
          t_i1j0k0 = read_vec4(
              &raw_image_data[desc.pixel_size * i1 + desc.row_pitch * j0 +
                              desc.slice_pitch * k0],
              desc.channel_order, desc.channel_type);
          t_i0j1k0 = read_vec4(
              &raw_image_data[desc.pixel_size * i0 + desc.row_pitch * j1 +
                              desc.slice_pitch * k0],
              desc.channel_order, desc.channel_type);
          t_i1j1k0 = read_vec4(
              &raw_image_data[desc.pixel_size * i1 + desc.row_pitch * j1 +
                              desc.slice_pitch * k0],
              desc.channel_order, desc.channel_type);
          t_i0j0k1 = read_vec4(
              &raw_image_data[desc.pixel_size * i0 + desc.row_pitch * j0 +
                              desc.slice_pitch * k1],
              desc.channel_order, desc.channel_type);
          t_i1j0k1 = read_vec4(
              &raw_image_data[desc.pixel_size * i1 + desc.row_pitch * j0 +
                              desc.slice_pitch * k1],
              desc.channel_order, desc.channel_type);
          t_i0j1k1 = read_vec4(
              &raw_image_data[desc.pixel_size * i0 + desc.row_pitch * j1 +
                              desc.slice_pitch * k1],
              desc.channel_order, desc.channel_type);
          t_i1j1k1 = read_vec4(
              &raw_image_data[desc.pixel_size * i1 + desc.row_pitch * j1 +
                              desc.slice_pitch * k1],
              desc.channel_order, desc.channel_type);

          break;
        }
        case CLK_ADDRESS_MIRRORED_REPEAT: {
          const libimg::Float s =
              libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::x);
          const libimg::Float t =
              libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::y);
          const libimg::Float r =
              libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::z);
          // NaN
          if ((s != s) || (t != t) || (r != r)) {
            return border_res;
          }
          // INF
          if (libimg::isinf(s) || libimg::isinf(t) || libimg::isinf(r)) {
            return border_res;
          }

          libimg::Float s_prim = 2.0f * libimg::rint(0.5f * s);
          s_prim = libimg::fabs(s - s_prim);
          const libimg::Float u = s_prim * width;
          i0 = libimg::floor(u - 0.5f);
          i1 = i0 + 1;
          i0 = libimg::max(i0, 0);
          i1 = libimg::min(i1, static_cast<libimg::Int>(width - 1));

          libimg::Float t_prim = 2.0f * libimg::rint(0.5f * t);
          t_prim = libimg::fabs(t - t_prim);
          const libimg::Float v = t_prim * height;
          j0 = libimg::floor(v - 0.5f);
          j1 = j0 + 1;
          j0 = libimg::max(j0, 0);
          j1 = libimg::min(j1, static_cast<libimg::Int>(height - 1));

          libimg::Float r_prim = 2.0f * libimg::rint(0.5f * r);
          r_prim = libimg::fabs(r - r_prim);
          const libimg::Float w = r_prim * depth;
          k0 = libimg::floor(w - 0.5f);
          k1 = k0 + 1;
          k0 = libimg::max(k0, 0);
          k1 = libimg::min(k1, static_cast<libimg::Int>(depth - 1));

          a = frac(u - 0.5f);
          b = frac(v - 0.5f);
          c = frac(w - 0.5f);

          t_i0j0k0 = read_vec4(
              &raw_image_data[desc.pixel_size * i0 + desc.row_pitch * j0 +
                              desc.slice_pitch * k0],
              desc.channel_order, desc.channel_type);
          t_i1j0k0 = read_vec4(
              &raw_image_data[desc.pixel_size * i1 + desc.row_pitch * j0 +
                              desc.slice_pitch * k0],
              desc.channel_order, desc.channel_type);
          t_i0j1k0 = read_vec4(
              &raw_image_data[desc.pixel_size * i0 + desc.row_pitch * j1 +
                              desc.slice_pitch * k0],
              desc.channel_order, desc.channel_type);
          t_i1j1k0 = read_vec4(
              &raw_image_data[desc.pixel_size * i1 + desc.row_pitch * j1 +
                              desc.slice_pitch * k0],
              desc.channel_order, desc.channel_type);
          t_i0j0k1 = read_vec4(
              &raw_image_data[desc.pixel_size * i0 + desc.row_pitch * j0 +
                              desc.slice_pitch * k1],
              desc.channel_order, desc.channel_type);
          t_i1j0k1 = read_vec4(
              &raw_image_data[desc.pixel_size * i1 + desc.row_pitch * j0 +
                              desc.slice_pitch * k1],
              desc.channel_order, desc.channel_type);
          t_i0j1k1 = read_vec4(
              &raw_image_data[desc.pixel_size * i0 + desc.row_pitch * j1 +
                              desc.slice_pitch * k1],
              desc.channel_order, desc.channel_type);
          t_i1j1k1 = read_vec4(
              &raw_image_data[desc.pixel_size * i1 + desc.row_pitch * j1 +
                              desc.slice_pitch * k1],
              desc.channel_order, desc.channel_type);

          break;
        }
      }
      return vec4_plus_vec4<VecTy, VecElemTy>(
          vec4_plus_vec4<VecTy, VecElemTy>(
              vec4_plus_vec4<VecTy, VecElemTy>(
                  vec4_times_scalar<VecTy, libimg::Float>(
                      t_i0j0k0, (1 - a) * (1 - b) * (1 - c)),
                  vec4_times_scalar<VecTy, libimg::Float>(
                      t_i1j0k0, a * (1 - b) * (1 - c))),
              vec4_plus_vec4<VecTy, VecElemTy>(
                  vec4_times_scalar<VecTy, libimg::Float>(
                      t_i0j1k0, (1 - a) * b * (1 - c)),
                  vec4_times_scalar<VecTy, libimg::Float>(t_i1j1k0,
                                                          a * b * (1 - c)))),
          vec4_plus_vec4<VecTy, VecElemTy>(
              vec4_plus_vec4<VecTy, VecElemTy>(
                  vec4_times_scalar<VecTy, libimg::Float>(
                      t_i0j0k1, (1 - a) * (1 - b) * c),
                  vec4_times_scalar<VecTy, libimg::Float>(t_i1j0k1,
                                                          a * (1 - b) * c)),
              vec4_plus_vec4<VecTy, VecElemTy>(
                  vec4_times_scalar<VecTy, libimg::Float>(t_i0j1k1,
                                                          (1 - a) * b * c),
                  vec4_times_scalar<VecTy, libimg::Float>(t_i1j1k1,
                                                          a * b * c))));
    }
  }

  return border_res;
}

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Read image.                                                               */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
libimg::Float4 __Codeplay_read_imagef_3d(Image *image, Sampler sampler,
                                         libimg::Int4 coord) {
  // CLK_NORMALIZED_COORDS_TRUE with int coordinate are not valid.
  if (get_sampler_normalized_coords(sampler)) {
    return libimg::make<libimg::Float4>(0.0f, 0.0f, 0.0f, 0.0f);
  }
  libimg::Float4 f_coord = libimg::convert_float4(coord);
  return __Codeplay_read_imagef_3d(image, sampler, f_coord);
}

libimg::Float4 __Codeplay_read_imagef_3d(Image *image, Sampler sampler,
                                         libimg::Float4 coord) {
  ImageMetaData &desc = image->meta_data;
  return image_3d_sampler_read_helper<libimg::Float4, libimg::Float>(
      coord, sampler, desc, image->raw_data,
      border_color<libimg::Float4>(desc.channel_order), float4_reader());
}

libimg::Float4 __Codeplay_read_imagef_2d_array(Image *image, Sampler sampler,
                                               libimg::Int4 coord) {
  // CLK_NORMALIZED_COORDS_TRUE with int coordinate are not valid.
  if (get_sampler_normalized_coords(sampler)) {
    return libimg::make<libimg::Float4>(0.0f, 0.0f, 0.0f, 0.0f);
  }
  libimg::Float4 f_coord = libimg::convert_float4(coord);
  return __Codeplay_read_imagef_2d_array(image, sampler, f_coord);
}

libimg::Float4 __Codeplay_read_imagef_2d_array(Image *image, Sampler sampler,
                                               libimg::Float4 coord) {
  ImageMetaData &desc = image->meta_data;
  const libimg::Int array_size = desc.array_size;
  libimg::Float w = libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::z);
  const libimg::Float array_max_idx = array_size - 1;
  libimg::Float layer_f = libimg::floor(w + 0.5f);
  layer_f = layer_f > array_max_idx ? array_max_idx : layer_f;
  layer_f = layer_f < 0.0f ? 0.0f : layer_f;
  const libimg::Int layer = libimg::convert_int_rte(layer_f);
  libimg::Float2 coord_2d;
  libimg::set_v2(coord_2d,
                 libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::x),
                 libimg::vec_elem::x);
  libimg::set_v2(coord_2d,
                 libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::y),
                 libimg::vec_elem::y);
  return image_2d_sampler_read_helper<libimg::Float4, libimg::Float>(
      coord_2d, sampler, desc, &image->raw_data[desc.slice_pitch * layer],
      border_color<libimg::Float4>(desc.channel_order), float4_reader());
}

libimg::Float4 __Codeplay_read_imagef_2d(Image *image, Sampler sampler,
                                         libimg::Int2 coord) {
  // CLK_NORMALIZED_COORDS_TRUE with int coordinate are not valid.
  if (get_sampler_normalized_coords(sampler)) {
    return libimg::make<libimg::Float4>(0.0f, 0.0f, 0.0f, 0.0f);
  }
  libimg::Float2 f_coord = libimg::convert_float2(coord);
  return __Codeplay_read_imagef_2d(image, sampler, f_coord);
}

libimg::Float4 __Codeplay_read_imagef_2d(Image *image, Sampler sampler,
                                         libimg::Float2 coord) {
  ImageMetaData &desc = image->meta_data;
  return image_2d_sampler_read_helper<libimg::Float4, libimg::Float>(
      coord, sampler, desc, image->raw_data,
      border_color<libimg::Float4>(desc.channel_order), float4_reader());
}

libimg::Float4 __Codeplay_read_imagef_1d_array(Image *image, Sampler sampler,
                                               libimg::Int2 coord) {
  // CLK_NORMALIZED_COORDS_TRUE with int coordinate are not valid.
  if (get_sampler_normalized_coords(sampler)) {
    return libimg::make<libimg::Float4>(0.0f, 0.0f, 0.0f, 0.0f);
  }
  libimg::Float2 f_coord = libimg::convert_float2(coord);
  return __Codeplay_read_imagef_1d_array(image, sampler, f_coord);
}

libimg::Float4 __Codeplay_read_imagef_1d_array(Image *image, Sampler sampler,
                                               libimg::Float2 coord) {
  ImageMetaData &desc = image->meta_data;
  const libimg::Int array_size = desc.array_size;
  libimg::Float v = libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::y);
  const libimg::Float array_mix_idx = array_size - 1;
  libimg::Float layer_f = libimg::floor(v + 0.5f);
  layer_f = layer_f > array_mix_idx ? array_mix_idx : layer_f;
  layer_f = layer_f < 0.0f ? 0.0f : layer_f;
  const libimg::Int layer = libimg::convert_int_rte(layer_f);
  return image_1d_sampler_read_helper<libimg::Float4, libimg::Float>(
      libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::x), sampler, desc,
      &image->raw_data[desc.slice_pitch * layer],
      border_color<libimg::Float4>(desc.channel_order), float4_reader());
}

libimg::Float4 __Codeplay_read_imagef_1d(Image *image, Sampler sampler,
                                         libimg::Int coord) {
  // CLK_NORMALIZED_COORDS_TRUE with int coordinate are not valid.
  if (get_sampler_normalized_coords(sampler)) {
    return libimg::make<libimg::Float4>(0.0f, 0.0f, 0.0f, 0.0f);
  }
  return __Codeplay_read_imagef_1d(image, sampler,
                                   static_cast<libimg::Float>(coord));
}

libimg::Float4 __Codeplay_read_imagef_1d(Image *image, Sampler sampler,
                                         libimg::Float coord) {
  ImageMetaData &desc = image->meta_data;
  libimg::Float4 ret =
      image_1d_sampler_read_helper<libimg::Float4, libimg::Float>(
          coord, sampler, desc, image->raw_data,
          border_color<libimg::Float4>(desc.channel_order), float4_reader());
  return ret;
}

libimg::Int4 __Codeplay_read_imagei_3d(Image *image, Sampler sampler,
                                       libimg::Int4 coord) {
  // CLK_NORMALIZED_COORDS_TRUE with int coordinate are not valid.
  if (get_sampler_normalized_coords(sampler)) {
    return libimg::make<libimg::Int4>(0, 0, 0, 0);
  }
  switch (get_sampler_addressing_mode(sampler)) {
    default:
      return libimg::make<libimg::Int4>(0, 0, 0, 0);
    case CLK_ADDRESS_CLAMP_TO_EDGE:
    case CLK_ADDRESS_CLAMP:
    case CLK_ADDRESS_NONE:  // libimg::Intentional fall-through;
      break;
  }
  libimg::Float4 f_coord = libimg::convert_float4(coord);
  return __Codeplay_read_imagei_3d(image, sampler, f_coord);
}

libimg::Int4 __Codeplay_read_imagei_3d(Image *image, Sampler sampler,
                                       libimg::Float4 coord) {
  ImageMetaData &desc = image->meta_data;
  return image_3d_sampler_read_helper<libimg::Int4, libimg::Int>(
      coord, sampler, desc, image->raw_data,
      border_color<libimg::Int4>(desc.channel_order), int4_reader());
}

libimg::Int4 __Codeplay_read_imagei_2d_array(Image *image, Sampler sampler,
                                             libimg::Int4 coord) {
  // CLK_NORMALIZED_COORDS_TRUE with int coordinate are not valid.
  if (get_sampler_normalized_coords(sampler)) {
    return libimg::make<libimg::Int4>(0, 0, 0, 0);
  }
  libimg::Float4 f_coord = libimg::convert_float4(coord);
  return __Codeplay_read_imagei_2d_array(image, sampler, f_coord);
}

libimg::Int4 __Codeplay_read_imagei_2d_array(Image *image, Sampler sampler,
                                             libimg::Float4 coord) {
  ImageMetaData &desc = image->meta_data;
  const libimg::Int array_size = desc.array_size;
  libimg::Float w = libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::z);
  const libimg::Float array_mix_idx = array_size - 1;
  libimg::Float layer_f = libimg::floor(w + 0.5f);
  layer_f = layer_f > array_mix_idx ? array_mix_idx : layer_f;
  layer_f = layer_f < 0.0f ? 0.0f : layer_f;
  const libimg::Int layer = libimg::convert_int_rte(layer_f);
  libimg::Float2 coord_2d;
  libimg::set_v2(coord_2d,
                 libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::x),
                 libimg::vec_elem::x);
  libimg::set_v2(coord_2d,
                 libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::y),
                 libimg::vec_elem::y);
  return image_2d_sampler_read_helper<libimg::Int4, libimg::Int>(
      coord_2d, sampler, desc, &image->raw_data[desc.slice_pitch * layer],
      border_color<libimg::Int4>(desc.channel_order), int4_reader());
}

libimg::Int4 __Codeplay_read_imagei_2d(Image *image, Sampler sampler,
                                       libimg::Int2 coord) {
  // CLK_NORMALIZED_COORDS_TRUE with int coordinate are not valid.
  if (get_sampler_normalized_coords(sampler)) {
    return libimg::make<libimg::Int4>(0, 0, 0, 0);
  }
  switch (get_sampler_addressing_mode(sampler)) {
    default:
      return libimg::make<libimg::Int4>(0, 0, 0, 0);
    case CLK_ADDRESS_CLAMP_TO_EDGE:
    case CLK_ADDRESS_CLAMP:
    case CLK_ADDRESS_NONE:  // libimg::Intentional fall-through;
      break;
  }
  libimg::Float2 f_coord = libimg::convert_float2(coord);
  return __Codeplay_read_imagei_2d(image, sampler, f_coord);
}

libimg::Int4 __Codeplay_read_imagei_2d(Image *image, Sampler sampler,
                                       libimg::Float2 coord) {
  ImageMetaData &desc = image->meta_data;
  return image_2d_sampler_read_helper<libimg::Int4, libimg::Int>(
      coord, sampler, desc, image->raw_data,
      border_color<libimg::Int4>(desc.channel_order), int4_reader());
}

libimg::Int4 __Codeplay_read_imagei_1d_array(Image *image, Sampler sampler,
                                             libimg::Int2 coord) {
  // CLK_NORMALIZED_COORDS_TRUE with int coordinate are not valid.
  if (get_sampler_normalized_coords(sampler)) {
    return libimg::make<libimg::Int4>(0, 0, 0, 0);
  }
  libimg::Float2 f_coord = libimg::convert_float2(coord);
  return __Codeplay_read_imagei_1d_array(image, sampler, f_coord);
}

libimg::Int4 __Codeplay_read_imagei_1d_array(Image *image, Sampler sampler,
                                             libimg::Float2 coord) {
  ImageMetaData &desc = image->meta_data;
  const libimg::Int array_size = desc.array_size;
  const libimg::Float v =
      libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::y);
  const libimg::Float array_mix_idx = array_size - 1;
  libimg::Float layer_f = libimg::floor(v + 0.5f);
  layer_f = layer_f > array_mix_idx ? array_mix_idx : layer_f;
  layer_f = layer_f < 0.0f ? 0.0f : layer_f;
  const libimg::Int layer = libimg::convert_int_rte(layer_f);
  return image_1d_sampler_read_helper<libimg::Int4, libimg::Int>(
      libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::x), sampler, desc,
      &image->raw_data[desc.slice_pitch * layer],
      border_color<libimg::Int4>(desc.channel_order), int4_reader());
}

libimg::Int4 __Codeplay_read_imagei_1d(Image *image, Sampler sampler,
                                       libimg::Int coord) {
  // CLK_NORMALIZED_COORDS_TRUE with int coordinate are not valid.
  if (get_sampler_normalized_coords(sampler)) {
    return libimg::make<libimg::Int4>(0, 0, 0, 0);
  }
  return __Codeplay_read_imagei_1d(image, sampler,
                                   static_cast<libimg::Float>(coord));
}

libimg::Int4 __Codeplay_read_imagei_1d(Image *image, Sampler sampler,
                                       libimg::Float coord) {
  ImageMetaData &desc = image->meta_data;
  return image_1d_sampler_read_helper<libimg::Int4, libimg::Int>(
      coord, sampler, desc, image->raw_data,
      border_color<libimg::Int4>(desc.channel_order), int4_reader());
}

libimg::UInt4 __Codeplay_read_imageui_3d(Image *image, Sampler sampler,
                                         libimg::Int4 coord) {
  // CLK_NORMALIZED_COORDS_TRUE with int coordinate are not valid.
  if (get_sampler_normalized_coords(sampler)) {
    return libimg::make<libimg::UInt4>(0u, 0u, 0u, 0u);
  }
  switch (get_sampler_addressing_mode(sampler)) {
    default:
      return libimg::make<libimg::UInt4>(0u, 0u, 0u, 0u);
    case CLK_ADDRESS_CLAMP_TO_EDGE:
    case CLK_ADDRESS_CLAMP:
    case CLK_ADDRESS_NONE:  // libimg::Intentional fall-through;
      break;
  }
  libimg::Float4 f_coord = libimg::convert_float4(coord);
  return __Codeplay_read_imageui_3d(image, sampler, f_coord);
}

libimg::UInt4 __Codeplay_read_imageui_3d(Image *image, Sampler sampler,
                                         libimg::Float4 coord) {
  ImageMetaData &desc = image->meta_data;
  return image_3d_sampler_read_helper<libimg::UInt4, libimg::UInt>(
      coord, sampler, desc, image->raw_data,
      border_color<libimg::UInt4>(desc.channel_order), uint4_reader());
}

libimg::UInt4 __Codeplay_read_imageui_2d_array(Image *image, Sampler sampler,
                                               libimg::Int4 coord) {
  // CLK_NORMALIZED_COORDS_TRUE with int coordinate are not valid.
  if (get_sampler_normalized_coords(sampler)) {
    return libimg::make<libimg::UInt4>(0u, 0u, 0u, 0u);
  }
  libimg::Float4 f_coord = libimg::convert_float4(coord);
  return __Codeplay_read_imageui_2d_array(image, sampler, f_coord);
}

libimg::UInt4 __Codeplay_read_imageui_2d_array(Image *image, Sampler sampler,
                                               libimg::Float4 coord) {
  ImageMetaData &desc = image->meta_data;
  const libimg::Int array_size = desc.array_size;
  libimg::Float w = libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::z);
  const libimg::Float array_mix_idx = array_size - 1;
  libimg::Float layer_f = libimg::floor(w + 0.5f);
  layer_f = layer_f > array_mix_idx ? array_mix_idx : layer_f;
  layer_f = layer_f < 0.0f ? 0.0f : layer_f;
  const libimg::Int layer = libimg::convert_int_rte(layer_f);
  libimg::Float2 coord_2d;
  libimg::set_v2(coord_2d,
                 libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::x),
                 libimg::vec_elem::x);
  libimg::set_v2(coord_2d,
                 libimg::get_v4<libimg::Float>(coord, libimg::vec_elem::y),
                 libimg::vec_elem::y);
  return image_2d_sampler_read_helper<libimg::UInt4, libimg::UInt>(
      coord_2d, sampler, desc, &image->raw_data[desc.slice_pitch * layer],
      border_color<libimg::UInt4>(desc.channel_order), uint4_reader());
}

libimg::UInt4 __Codeplay_read_imageui_2d(Image *image, Sampler sampler,
                                         libimg::Int2 coord) {
  // CLK_NORMALIZED_COORDS_TRUE with int coordinate are not valid.
  if (get_sampler_normalized_coords(sampler)) {
    return libimg::make<libimg::UInt4>(0u, 0u, 0u, 0u);
  }
  switch (get_sampler_addressing_mode(sampler)) {
    default:
      return libimg::make<libimg::UInt4>(0u, 0u, 0u, 0u);
    case CLK_ADDRESS_CLAMP_TO_EDGE:
    case CLK_ADDRESS_CLAMP:
    case CLK_ADDRESS_NONE:  // libimg::Intentional fall-through;
      break;
  }
  libimg::Float2 f_coord = libimg::convert_float2(coord);
  return __Codeplay_read_imageui_2d(image, sampler, f_coord);
}

libimg::UInt4 __Codeplay_read_imageui_2d(Image *image, Sampler sampler,
                                         libimg::Float2 coord) {
  ImageMetaData &desc = image->meta_data;
  return image_2d_sampler_read_helper<libimg::UInt4, libimg::UInt>(
      coord, sampler, desc, image->raw_data,
      border_color<libimg::UInt4>(desc.channel_order), uint4_reader());
}

libimg::UInt4 __Codeplay_read_imageui_1d_array(Image *image, Sampler sampler,
                                               libimg::Int2 coord) {
  // CLK_NORMALIZED_COORDS_TRUE with int coordinate are not valid.
  if (get_sampler_normalized_coords(sampler)) {
    return libimg::make<libimg::UInt4>(0u, 0u, 0u, 0u);
  }
  libimg::Float2 f_coord = libimg::convert_float2(coord);
  return __Codeplay_read_imageui_1d_array(image, sampler, f_coord);
}

libimg::UInt4 __Codeplay_read_imageui_1d_array(Image *image, Sampler sampler,
                                               libimg::Float2 coord) {
  ImageMetaData &desc = image->meta_data;
  const libimg::Int array_size = desc.array_size;
  const libimg::Float v =
      libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::y);
  const libimg::Float array_mix_idx = array_size - 1;
  libimg::Float layer_f = libimg::floor(v + 0.5f);
  layer_f = layer_f > array_mix_idx ? array_mix_idx : layer_f;
  layer_f = layer_f < 0.0f ? 0.0f : layer_f;
  const libimg::Int layer = libimg::convert_int_rte(layer_f);
  return image_1d_sampler_read_helper<libimg::UInt4, libimg::UInt>(
      libimg::get_v2<libimg::Float>(coord, libimg::vec_elem::x), sampler, desc,
      &image->raw_data[desc.slice_pitch * layer],
      border_color<libimg::UInt4>(desc.channel_order), uint4_reader());
}

libimg::UInt4 __Codeplay_read_imageui_1d(Image *image, Sampler sampler,
                                         libimg::Int coord) {
  // CLK_NORMALIZED_COORDS_TRUE with int coordinate are not valid.
  if (get_sampler_normalized_coords(sampler)) {
    return libimg::make<libimg::UInt4>(0u, 0u, 0u, 0u);
  }
  return __Codeplay_read_imageui_1d(image, sampler,
                                    static_cast<libimg::Float>(coord));
}

libimg::UInt4 __Codeplay_read_imageui_1d(Image *image, Sampler sampler,
                                         libimg::Float coord) {
  ImageMetaData &desc = image->meta_data;
  return image_1d_sampler_read_helper<libimg::UInt4, libimg::UInt>(
      coord, sampler, desc, image->raw_data,
      border_color<libimg::UInt4>(desc.channel_order), uint4_reader());
}

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Samplerless read image.                                                   */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
libimg::Float4 __Codeplay_read_imagef_3d(Image *image, libimg::Int4 coord) {
  ImageMetaData &desc = image->meta_data;
  const void *data =
      &image->raw_data[desc.pixel_size * libimg::get_v4<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v4<libimg::Int>(
                                            coord, libimg::vec_elem::y) +
                       desc.slice_pitch * libimg::get_v4<libimg::Int>(
                                              coord, libimg::vec_elem::z)];
  return float4_reader::read(data, desc.channel_order, desc.channel_type);
}

libimg::Float4 __Codeplay_read_imagef_2d_array(Image *image,
                                               libimg::Int4 coord) {
  ImageMetaData &desc = image->meta_data;
  const void *data =
      &image->raw_data[desc.pixel_size * libimg::get_v4<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v4<libimg::Int>(
                                            coord, libimg::vec_elem::y) +
                       desc.slice_pitch * libimg::get_v4<libimg::Int>(
                                              coord, libimg::vec_elem::z)];
  return float4_reader::read(data, desc.channel_order, desc.channel_type);
}

libimg::Float4 __Codeplay_read_imagef_2d(Image *image, libimg::Int2 coord) {
  ImageMetaData &desc = image->meta_data;
  const void *data =
      &image->raw_data[desc.pixel_size * libimg::get_v2<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v2<libimg::Int>(
                                            coord, libimg::vec_elem::y)];
  return float4_reader::read(data, desc.channel_order, desc.channel_type);
}

libimg::Float4 __Codeplay_read_imagef_1d_array(Image *image,
                                               libimg::Int2 coord) {
  ImageMetaData &desc = image->meta_data;
  const void *data =
      &image->raw_data[desc.slice_pitch * libimg::get_v2<libimg::Int>(
                                              coord, libimg::vec_elem::y) +
                       desc.pixel_size * libimg::get_v2<libimg::Int>(
                                             coord, libimg::vec_elem::x)];
  return float4_reader::read(data, desc.channel_order, desc.channel_type);
}

libimg::Float4 __Codeplay_read_imagef_1d(Image *image, libimg::Int coord) {
  ImageMetaData &desc = image->meta_data;
  const void *data = &image->raw_data[desc.pixel_size * coord];
  return float4_reader::read(data, desc.channel_order, desc.channel_type);
}

libimg::Int4 __Codeplay_read_imagei_3d(Image *image, libimg::Int4 coord) {
  ImageMetaData &desc = image->meta_data;
  const void *data =
      &image->raw_data[desc.pixel_size * libimg::get_v4<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v4<libimg::Int>(
                                            coord, libimg::vec_elem::y) +
                       desc.slice_pitch * libimg::get_v4<libimg::Int>(
                                              coord, libimg::vec_elem::z)];
  return int4_reader::read(data, desc.channel_order, desc.channel_type);
}

libimg::Int4 __Codeplay_read_imagei_2d_array(Image *image, libimg::Int4 coord) {
  ImageMetaData &desc = image->meta_data;
  const void *data =
      &image->raw_data[desc.pixel_size * libimg::get_v4<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v4<libimg::Int>(
                                            coord, libimg::vec_elem::y) +
                       desc.slice_pitch * libimg::get_v4<libimg::Int>(
                                              coord, libimg::vec_elem::z)];
  return int4_reader::read(data, desc.channel_order, desc.channel_type);
}

libimg::Int4 __Codeplay_read_imagei_2d(Image *image, libimg::Int2 coord) {
  ImageMetaData &desc = image->meta_data;
  const void *data =
      &image->raw_data[desc.pixel_size * libimg::get_v2<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v2<libimg::Int>(
                                            coord, libimg::vec_elem::y)];
  return int4_reader::read(data, desc.channel_order, desc.channel_type);
}

libimg::Int4 __Codeplay_read_imagei_1d_array(Image *image, libimg::Int2 coord) {
  ImageMetaData &desc = image->meta_data;
  const void *data =
      &image->raw_data[desc.slice_pitch * libimg::get_v2<libimg::Int>(
                                              coord, libimg::vec_elem::y) +
                       desc.pixel_size * libimg::get_v2<libimg::Int>(
                                             coord, libimg::vec_elem::x)];
  return int4_reader::read(data, desc.channel_order, desc.channel_type);
}

libimg::Int4 __Codeplay_read_imagei_1d(Image *image, libimg::Int coord) {
  ImageMetaData &desc = image->meta_data;
  const void *data = &image->raw_data[desc.pixel_size * coord];
  return int4_reader::read(data, desc.channel_order, desc.channel_type);
}

libimg::UInt4 __Codeplay_read_imageui_3d(Image *image, libimg::Int4 coord) {
  ImageMetaData &desc = image->meta_data;
  const void *data =
      &image->raw_data[desc.pixel_size * libimg::get_v4<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v4<libimg::Int>(
                                            coord, libimg::vec_elem::y) +
                       desc.slice_pitch * libimg::get_v4<libimg::Int>(
                                              coord, libimg::vec_elem::z)];
  return uint4_reader::read(data, desc.channel_order, desc.channel_type);
}

libimg::UInt4 __Codeplay_read_imageui_2d_array(Image *image,
                                               libimg::Int4 coord) {
  ImageMetaData &desc = image->meta_data;
  const void *data =
      &image->raw_data[desc.pixel_size * libimg::get_v4<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v4<libimg::Int>(
                                            coord, libimg::vec_elem::y) +
                       desc.slice_pitch * libimg::get_v4<libimg::Int>(
                                              coord, libimg::vec_elem::z)];
  return uint4_reader::read(data, desc.channel_order, desc.channel_type);
}

libimg::UInt4 __Codeplay_read_imageui_2d(Image *image, libimg::Int2 coord) {
  ImageMetaData &desc = image->meta_data;
  const void *data =
      &image->raw_data[desc.pixel_size * libimg::get_v2<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v2<libimg::Int>(
                                            coord, libimg::vec_elem::y)];
  return uint4_reader::read(data, desc.channel_order, desc.channel_type);
}

libimg::UInt4 __Codeplay_read_imageui_1d_array(Image *image,
                                               libimg::Int2 coord) {
  ImageMetaData &desc = image->meta_data;
  const void *data =
      &image->raw_data[desc.slice_pitch * libimg::get_v2<libimg::Int>(
                                              coord, libimg::vec_elem::y) +
                       desc.pixel_size * libimg::get_v2<libimg::Int>(
                                             coord, libimg::vec_elem::x)];
  return uint4_reader::read(data, desc.channel_order, desc.channel_type);
}

libimg::UInt4 __Codeplay_read_imageui_1d(Image *image, libimg::Int coord) {
  ImageMetaData &desc = image->meta_data;
  const void *data = &image->raw_data[desc.pixel_size * coord];
  return uint4_reader::read(data, desc.channel_order, desc.channel_type);
}

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Write image.                                                              */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
// 3d writes are OpenCL extension.
void __Codeplay_write_imagef_3d(Image *image, libimg::Int4 coord,
                                libimg::Float4 color) {
  ImageMetaData &desc = image->meta_data;
  libimg::UChar *data =
      &image->raw_data[desc.pixel_size * libimg::get_v4<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v4<libimg::Int>(
                                            coord, libimg::vec_elem::y) +
                       desc.slice_pitch * libimg::get_v4<libimg::Int>(
                                              coord, libimg::vec_elem::z)];
  float4_writer::write(data, color, desc.channel_order, desc.channel_type);
}

void __Codeplay_write_imagef_2d_array(Image *image, libimg::Int4 coord,
                                      libimg::Float4 color) {
  ImageMetaData &desc = image->meta_data;
  libimg::UChar *data =
      &image->raw_data[desc.pixel_size * libimg::get_v4<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v4<libimg::Int>(
                                            coord, libimg::vec_elem::y) +
                       desc.slice_pitch * libimg::get_v4<libimg::Int>(
                                              coord, libimg::vec_elem::z)];
  float4_writer::write(data, color, desc.channel_order, desc.channel_type);
}

void __Codeplay_write_imagef_2d(Image *image, libimg::Int2 coord,
                                libimg::Float4 color) {
  ImageMetaData &desc = image->meta_data;
  libimg::UChar *data =
      &image->raw_data[desc.pixel_size * libimg::get_v2<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v2<libimg::Int>(
                                            coord, libimg::vec_elem::y)];
  float4_writer::write(data, color, desc.channel_order, desc.channel_type);
}

void __Codeplay_write_imagef_1d_array(Image *image, libimg::Int2 coord,
                                      libimg::Float4 color) {
  ImageMetaData &desc = image->meta_data;
  libimg::UChar *data =
      &image->raw_data[desc.slice_pitch * libimg::get_v2<libimg::Int>(
                                              coord, libimg::vec_elem::y) +
                       desc.pixel_size * libimg::get_v2<libimg::Int>(
                                             coord, libimg::vec_elem::x)];
  float4_writer::write(data, color, desc.channel_order, desc.channel_type);
}

void __Codeplay_write_imagef_1d(Image *image, libimg::Int coord,
                                libimg::Float4 color) {
  ImageMetaData &desc = image->meta_data;
  libimg::UChar *data = &image->raw_data[desc.pixel_size * coord];
  float4_writer::write(data, color, desc.channel_order, desc.channel_type);
}

// 3d writes are OpenCL extension.
void __Codeplay_write_imagei_3d(Image *image, libimg::Int4 coord,
                                libimg::Int4 color) {
  ImageMetaData &desc = image->meta_data;
  libimg::UChar *data =
      &image->raw_data[desc.pixel_size * libimg::get_v4<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v4<libimg::Int>(
                                            coord, libimg::vec_elem::y) +
                       desc.slice_pitch * libimg::get_v4<libimg::Int>(
                                              coord, libimg::vec_elem::z)];
  int4_writer::write(data, color, desc.channel_order, desc.channel_type);
}

void __Codeplay_write_imagei_2d_array(Image *image, libimg::Int4 coord,
                                      libimg::Int4 color) {
  ImageMetaData &desc = image->meta_data;
  libimg::UChar *data =
      &image->raw_data[desc.pixel_size * libimg::get_v4<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v4<libimg::Int>(
                                            coord, libimg::vec_elem::y) +
                       desc.slice_pitch * libimg::get_v4<libimg::Int>(
                                              coord, libimg::vec_elem::z)];
  int4_writer::write(data, color, desc.channel_order, desc.channel_type);
}

void __Codeplay_write_imagei_2d(Image *image, libimg::Int2 coord,
                                libimg::Int4 color) {
  ImageMetaData &desc = image->meta_data;
  libimg::UChar *data =
      &image->raw_data[desc.pixel_size * libimg::get_v2<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v2<libimg::Int>(
                                            coord, libimg::vec_elem::y)];
  int4_writer::write(data, color, desc.channel_order, desc.channel_type);
}

void __Codeplay_write_imagei_1d_array(Image *image, libimg::Int2 coord,
                                      libimg::Int4 color) {
  ImageMetaData &desc = image->meta_data;
  libimg::UChar *data =
      &image->raw_data[desc.slice_pitch * libimg::get_v2<libimg::Int>(
                                              coord, libimg::vec_elem::y) +
                       desc.pixel_size * libimg::get_v2<libimg::Int>(
                                             coord, libimg::vec_elem::x)];
  int4_writer::write(data, color, desc.channel_order, desc.channel_type);
}

void __Codeplay_write_imagei_1d(Image *image, libimg::Int coord,
                                libimg::Int4 color) {
  ImageMetaData &desc = image->meta_data;
  libimg::UChar *data = &image->raw_data[desc.pixel_size * coord];
  int4_writer::write(data, color, desc.channel_order, desc.channel_type);
}

// 3d writes are OpenCL extension.
void __Codeplay_write_imageui_3d(Image *image, libimg::Int4 coord,
                                 libimg::UInt4 color) {
  ImageMetaData &desc = image->meta_data;
  libimg::UChar *data =
      &image->raw_data[desc.pixel_size * libimg::get_v4<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v4<libimg::Int>(
                                            coord, libimg::vec_elem::y) +
                       desc.slice_pitch * libimg::get_v4<libimg::Int>(
                                              coord, libimg::vec_elem::z)];
  uint4_writer::write(data, color, desc.channel_order, desc.channel_type);
}

void __Codeplay_write_imageui_2d_array(Image *image, libimg::Int4 coord,
                                       libimg::UInt4 color) {
  ImageMetaData &desc = image->meta_data;
  libimg::UChar *data =
      &image->raw_data[desc.pixel_size * libimg::get_v4<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v4<libimg::Int>(
                                            coord, libimg::vec_elem::y) +
                       desc.slice_pitch * libimg::get_v4<libimg::Int>(
                                              coord, libimg::vec_elem::z)];
  uint4_writer::write(data, color, desc.channel_order, desc.channel_type);
}

void __Codeplay_write_imageui_2d(Image *image, libimg::Int2 coord,
                                 libimg::UInt4 color) {
  ImageMetaData &desc = image->meta_data;
  libimg::UChar *data =
      &image->raw_data[desc.pixel_size * libimg::get_v2<libimg::Int>(
                                             coord, libimg::vec_elem::x) +
                       desc.row_pitch * libimg::get_v2<libimg::Int>(
                                            coord, libimg::vec_elem::y)];
  uint4_writer::write(data, color, desc.channel_order, desc.channel_type);
}

void __Codeplay_write_imageui_1d_array(Image *image, libimg::Int2 coord,
                                       libimg::UInt4 color) {
  ImageMetaData &desc = image->meta_data;
  libimg::UChar *data =
      &image->raw_data[desc.slice_pitch * libimg::get_v2<libimg::Int>(
                                              coord, libimg::vec_elem::y) +
                       desc.pixel_size * libimg::get_v2<libimg::Int>(
                                             coord, libimg::vec_elem::x)];
  uint4_writer::write(data, color, desc.channel_order, desc.channel_type);
}

void __Codeplay_write_imageui_1d(Image *image, libimg::Int coord,
                                 libimg::UInt4 color) {
  ImageMetaData &desc = image->meta_data;
  libimg::UChar *data = &image->raw_data[desc.pixel_size * coord];
  uint4_writer::write(data, color, desc.channel_order, desc.channel_type);
}

// Image Query Functions
libimg::Int __Codeplay_get_image_width(Image *image) {
  return image->meta_data.width;
}

libimg::Int __Codeplay_get_image_height(Image *image) {
  return image->meta_data.height;
}

libimg::Int __Codeplay_get_image_depth(Image *image) {
  return image->meta_data.depth;
}

libimg::Int __Codeplay_get_image_channel_data_type(Image *image) {
  return image->meta_data.channel_type;
}

libimg::Int __Codeplay_get_image_channel_order(Image *image) {
  return image->meta_data.channel_order;
}

libimg::Int2 __Codeplay_get_image_dim_vec2(Image *image) {
  libimg::Int2 res;
  libimg::set_v2(res, image->meta_data.width, libimg::vec_elem::x);
  libimg::set_v2(res, image->meta_data.height, libimg::vec_elem::y);
  return res;
}

libimg::Int4 __Codeplay_get_image_dim_vec4(Image *image) {
  libimg::Int4 res;
  libimg::set_v4(res, image->meta_data.width, libimg::vec_elem::x);
  libimg::set_v4(res, image->meta_data.height, libimg::vec_elem::y);
  libimg::set_v4(res, image->meta_data.depth, libimg::vec_elem::z);
  libimg::set_v4(res, 0, libimg::vec_elem::w);
  return res;
}

libimg::Size __Codeplay_get_image_array_size(Image *image) {
  return image->meta_data.array_size;
}
