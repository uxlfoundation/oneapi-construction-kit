// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "GLSLTestDefs.h"

class op_glsl_Barrier : public GlslBuiltinTest<glsl::intTy, glsl::intTy> {
 public:
  op_glsl_Barrier()
      : GlslBuiltinTest<glsl::intTy, glsl::intTy>(
            uvk::Shader::op_glsl_Barrier) {}
};

TEST_F(op_glsl_Barrier, Smoke) { RunWithArgs(1); }
