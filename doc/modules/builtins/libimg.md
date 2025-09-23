# Image Library

The Codeplay image library provides a CPU implementation of the image specific
OpenCL functionality, this includes both OpenCL API (host) and OpenCL C (kernel)
side operations. The image library is designed to be integrated into a client
OpenCL or SYCL implementation which must fulfil the required external
dependencies.

## Tools

The tools in this list are the minimum versions which have been tested and known
to work correctly, newer versions should also work as expected but have not been
tested at the time of writing.

* cmake 2.8.12.2
* C++11 compiler toolchain
  * Visual C++ 12
  * gcc 4.8.1
  * clang 3.6.0

## Client Integration

The image library requires that the client provides an integration header, this
header must be named `image_library_integration.h`. The image library also
requires the client to provide the include directory for the integration header
as well as the path to the `CL/cl.h` OpenCL API header. These can be provided
using the following CMake variables.

```
-DCODEPLAY_IMG_INTEGRATION_HEADER_PATH=</path/to/image_library_integration.h>
-DCODEPLAY_IMG_INTEGRATION_INCLUDE_DIRS=</path/to/OpenCL/dir>;</path/to/integration/dir>
```

The integration header must provide three distinct groups of symbols, the
required types, vector access functions, and a small number of OpenCL math
built-in functions. These are outlined in the following sections.

### Header Types

The integration header is required to provide the following type definitions,
these are used throughout the image library.

```cpp
namespace libimg {
typedef <client char> Char;
typedef <client char2> Char2;
typedef <client char4> Char4;

typedef <client uchar> UChar;
typedef <client uchar2> UChar2;
typedef <client uchar4> UChar4;

typedef <client short> Short;
typedef <client short2> Short2;
typedef <client short4> Short4;

typedef <client ushort> UShort;
typedef <client ushort2> UShort2;
typedef <client ushort4> UShort4;

typedef <client int> Int;
typedef <client int2> Int2;
typedef <client int4> Int4;

typedef <client uint> UInt;
typedef <client uint2> UInt2;
typedef <client uint4> UInt4;

typedef <client float> Float;
typedef <client float2> Float2;
typedef <client float4> Float4;

typedef <client ushort> Half;
typedef <client ushort4> Half4;

typedef <client size_t> Size;
}  // libimg
```

### Header Vector Access Functions

The image library also requires a mechanism to access elements in the vector
types described above. The client must provide the following template functions
in the integration header.

```cpp
namespace libimg {
enum class vec_elem { x = 0, y = 1, z = 2, w = 3 };

// Construct a two element vector.
template <typename VecType, typename ElemType>
inline VecType make(ElemType x, ElemType y);

// Construct a four element vector.
template <typename VecType, typename ElemType>
inline VecType make(ElemType x, ElemType y, ElemType z, ElemType w);

// Get an element of a two element vector.
template <typename ElemType, typename VecType>
inline ElemType get_v2(const VecType &v, const vec_elem elem);

// Get an element of a four element vector.
template <typename ElemType, typename VecType>
inline ElemType get_v4(const VecType &v, const vec_elem elem);

// Set an element of a two element vector.
template <typename VecType, typename ElemType>
inline void set_v2(VecType &v, const ElemType val, const vec_elem elem);

// Set an element of a four element vector.
template <typename VecType, typename ElemType>
inline void set_v4(VecType &v, const ElemType val, const vec_elem elem);
}  // libimg
```

### Header Math Built-in Functions

```cpp
namespace libimg {
template <typename Type>
    inline Type clamp(Type x, Type minval, Type maxval);

template <typename Type>
    inline Type fabs(Type value);

template <typename Type>
    inline Type floor(Type value);

inline Int isinf(Float value);

template <typename Type>
    inline Type min(Type a, Type b);

template <typename Type>
    inline Type max(Type a, Type b);

template <typename Type>
    inline Type rint(Type value);

template <typename Type>
    inline Type pow(Type value, Type power);

template <typename Type>
    inline Char convert_char_sat(Type value);

template <typename Type>
    inline Char convert_char_sat_rte(Type value);

template <typename Type>
    inline Char4 convert_char4_sat(Type value);

template <typename Type>
    inline UChar convert_uchar_sat(Type value);

template <typename Type>
    inline UChar convert_uchar_sat_rte(Type value);

template <typename Type>
    inline UChar4 convert_uchar4_sat(Type value);

template <typename Type>
    inline Short convert_short_sat(Type value);

template <typename Type>
    inline Short convert_short_sat_rte(Type value);

template <typename Type>
    inline Short4 convert_short4_sat(Type value);

template <typename Type>
    inline UShort convert_ushort_sat(Type value);

template <typename Type>
    inline UShort convert_ushort_sat_rte(Type value);

template <typename Type>
    inline UShort4 convert_ushort4_sat(Type value);

template <typename Type>
    inline Int convert_int_rte(Type value);

template <typename Type>
    inline Float2 convert_float2(Type value);

template <typename Type>
    inline Float4 convert_float4(Type value);

inline UShort convert_float_to_half(Float arg);

inline Float convert_half_to_float(UShort arg);

inline UShort4 convert_float4_to_half4_rte(Float4 arg);
}  // libimg
```

## Options

### Build Kernel Library

The image library provides the option to build the `image_library_kernel` static
library CMake target by enabling the `CODEPLAY_IMG_BUILD_KERNEL_LIBRARY` option,
this is disabled by default.

```
-DCODEPLAY_IMG_BUILD_KERNEL_LIBRARY=ON
```

### Export Kernel Source Variables

The image library provides the option to set global cached CMake variables
pointing to the kernel side source files by enabling the option
`CODEPLAY_IMG_EXPORT_KERNEL_SOURCES`, this is disabled by default.

```
-DCODEPLAY_IMG_EXPORT_KERNEL_SOURCES=ON
```

This results in the following CMake variable being set.

* `CODEPLAY_IMG_INCLUDE_DIR` is the path to the image libraries include
  directory.
* `CODEPLAY_IMG_KERNEL_HEADER_PATH` is the path to the kernel side header file.
* `CODEPLAY_IMG_KERNEL_SOURCE_PATH` is the path to the kernel side source file.

### Enable Documentation

The image library provides Doxygen documentation, to enable generation of this
documentation set the `CODEPLAY_IMG_ENABLE_DOCUMENTATION` CMake variable, this
is disabled by default. Note that documentation generation requires Doxygen to
be installed on system, if Doxygen is not available then no documentation will
be generated.

```
-DCODEPLAY_IMG_ENABLE_DOCUMENTATION=ON
```
