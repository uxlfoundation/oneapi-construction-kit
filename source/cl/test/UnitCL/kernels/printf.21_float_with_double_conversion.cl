// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// If a device supports doubles, then printf will implicitly convert floats
// to doubles. If the device does not support doubles then this implicit
// conversion will not happen. However, as we generate and commit SPIR-V on
// a system that does support doubles, that implicit conversion is
// included in the SPIR-V bytecode and is used as part of those tests, even if
// the device doesn't support FP64 (such as Windows).
//
// To work around this issue, we disabled the fp64 extension in a number of
// printf tests that use floats. However, to avoid losing test coverage of this
// behaviour, this test will **not** suppress this, but instead will be disabled
// on platforms that don't support the FP64 extension.

// REQUIRES: double

__kernel void float_with_double_conversion(void) {
    float value = 4.0f;
    printf("%f\n", value);
}
