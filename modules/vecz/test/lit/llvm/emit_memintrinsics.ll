; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k entry -vecz-passes="builtin-inlining,function(instcombine,early-cse),cfg-convert,packetizer" -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; Laid out, this struct is 80 bytes
%struct.S2 = type { i16, [7 x i32], i32, <16 x i8>, [4 x i32] }

; Function Attrs: norecurse nounwind
define spir_kernel void @entry(i64 addrspace(1)* %result, %struct.S2* %result2) {
entry:
  %gid = call i64 @_Z12get_local_idj(i32 0)
  %sa = alloca %struct.S2, align 16
  %sb = alloca %struct.S2, align 16
  %sa_i8 = bitcast %struct.S2* %sa to i8*
  %sb_i8 = bitcast %struct.S2* %sb to i8*
  %sb_i8as = addrspacecast i8* %sb_i8 to i8 addrspace(1)*
  %rsi = ptrtoint i64 addrspace(1)* %result to i64
  %rsit = trunc i64 %rsi to i8
  call void @llvm.memset.p0i8.i64(i8* %sa_i8, i8 %rsit, i64 80, i32 16, i1 false)
  call void @llvm.memset.p1i8.i64(i8 addrspace(1)* %sb_i8as, i8 0, i64 80, i32 16, i1 false)
  %lr = addrspacecast %struct.S2* %result2 to %struct.S2 addrspace(1)*
  %lri = bitcast %struct.S2 addrspace(1)* %lr to i64 addrspace(1)*
  %cond = icmp eq i64 addrspace(1)* %result, %lri
  br i1 %cond, label %middle, label %end

middle:
  call void @llvm.memcpy.p1i8.p0i8.i64(i8 addrspace(1)* %sb_i8as, i8* %sa_i8, i64 80, i32 16, i1 false)
  br label %end

end:
  %g_343 = getelementptr inbounds %struct.S2, %struct.S2* %sa, i64 0, i32 0
  %g_343_load = load i16, i16* %g_343
  %g_343_zext = zext i16 %g_343_load to i64
  %resp = getelementptr i64, i64 addrspace(1)* %result, i64 %gid
  store i64 %g_343_zext, i64 addrspace(1)* %resp, align 8
  %result2_i8 = bitcast %struct.S2* %result2 to i8*
  call void @llvm.memcpy.p0i8.p1i8.i64(i8* %result2_i8, i8 addrspace(1)* %sb_i8as, i64 80, i32 16, i1 false)
  ret void
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1)
declare void @llvm.memset.p1i8.i64(i8 addrspace(1)* nocapture, i8, i64, i32, i1)

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p1i8.p0i8.i64(i8 addrspace(1)* nocapture, i8* nocapture readonly, i64, i32, i1)
declare void @llvm.memcpy.p0i8.p1i8.i64(i8* nocapture, i8 addrspace(1)* nocapture readonly, i64, i32, i1)

declare i64 @_Z12get_local_idj(i32)

; Sanity checks: Make sure the non-vecz entry function is still in place and
; contains memset and memcpy. This is done in order to prevent future bafflement
; in case some pass optimizes them out.
; CHECK: define spir_kernel void @entry
; CHECK: entry:
; CHECK: call void @llvm.memset
; CHECK: call void @llvm.memset
; CHECK: middle:
; CHECK: call void @llvm.memcpy
; CHECK: end:
; CHECK: call void @llvm.memcpy

; And now for the actual checks

; Check if the kernel was vectorized
; CHECK: define spir_kernel void @__vecz_v{{[0-9]+}}_entry
; CHECK-GE15: %[[SB_I8AS:.*]] = addrspacecast ptr %sb to ptr addrspace(1)

; Check if the memset and memcpy calls have been removed
; CHECK-NOT: call void @llvm.memset
; CHECK-NOT: call void @llvm.memcpy

; Check if the calculation of the stored value for the second memset is in place
; CHECK: %ms64val

; Check if the generated loads and stores are in place
; Check the stores for the first memset
; CHECK-LT15: %[[SA_I81:.+]] = bitcast %struct.S2* %sa to i64*
; CHECK-GE15: store i64 %ms64val, ptr %sa, align 16
; CHECK-LT15: store i64 %ms64val, i64* %[[SA_I81]], align 16
; CHECK-GE15: %[[V14:[0-9]+]] = getelementptr inbounds i8, ptr %sa, i64 8
; CHECK-LT15: %[[V14:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sa, i64 0, i32 1, i64 1
; CHECK-LT15: %[[SA_I82:.+]] = bitcast i32* %[[V14]] to i64*
; CHECK-GE15: store i64 %ms64val, ptr %[[V14]], align 8
; CHECK-LT15: store i64 %ms64val, i64* %[[SA_I82]], align 8
; CHECK-GE15: %[[V15:[0-9]+]] = getelementptr inbounds i8, ptr %sa, i64 16
; CHECK-LT15: %[[V15:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sa, i64 0, i32 1, i64 3
; CHECK-LT15: %[[SA_I83:.+]] = bitcast i32* %[[V15]] to i64*
; CHECK-GE15: store i64 %ms64val, ptr %[[V15]], align {{(8|16)}}
; CHECK-LT15: store i64 %ms64val, i64* %[[SA_I83]], align {{(8|16)}}
; CHECK-GE15: %[[V16:[0-9]+]] = getelementptr inbounds i8, ptr %sa, i64 24
; CHECK-LT15: %[[V16:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sa, i64 0, i32 1, i64 5
; CHECK-LT15: %[[SA_I84:.+]] = bitcast i32* %[[V16]] to i64*
; CHECK-GE15: store i64 %ms64val, ptr %[[V16]], align 8
; CHECK-LT15: store i64 %ms64val, i64* %[[SA_I84]], align 8
; CHECK-GE15: %[[V17:[0-9]+]] = getelementptr inbounds i8, ptr %sa, i64 32
; CHECK-LT15: %[[V17:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sa, i64 0, i32 2
; CHECK-LT15: %[[SA_I85:.+]] = bitcast i32* %[[V17]] to i64*
; CHECK-GE15: store i64 %ms64val, ptr %[[V17]], align 16
; CHECK-LT15: store i64 %ms64val, i64* %[[SA_I85]], align 16
; CHECK-GE15: %[[V18:[0-9]+]] = getelementptr inbounds i8, ptr %sa, i64 40
; CHECK-LT15: %[[V18:[0-9]+]] = getelementptr inbounds i8, i8* %sa_i8, i64 40
; CHECK-LT15: %[[SA_I86:.+]] = bitcast i8* %[[V18]] to i64*
; CHECK-GE15: store i64 %ms64val, ptr %[[V18]], align 8
; CHECK-LT15: store i64 %ms64val, i64* %[[SA_I86]], align 8
; CHECK-GE15: %[[V19:[0-9]+]] = getelementptr inbounds i8, ptr %sa, i64 48
; CHECK-LT15: %[[V19:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sa, i64 0, i32 3, i64 0
; CHECK-LT15: %[[SA_I87:.+]] = bitcast i8* %[[V19]] to i64*
; CHECK-GE15: store i64 %ms64val, ptr %[[V19]], align 16
; CHECK-LT15: store i64 %ms64val, i64* %[[SA_I87]], align 16
; CHECK-GE15: %[[V20:[0-9]+]] = getelementptr inbounds i8, ptr %sa, i64 56
; CHECK-EQ14: %[[V20:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sa, i64 0, i32 3, i64 8
; CHECK-LT15: %[[SA_I88:.+]] = bitcast i8* %[[V20]] to i64*
; CHECK-LT15: store i64 %ms64val, i64* %[[SA_I88]], align 8
; CHECK-GE15: %[[V21:[0-9]+]] = getelementptr inbounds i8, ptr %sa, i64 64
; CHECK-LT15: %[[V21:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sa, i64 0, i32 4
; CHECK-LT15: %[[SA_I89:.+]] = bitcast [4 x i32]* %[[V21]] to i64*
; CHECK-LT15: store i64 %ms64val, i64* %[[SA_I89]], align 16
; CHECK-GE15: %[[V22:[0-9]+]] = getelementptr inbounds i8, ptr %sa, i64 72
; CHECK-LT15: %[[V22:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sa, i64 0, i32 4, i64 2
; CHECK-LT15: %[[SA_I810:.+]] = bitcast i32* %[[V22]] to i64*
; CHECK-LT15: store i64 %ms64val, i64* %[[SA_I810]], align 8

; Check the stores for the second memset
; CHECK-LT15: %[[V23:[0-9]+]] = bitcast %struct.S2* %sb to i64*
; CHECK-LT15: %[[SB_I8AS11:.+]] = addrspacecast i64* %[[V23]] to i64 addrspace(1)*
; CHECK-GE15: store i64 0, ptr addrspace(1) %[[SB_I8AS]], align 16
; CHECK-LT15: store i64 0, i64 addrspace(1)* %[[SB_I8AS11]], align 16
; CHECK-GE15: %[[V24:[0-9]+]] = getelementptr inbounds i8, ptr addrspace(1) %[[SB_I8AS]], i64 8
; CHECK-LT15: %[[V24:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sb, i64 0, i32 1, i64 1
; CHECK-LT15: %[[V25:[0-9]+]] = bitcast i32* %[[V24]] to i64*
; CHECK-LT15: %[[SB_I8AS12:.+]] = addrspacecast i64* %[[V25]] to i64 addrspace(1)*
; CHECK-GE15: store i64 0, ptr addrspace(1) %[[V24]], align 8
; CHECK-LT15: store i64 0, i64 addrspace(1)* %[[SB_I8AS12]], align 8
; CHECK-GE15: %[[V26:[0-9]+]] = getelementptr inbounds i8, ptr addrspace(1) %[[SB_I8AS]], i64 16
; CHECK-LT15: %[[V26:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sb, i64 0, i32 1, i64 3
; CHECK-LT15: %[[V27:[0-9]+]] = bitcast i32* %[[V26]] to i64*
; CHECK-LT15: %[[SB_I8AS13:.+]] = addrspacecast i64* %[[V27]] to i64 addrspace(1)*
; CHECK-GE15: store i64 0, ptr addrspace(1) %[[V26]], align 8
; CHECK-LT15: store i64 0, i64 addrspace(1)* %[[SB_I8AS13]], align 8
; CHECK-GE15: %[[V28:[0-9]+]] = getelementptr inbounds i8, ptr addrspace(1) %[[SB_I8AS]], i64 24
; CHECK-LT15: %[[V28:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sb, i64 0, i32 1, i64 5
; CHECK-LT15: %[[V29:[0-9]+]] = bitcast i32* %[[V28]] to i64*
; CHECK-LT15: %[[SB_I8AS14:.+]] = addrspacecast i64* %[[V29]] to i64 addrspace(1)*
; CHECK-GE15: store i64 0, ptr addrspace(1) %[[V28]], align 8
; CHECK-LT15: store i64 0, i64 addrspace(1)* %[[SB_I8AS14]], align 8
; CHECK-GE15: %[[V30:[0-9]+]] = getelementptr inbounds i8, ptr addrspace(1) %[[SB_I8AS]], i64 32
; CHECK-LT15: %[[V30:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sb, i64 0, i32 2
; CHECK-LT15: %[[V31:[0-9]+]] = bitcast i32* %[[V30]] to i64*
; CHECK-LT15: %[[SB_I8AS15:.+]] = addrspacecast i64* %[[V31]] to i64 addrspace(1)*
; CHECK-GE15: store i64 0, ptr addrspace(1) %[[V30]], align 8
; CHECK-LT15: store i64 0, i64 addrspace(1)* %[[SB_I8AS15]], align 8
; CHECK-GE15: %[[V32:[0-9]+]] = getelementptr inbounds i8, ptr addrspace(1) %[[SB_I8AS]], i64 40
; CHECK-LT15: %[[V32:[0-9]+]] = getelementptr inbounds i8, i8 addrspace(1)* %sb_i8as, i64 40
; CHECK-LT15: %[[SB_I8AS16:.+]] = bitcast i8 addrspace(1)* %[[V32]] to i64 addrspace(1)*
; CHECK-GE15: store i64 0, ptr addrspace(1) %[[V32]], align 8
; CHECK-LT15: store i64 0, i64 addrspace(1)* %[[SB_I8AS16]], align 8
; CHECK-GE15: %[[V33:[0-9]+]] = getelementptr inbounds i8, ptr addrspace(1) %[[SB_I8AS]], i64 48
; CHECK-LT15: %[[V33:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sb, i64 0, i32 3, i64 0
; CHECK-LT15: %[[V34:[0-9]+]] = bitcast i8* %[[V33]] to i64*
; CHECK-LT15: %[[SB_I8AS17:.+]] = addrspacecast i64* %[[V34]] to i64 addrspace(1)*
; CHECK-GE15: store i64 0, ptr addrspace(1) %[[V33]], align 8
; CHECK-LT15: store i64 0, i64 addrspace(1)* %[[SB_I8AS17]], align 8
; CHECK-GE15: %[[V35T:[0-9]+]] = getelementptr inbounds i8, ptr addrspace(1) %[[SB_I8AS]], i64 56
; CHECK-EQ14: %[[V35T:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sb, i64 0, i32 3, i64 8
; CHECK-EQ14: %[[V35:[0-9]+]] = bitcast i8* %[[V35T]] to i64*
; CHECK-EQ14: %[[SB_I8AS18:.+]] = addrspacecast i64* %[[V35]] to i64 addrspace(1)*
; CHECK-GE15: store i64 0, ptr addrspace(1) %[[V35T]], align 8
; CHECK-LT15: store i64 0, i64 addrspace(1)* %[[SB_I8AS18]], align 8
; CHECK-GE15: %[[V36:[0-9]+]] = getelementptr inbounds i8, ptr addrspace(1) %[[SB_I8AS]], i64 64
; CHECK-LT15: %[[V36:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sb, i64 0, i32 4
; CHECK-LT15: %[[V37:[0-9]+]] = bitcast [4 x i32]* %[[V36]] to i64*
; CHECK-LT15: %[[SB_I8AS19:.+]] = addrspacecast i64* %[[V37]] to i64 addrspace(1)*
; CHECK-GE15: store i64 0, ptr addrspace(1) %[[V36]], align 8
; CHECK-LT15: store i64 0, i64 addrspace(1)* %[[SB_I8AS19]], align 8
; CHECK-GE15: %[[V38:[0-9]+]] = getelementptr inbounds i8, ptr addrspace(1) %[[SB_I8AS]], i64 72
; CHECK-LT15: %[[V38:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %sb, i64 0, i32 4, i64 2
; CHECK-LT15: %[[V39:[0-9]+]] = bitcast i32* %[[V38]] to i64*
; CHECK-LT15: %[[SB_I8AS20:.+]] = addrspacecast i64* %[[V39]] to i64 addrspace(1)*
; CHECK-GE15: store i64 0, ptr addrspace(1) %[[V38]], align 8
; CHECK-LT15: store i64 0, i64 addrspace(1)* %[[SB_I8AS20]], align 8

; CHECK-LT15: %[[V40:[0-9]+]] = bitcast %struct.S2* %result2 to i64*

; Check the loads and stores for the first memcpy
; CHECK:middle:                                           ; preds = %entry
; CHECK-GE15: %[[SA_I822:.+]] = load i64, ptr %sa, align 16
; CHECK-LT15: %[[SA_I822:.+]] = load i64, i64* %[[SA_I81]], align 16
; CHECK-GE15: store i64 %[[SA_I822]], ptr addrspace(1) %[[SB_I8AS]], align 16
; CHECK-LT15: store i64 %[[SA_I822]], i64 addrspace(1)* %[[SB_I8AS11]], align 16
; CHECK-GE15: %[[SA_I824:.+]] = load i64, ptr %[[V14]], align 8
; CHECK-LT15: %[[SA_I824:.+]] = load i64, i64* %[[SA_I82]], align 8
; CHECK-GE15: store i64 %[[SA_I824]], ptr addrspace(1) %[[V24]], align 8
; CHECK-LT15: store i64 %[[SA_I824]], i64 addrspace(1)* %[[SB_I8AS12]], align 8
; CHECK-GE15: %[[SA_I826:.+]] = load i64, ptr %[[V15]], align {{(8|16)}}
; CHECK-LT15: %[[SA_I826:.+]] = load i64, i64* %[[SA_I83]], align {{(8|16)}}
; CHECK-GE15: store i64 %[[SA_I826]], ptr addrspace(1) %[[V26]], align 8
; CHECK-LT15: store i64 %[[SA_I826]], i64 addrspace(1)* %[[SB_I8AS13]], align 8
; CHECK-GE15: %[[SA_I828:.+]] = load i64, ptr %[[V16]], align 8
; CHECK-LT15: %[[SA_I828:.+]] = load i64, i64* %[[SA_I84]], align 8
; CHECK-GE15: store i64 %[[SA_I828]], ptr addrspace(1) %[[V28]], align 8
; CHECK-LT15: store i64 %[[SA_I828]], i64 addrspace(1)* %[[SB_I8AS14]], align 8
; CHECK-GE15: %[[SA_I830:.+]] = load i64, ptr %[[V17]], align 16
; CHECK-LT15: %[[SA_I830:.+]] = load i64, i64* %[[SA_I85]], align 16
; CHECK-GE15: store i64 %[[SA_I830]], ptr addrspace(1) %[[V30]], align 8
; CHECK-LT15: store i64 %[[SA_I830]], i64 addrspace(1)* %[[SB_I8AS15]], align 8
; CHECK-GE15: %[[SA_I832:.+]] = load i64, ptr %[[V18]], align 8
; CHECK-LT15: %[[SA_I832:.+]] = load i64, i64* %[[SA_I86]], align 8
; CHECK-GE15: store i64 %[[SA_I832]], ptr addrspace(1) %[[V32]], align 8
; CHECK-LT15: store i64 %[[SA_I832]], i64 addrspace(1)* %[[SB_I8AS16]], align 8
; CHECK-GE15: %[[SA_I834:.+]] = load i64, ptr %[[V19]], align 16
; CHECK-LT15: %[[SA_I834:.+]] = load i64, i64* %[[SA_I87]], align 16
; CHECK-GE15: store i64 %[[SA_I834]], ptr addrspace(1) %[[V33]], align 8
; CHECK-LT15: store i64 %[[SA_I834]], i64 addrspace(1)* %[[SB_I8AS17]], align 8
; CHECK-GE15: %[[SA_I836:.+]] = load i64, ptr %[[V20]], align 8
; CHECK-LT15: %[[SA_I836:.+]] = load i64, i64* %[[SA_I88]], align 8
; CHECK-GE15: store i64 %[[SA_I836]], ptr addrspace(1) %[[V35T]], align 8
; CHECK-LT15: store i64 %[[SA_I836]], i64 addrspace(1)* %[[SB_I8AS18]], align 8
; CHECK-GE15: %[[SA_I838:.+]] = load i64, ptr %[[V21]], align 16
; CHECK-LT15: %[[SA_I838:.+]] = load i64, i64* %[[SA_I89]], align 16
; CHECK-GE15: store i64 %[[SA_I838]], ptr addrspace(1) %[[V36]], align 8
; CHECK-LT15: store i64 %[[SA_I838]], i64 addrspace(1)* %[[SB_I8AS19]], align 8
; CHECK-GE15: %[[SA_I840:.+]] = load i64, ptr %[[V22]], align 8
; CHECK-LT15: %[[SA_I840:.+]] = load i64, i64* %[[SA_I810]], align 8
; CHECK-GE15: store i64 %[[SA_I840]], ptr addrspace(1) %[[V38]], align 8
; CHECK-LT15: store i64 %[[SA_I840]], i64 addrspace(1)* %[[SB_I8AS20]], align 8

; Check the loads and stores for the second memcpy
; CHECK:end:                                              ; preds = %middle, %entry
; CHECK-LT15: %[[RESULT2_I8:.+]] = bitcast %struct.S2* %result2 to i8*
; CHECK-GE15: %[[SB_I8AS42:.+]] = load i64, ptr addrspace(1) %[[SB_I8AS]], align 16
; CHECK-LT15: %[[SB_I8AS42:.+]] = load i64, i64 addrspace(1)* %[[SB_I8AS11]], align 16
; CHECK-GE15: store i64 %[[SB_I8AS42]], ptr %result2, align 16
; CHECK-LT15: store i64 %[[SB_I8AS42]], i64* %[[V40]], align 16
; CHECK-GE15: %[[V42:[0-9]+]] = getelementptr inbounds i8, ptr %result2, i64 8
; CHECK-LT15: %[[V42:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %result2, i64 0, i32 1, i64 1
; CHECK-LT15: %[[RESULT2_I843:.+]] = bitcast i32* %[[V42]] to i64*
; CHECK-GE15: %[[SB_I8AS44:.+]] = load i64, ptr addrspace(1) %[[V24]], align 8
; CHECK-LT15: %[[SB_I8AS44:.+]] = load i64, i64 addrspace(1)* %[[SB_I8AS12]], align 8
; CHECK-GE15: store i64 %[[SB_I8AS44]], ptr %[[V42]], align 8
; CHECK-LT15: store i64 %[[SB_I8AS44]], i64* %[[RESULT2_I843]], align 8
; CHECK-GE15: %[[V43:[0-9]+]] = getelementptr inbounds i8, ptr %result2, i64 16
; CHECK-LT15: %[[V43:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %result2, i64 0, i32 1, i64 3
; CHECK-LT15: %[[RESULT2_I845:.+]] = bitcast i32* %[[V43]] to i64*
; CHECK-GE15: %[[SB_I8AS46:.+]] = load i64, ptr addrspace(1) %[[V26]], align 8
; CHECK-LT15: %[[SB_I8AS46:.+]] = load i64, i64 addrspace(1)* %[[SB_I8AS13]], align 8
; CHECK-GE15: store i64 %[[SB_I8AS46]], ptr %[[V43]], align 8
; CHECK-LT15: store i64 %[[SB_I8AS46]], i64* %[[RESULT2_I845]], align 8
; CHECK-GE15: %[[V44:[0-9]+]] = getelementptr inbounds i8, ptr %result2, i64 24
; CHECK-LT15: %[[V44:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %result2, i64 0, i32 1, i64 5
; CHECK-LT15: %[[RESULT2_I847:.+]] = bitcast i32* %[[V44]] to i64*
; CHECK-GE15: %[[SB_I8AS48:.+]] = load i64, ptr addrspace(1) %[[V28]], align 8
; CHECK-LT15: %[[SB_I8AS48:.+]] = load i64, i64 addrspace(1)* %[[SB_I8AS14]], align 8
; CHECK-GE15: store i64 %[[SB_I8AS48]], ptr %[[V44]], align 8
; CHECK-LT15: store i64 %[[SB_I8AS48]], i64* %[[RESULT2_I847]], align 8
; CHECK-GE15: %[[V45:[0-9]+]] = getelementptr inbounds i8, ptr %result2, i64 32
; CHECK-LT15: %[[V45:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %result2, i64 0, i32 2
; CHECK-LT15: %[[RESULT2_I849:.+]] = bitcast i32* %[[V45]] to i64*
; CHECK-GE15: %[[SB_I8AS50:.+]] = load i64, ptr addrspace(1) %[[V30]], align 8
; CHECK-LT15: %[[SB_I8AS50:.+]] = load i64, i64 addrspace(1)* %[[SB_I8AS15]], align 8
; CHECK-GE15: store i64 %[[SB_I8AS50]], ptr %[[V45]], align 8
; CHECK-LT15: store i64 %[[SB_I8AS50]], i64* %[[RESULT2_I849]], align 8
; CHECK-GE15: %[[V46:[0-9]+]] = getelementptr inbounds i8, ptr %result2, i64 40
; CHECK-LT15: %[[V46:[0-9]+]] = getelementptr inbounds i8, i8* %[[RESULT2_I8]], i64 40
; CHECK-LT15: %[[RESULT2_I851:.+]] = bitcast i8* %[[V46]] to i64*
; CHECK-GE15: %[[SB_I8AS52:.+]] = load i64, ptr addrspace(1) %[[V32]], align 8
; CHECK-LT15: %[[SB_I8AS52:.+]] = load i64, i64 addrspace(1)* %[[SB_I8AS16]], align 8
; CHECK-GE15: store i64 %[[SB_I8AS52]], ptr %[[V46]], align 8
; CHECK-LT15: store i64 %[[SB_I8AS52]], i64* %[[RESULT2_I851]], align 8
; CHECK-GE15: %[[V47:[0-9]+]] = getelementptr inbounds i8, ptr %result2, i64 48
; CHECK-LT15: %[[V47:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %result2, i64 0, i32 3, i64 0
; CHECK-LT15: %[[RESULT2_I853:.+]] = bitcast i8* %[[V47]] to i64*
; CHECK-GE15: %[[SB_I8AS54:.+]] = load i64, ptr addrspace(1) %[[V33]], align 8
; CHECK-LT15: %[[SB_I8AS54:.+]] = load i64, i64 addrspace(1)* %[[SB_I8AS17]], align 8
; CHECK-GE15: store i64 %[[SB_I8AS54]], ptr %[[V47]], align 8
; CHECK-LT15: store i64 %[[SB_I8AS54]], i64* %[[RESULT2_I853]], align 8
; CHECK-GE15: %[[V48:[0-9]+]] = getelementptr inbounds i8, ptr %result2, i64 56
; CHECK-EQ14: %[[V48:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %result2, i64 0, i32 3, i64 8
; CHECK-LT15: %[[RESULT2_I855:.+]] = bitcast i8* %[[V48]] to i64*
; CHECK-GE15: %[[SB_I8AS56:.+]] = load i64, ptr addrspace(1) %[[V35T]], align 8
; CHECK-LT15: %[[SB_I8AS56:.+]] = load i64, i64 addrspace(1)* %[[SB_I8AS18]], align 8
; CHECK-GE15: store i64 %[[SB_I8AS56]], ptr %[[V48]], align 8
; CHECK-LT15: store i64 %[[SB_I8AS56]], i64* %[[RESULT2_I855]], align 8
; CHECK-GE15: %[[V49:[0-9]+]] = getelementptr inbounds i8, ptr %result2, i64 64
; CHECK-LT15: %[[V49:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %result2, i64 0, i32 4
; CHECK-LT15: %[[RESULT2_I857:.+]] = bitcast [4 x i32]* %[[V49]] to i64*
; CHECK-GE15: %[[SB_I8AS58:.+]] = load i64, ptr addrspace(1) %[[V36]], align 8
; CHECK-LT15: %[[SB_I8AS58:.+]] = load i64, i64 addrspace(1)* %[[SB_I8AS19]], align 8
; CHECK-GE15: store i64 %[[SB_I8AS58]], ptr %[[V49]], align 8
; CHECK-LT15: store i64 %[[SB_I8AS58]], i64* %[[RESULT2_I857]], align 8
; CHECK-GE15: %[[V50:[0-9]+]] = getelementptr inbounds i8, ptr %result2, i64 72
; CHECK-LT15: %[[V50:[0-9]+]] = getelementptr inbounds %struct.S2, %struct.S2* %result2, i64 0, i32 4, i64 2
; CHECK-LT15: %[[RESULT2_I859:.+]] = bitcast i32* %[[V50]] to i64*
; CHECK-GE15: %[[SB_I8AS60:.+]] = load i64, ptr addrspace(1) %[[V38]], align 8
; CHECK-LT15: %[[SB_I8AS60:.+]] = load i64, i64 addrspace(1)* %[[SB_I8AS20]], align 8
; CHECK-GE15: store i64 %[[SB_I8AS60]], ptr %[[V50]], align 8
; CHECK-LT15: store i64 %[[SB_I8AS60]], i64* %[[RESULT2_I859]], align 8

; End of function
; CHECK: ret void
