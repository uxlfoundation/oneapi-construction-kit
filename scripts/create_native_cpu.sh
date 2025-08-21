#!/bin/bash

# Usage : ./create_native_cpu.sh <ock_repo_path> <llvm_repo_path>
# Build dpc++ as normal
ock_repo=$1
llvm_repo=$2

mkdir -p $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes
cp -r $ock_repo/modules/compiler/multi_llvm/include/multi_llvm/* $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/compiler_pipeline/include/multi_llvm
cp -r $ock_repo/modules/compiler/vecz/include/vecz/* $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz/include

for f in address_spaces.h cl_builtin_info.h encode_kernel_metadata_pass.h pass_functions.h scheduling.h verify_reqd_sub_group_size_pass.h \
attributes.h define_mux_builtins_pass.h group_collective_helpers.h pass_machinery.h sub_group_analysis.h work_item_loops_pass.h \
barrier_regions.h device_info.h mangling.h prepare_barriers_pass.h target_extension_types.h \
builtin_info.h dma.h metadata.h replace_local_module_scope_variables_pass.h unique_opaque_structs_pass.h
do
    cp $ock_repo/modules/compiler/compiler_pipeline/include/compiler/utils/$f $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/compiler_pipeline/include/compiler/utils
done

for f in attributes.cpp       define_mux_builtins_pass.cpp     mangling.cpp          pass_machinery.cpp   sub_group_analysis.cpp  work_item_loops_pass.cpp \
barrier_regions.cpp  dma.cpp             metadata.cpp          prepare_barriers_pass.cpp         target_extension_types.cpp \
builtin_info.cpp     encode_kernel_metadata_pass.cpp  mux_builtin_info.cpp  replace_local_module_scope_variables_pass.cpp  unique_opaque_structs_pass.cpp \
cl_builtin_info.cpp  group_collective_helpers.cpp     pass_functions.cpp    scheduling.cpp       verify_reqd_sub_group_size_pass.cpp
do
    cp $ock_repo/modules/compiler/compiler_pipeline/source/$f $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/compiler_pipeline/source
done



# cp -r $ock_repo/modules/compiler/compiler_pipeline/include/ $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes
# cp -r $ock_repo/modules/compiler/vecz $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes
# mkdir -p $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/cmake
# cp $ock_repo/cmake/AddCA.cmake $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/cmake
# cp $ock_repo/scripts/native_cpu_CMakeLists.txt $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/CMakeLists.txt
# rm $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz/README.md
# cp $ock_repo/doc/modules/vecz.rst $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz
# mkdir -p  $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/compiler_pipeline/docs
# cp -r $ock_repo/doc/modules/compiler* $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/compiler_pipeline/docs
# cd $llvm_repo/llvm/lib/SYCLNativeCPUUtils/
# git -C $llvm_repo apply $ock_repo/scripts/DPCPP-0001-Update-CMakeLists.txt-to-automatically-use-included-.patch
# git -C $llvm_repo add $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes
# git -C $llvm_repo add $llvm_repo/llvm/lib/SYCLNativeCPUUtils/CMakeLists.txt

