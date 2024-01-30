# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(abacus_math_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/acos.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/acosh.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/acospi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/asin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/asinh.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/asinpi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/atan2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/atan2pi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/atan.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/atanh.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/atanpi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cbrt.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ceil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/copysign.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cos.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cosh.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cospi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/erfc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/erf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/exp10.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/exp2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/exp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/expm1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fabs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fdim.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/floor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fma.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fmax.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fmin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fmod.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fract.cl
  ${CMAKE_CURRENT_SOURCE_DIR}/fract.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/frexp.cl
  ${CMAKE_CURRENT_SOURCE_DIR}/frexp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/half_cos.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/half_divide.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/half_exp10.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/half_exp2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/half_exp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/half_log10.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/half_log2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/half_log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/half_powr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/half_recip.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/half_rsqrt.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/half_sin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/half_sqrt.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/half_tan.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hypot.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ilogb.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ldexp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lgamma.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lgamma_r.cl
  ${CMAKE_CURRENT_SOURCE_DIR}/lgamma_r.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/log10.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/log1p.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/log2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/logb.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mad.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/maxmag.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/minmag.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/modf.cl
  ${CMAKE_CURRENT_SOURCE_DIR}/modf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/nan.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/native_cos.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/native_divide.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/native_exp10.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/native_exp2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/native_exp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/native_log10.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/native_log2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/native_log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/native_powr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/native_recip.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/native_rsqrt.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/native_sin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/native_sqrt.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/native_tan.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/nextafter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pown.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/powr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/remainder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/remquo.cl
  ${CMAKE_CURRENT_SOURCE_DIR}/remquo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rint.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rootn.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/round.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rsqrt.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sincos.cl
  ${CMAKE_CURRENT_SOURCE_DIR}/sincos.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sinh.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sinpi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sqrt.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tan.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tanh.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tanpi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tgamma.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/trunc.cpp)
set(abacus_math_sources_host ${abacus_math_sources}
  ${CMAKE_CURRENT_SOURCE_DIR}/inplace_fma.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/inplace_sqrt.cpp
  PARENT_SCOPE)
set(abacus_math_sources_device ${abacus_math_sources}
  ${CMAKE_CURRENT_SOURCE_DIR}/inplace_fma.ll.in
  ${CMAKE_CURRENT_SOURCE_DIR}/inplace_sqrt.ll.in
  PARENT_SCOPE)
