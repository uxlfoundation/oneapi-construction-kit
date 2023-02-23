// Copyright (C) Codeplay Software Limited. All Rights Reserved.

char8 shuffle_helper(char8 source) { return source; }

__kernel void shuffle_function_call(__global char8* source,
                                    __global char16* dest) {
  char16 tmp = 0;
  tmp.S5B = shuffle_helper(source[0]).s37;
  dest[0] = tmp;
}
