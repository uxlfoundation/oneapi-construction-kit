#!/bin/bash

# Usage : ./create_native_cpu.sh <ock_repo_path> <llvm_repo_path>
# Build dpc++ as normal
ock_repo=$1
llvm_repo=$2

mkdir -p $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes
cp -r $ock_repo/modules/compiler/multi_llvm $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes
cp -r $ock_repo/modules/compiler/compiler_pipeline $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes
cp -r $ock_repo/modules/compiler/vecz $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes
mkdir -p $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/cmake
cp $ock_repo/cmake/AddCA.cmake $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/cmake
cp $ock_repo/scripts/native_cpu_CMakeLists.txt $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/CMakeLists.txt
rm $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz/README.md
cp $ock_repo/doc/modules/vecz.rst $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz
mkdir -p  $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/compiler_pipeline/docs
cp -r $ock_repo/doc/modules/compiler* $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/compiler_pipeline/docs
cd $llvm_repo/llvm/lib/SYCLNativeCPUUtils/
git -C $llvm_repo apply $ock_repo/scripts/DPCPP-0001-Update-CMakeLists.txt-to-automatically-use-included-.patch
git -C $llvm_repo add $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes
git -C $llvm_repo add $llvm_repo/llvm/lib/SYCLNativeCPUUtils/CMakeLists.txt

