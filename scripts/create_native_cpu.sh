#!/bin/bash

# Usage : ./create_native_cpu.sh <ock_repo_path> <llvm_repo_path>
# Build dpc++ as normal
ock_repo=$1
ock_user=$2
ock_branch=$3
llvm_repo=$4
llvm_branch=$5
script_dir=$( realpath `dirname -- "${BASH_SOURCE[0]}"` )
set -x
rm -rf $1
git clone git@github.com:uxlfoundation/oneapi-construction-kit.git $1
cd $1
git-filter-repo --paths-from-file $script_dir/native_cpu_paths.txt \
    --path-rename modules/compiler/compiler_pipeline:llvm/lib/SYCLNativeCPUUtils/compiler_passes/compiler_pipeline \
    --path-rename modules/compiler/vecz:llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz \
    --path-rename modules/compiler/multi_llvm/include/multi_llvm:llvm/lib/SYCLNativeCPUUtils/compiler_passes/compiler_pipeline/include/multi_llvm \
    --path-rename doc/modules/vecz/vecz.md:llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz/vecz.md \
    --path-rename doc/modules/compiler/overview.rst:llvm/lib/SYCLNativeCPUUtils/compiler_passes/compiler_pipeline/docs/overview.rst \
    --path-rename doc/modules/compiler/utils.rst:llvm/lib/SYCLNativeCPUUtils/compiler_passes/compiler_pipeline/docs/utils.rst

if [ "$ock_branch" != "" ]; then
    git branch -D $ock_branch || true
    git checkout -b $ock_branch
    # git add llvm
    # git commit -F $script_dir/native_cpu_commit_message.txt
    git clean -d -f
    git remote add ock_fork  git@github.com:${ock_user}/oneapi-construction-kit.git
    git push --set-upstream ock_fork $ock_branch --force

    if [ "$llvm_repo" != "" ]; then
    if [ "$llvm_branch" != "" ]; then
        cd $llvm_repo
        git checkout sycl
        git clean -d -f
        git branch -D $llvm_branch || true
        git remote add ock_fork git@github.com:${ock_user}/oneapi-construction-kit.git || true
        git checkout -b $llvm_branch
        git fetch ock_fork $ock_branch
        git merge -m "[NATIVE_CPU][SYCL] Merge from oneAPI Construction Kit into native_cpu" --allow-unrelated-histories ock_fork/$ock_branch
        cp $script_dir/native_cpu_CMakeLists.txt $llvm_repo/llvm/lib/SYCLNativeCPUUtils/CMakeLists.txt
        sed -i "s/DNATIVECPU_USE_OCK=Off/DNATIVECPU_USE_OCK=ON/" $llvm_repo/.github/workflows/sycl-linux-build.yml
        sed -i -e "s/config.name = 'Vecz'/config.name = \"Vecz\"/" -e "s/config.suffixes = \['.hlsl', '.ll'\]/config.suffixes = \[\".hlsl\", \".ll\"\]/" $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz/test/lit/lit.cfg.py
        for f in $( find $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes -name '*.h' -o -name '*.cpp' -o -name '*.inc'); do
           clang-format $f > /tmp/format.txt; cp /tmp/format.txt $f
        done
        cp $script_dir/native_cpu_compiler_passes_CMakeLists.txt llvm/lib/SYCLNativeCPUUtils/compiler_passes/CMakeLists.txt
        cp $script_dir/native_cpu_compiler_passes.rst llvm/lib/SYCLNativeCPUUtils/compiler_passes/compiler_passes.rst
        cp $script_dir/native_cpu_compiler_pipeline_CMakeLists.txt llvm/lib/SYCLNativeCPUUtils/compiler_passes/compiler_pipeline/CMakeLists.txt
        cp $script_dir/native_cpu_vecz_CMakeLists.txt llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz/CMakeLists.txt
        cp $script_dir/native_cpu_vecz_test_CMakeLists.txt llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz/test/CMakeLists.txt
        cp $script_dir/native_cpu_vecz_lit_CMakeLists.txt llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz/test/lit/CMakeLists.txt
        cp $native_cpu_vecz_test_lit_lit.cfg.py llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz/test/lit/CMakeLists.txt
        cp $script_dir/native_cpu_vecz_tools_CMakeLists.txt llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz/tools/CMakeLists.txt
        cp $script_dir/native_cpu_vecz_test_lit_lit.cfg.py llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz/test/lit/lit.cfg.py
        cp $script_dir/native_cpu_vecz_test_lit_lit.site.cfg.py.in llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz/test/lit/lit.site.cfg.py.in
        cp $script_dir/native_cpu_vecz_test_lit_llvm_RISCV_lit.local.cfg llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz/test/lit/llvm/RISCV/lit.local.cfg
        git apply --index $script_dir/native_cpu_vecz_lit.patch
        git add llvm/lib/SYCLNativeCPUUtils .github/workflows/sycl-linux-build.yml
        git rm $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz/test/lit/llvm/partial_linearization22-llvm18.ll
        git rm $llvm_repo/llvm/lib/SYCLNativeCPUUtils/compiler_passes/vecz/test/lit/llvm/ScalableVectors/lit.local.cfg
        git commit -F $script_dir/native_cpu_commit_message.txt
    fi
    fi
fi
