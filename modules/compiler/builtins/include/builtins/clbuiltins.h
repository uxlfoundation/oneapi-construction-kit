// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef OCL_CLBUILTINS_H_INCLUDED
#define OCL_CLBUILTINS_H_INCLUDED

#include <abacus/abacus_common.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_extra.h>
#include <abacus/abacus_geometric.h>
#include <abacus/abacus_integer.h>
// Including abacus_math.h results in multiply defined symbols.
// Fixing this would allow us to remove the ABACUS_ENABLE_OPENCL_X_Y_BUILTINS
// defines from the abacus-${triple}${cap_suf}.bc target since currently
// definitions from abacus_math.h are exposed through that file which is linked
// as part of the bitcode linking.
//#include <abacus/abacus_math.h>
#include <abacus/abacus_cast.h>
#include <abacus/abacus_memory.h>
#include <abacus/abacus_misc.h>
#include <abacus/abacus_relational.h>

extern bool __attribute__((const)) __mux_isftz(void);
extern bool __attribute__((const)) __mux_usefast(void);
extern bool __attribute__((const)) __mux_isembeddedprofile(void);
extern size_t __attribute__((pure)) __mux_get_global_size(uint x);
extern size_t __attribute__((pure)) __mux_get_global_id(uint x);
extern size_t __attribute__((pure)) __mux_get_global_offset(uint x);
extern size_t __attribute__((pure)) __mux_get_local_size(uint x);
extern size_t __attribute__((pure)) __mux_get_local_id(uint x);
extern size_t __attribute__((pure)) __mux_get_num_groups(uint x);
extern size_t __attribute__((pure)) __mux_get_group_id(uint x);
extern uint __attribute__((pure)) __mux_get_work_dim(void);

bool __CL_CONST_ATTRIBUTES __abacus_isftz() { return __mux_isftz(); }
bool __CL_CONST_ATTRIBUTES __abacus_usefast() { return __mux_usefast(); }
bool __CL_CONST_ATTRIBUTES __abacus_isembeddedprofile() {
  return __mux_isembeddedprofile();
}

size_t __CL_WORK_ITEM_ATTRIBUTES get_global_size(uint x) {
  return __mux_get_global_size(x);
}

size_t __CL_WORK_ITEM_ATTRIBUTES get_global_id(uint x) {
  return __mux_get_global_id(x);
}

size_t __CL_WORK_ITEM_ATTRIBUTES get_global_offset(uint x) {
  return __mux_get_global_offset(x);
}

size_t __CL_WORK_ITEM_ATTRIBUTES get_local_size(uint x) {
  return __mux_get_local_size(x);
}

size_t __CL_WORK_ITEM_ATTRIBUTES get_local_id(uint x) {
  return __mux_get_local_id(x);
}

size_t __CL_WORK_ITEM_ATTRIBUTES get_num_groups(uint x) {
  return __mux_get_num_groups(x);
}

size_t __CL_WORK_ITEM_ATTRIBUTES get_group_id(uint x) {
  return __mux_get_group_id(x);
}

uint __CL_WORK_ITEM_ATTRIBUTES get_work_dim(void) {
  return __mux_get_work_dim();
}

#endif  // OCL_CLBUILTINS_H_INCLUDED
