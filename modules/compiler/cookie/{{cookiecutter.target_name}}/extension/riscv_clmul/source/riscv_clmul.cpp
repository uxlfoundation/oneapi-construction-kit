 // Copyright (C) Codeplay Software Limited. All Rights Reserved.

 #include <extension/riscv_clmul.h>
 
 extension::riscv_clmul::riscv_clmul()
     : extension("cl_riscv_clmul",
                 usage_category::DEVICE CA_CL_EXT_VERSION(1, 0, 0)) {}
