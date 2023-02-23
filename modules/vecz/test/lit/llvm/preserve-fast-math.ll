; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -S -vecz-passes=packetizer < %s | %filecheck %s

; CHECK: %{{.*}} = fcmp nnan ninf olt <4 x float> %{{.*}}, %{{.*}}

define spir_kernel void @fast_nan(float addrspace(1)* %src1, float addrspace(1)* %src2, i16 addrspace(1)* %dst, i32 %width) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %src1, i64 %call
  %0 = load float, float addrspace(1)* %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds float, float addrspace(1)* %src2, i64 %call
  %1 = load float, float addrspace(1)* %arrayidx2, align 4
  %cmp = fcmp nnan ninf olt float %0, %1
  %conv4 = zext i1 %cmp to i16
  %arrayidx6 = getelementptr inbounds i16, i16 addrspace(1)* %dst, i64 %call
  store i16 %conv4, i16 addrspace(1)* %arrayidx6, align 2
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)
