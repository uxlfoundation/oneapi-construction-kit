// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: images nospirv

void __kernel other_kernel(__global uint *out, __read_only image1d_t img,
                           sampler_t sampler1, sampler_t sampler2) {
  int gid = get_global_id(0);
  float coord = (float)gid;
  coord = coord / (get_global_size(0) / 2.0f) +
          0.05f;  // just to ensure it avoids being right on the edge of a pixel
  // First sampler passed will be repeat
  uint4 pxl1 = read_imageui(img, sampler1, coord);

  // Second sampler passed will be clamp
  uint4 pxl2 = read_imageui(img, sampler2, coord);

  out[gid * 2] = pxl1.x;
  out[gid * 2 + 1] = pxl2.x;
}

void __kernel image_sampler_kernel_call_kernel(__global uint *out,
                                               __read_only image1d_t img,
                                               sampler_t sampler1,
                                               sampler_t sampler2) {
  other_kernel(out, img, sampler1, sampler2);
}
