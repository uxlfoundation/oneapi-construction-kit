#ifndef ABACUS_INTERNAL_FLOATING_POINT
#define ABACUS_INTERNAL_FLOATING_POINT

#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_type_traits.h>

namespace abacus {
namespace internal {

// Utility struct to represent and manipulate IEEE754 floating point values
template <typename T>
struct FloatingPoint {
  using Type = T;
  using UnsignedType = typename TypeTraits<T>::UnsignedType;
  using SignedType = typename TypeTraits<T>::SignedType;
  using Shape = FPShape<T>;

  UnsignedType Mantissa;
  UnsignedType Exponent;
  UnsignedType Sign;

  FloatingPoint() = default;
  FloatingPoint(Type x) {
    const UnsignedType ux = abacus::detail::cast::as<UnsignedType>(x);

    Exponent = (ux & Shape::ExponentMask()) >> Shape::Mantissa();
    Mantissa = ux & Shape::MantissaMask();
    Sign = (ux & Shape::SignMask()) >> (Shape::Mantissa() + Shape::Exponent());
  }

  Type Get() const {
    const UnsignedType result =
        (Sign << (Shape::Mantissa() + Shape::Exponent())) |
        (Exponent << Shape::Mantissa()) | Mantissa;
    return abacus::detail::cast::as<Type>(result);
  }

  SignedType Bias() const { return (1u << (Shape::Exponent() - 1u)) - 1u; }
  SignedType Denormal() const { return (Exponent == 0u) & (Mantissa != 0u); }
  SignedType Inf() const {
    return (Exponent == Shape::ExponentOnes()) & (Mantissa == 0u);
  }
  SignedType NaN() const {
    return (Exponent == Shape::ExponentOnes()) & (Mantissa != 0u);
  }
  SignedType Zero() const { return (Exponent == 0u) & (Mantissa == 0u); }
};
}  // namespace internal
}  // namespace abacus

#endif
