// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// This file is used to check that the Khronos clang SPIR generator whilst
// configuring CMake when the CA_EXTERNAL_KHRONOS_CLANG option is specified.
// Specifically it checks if the compiler was built with assertions enabled,
// which improves the readability of the IR disassembled from the bitcode.
void kernel spir_generator_check() {}
