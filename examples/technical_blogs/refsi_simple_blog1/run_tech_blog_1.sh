#!/bin/bash

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

# This script follows the technical blog for creating a fake Refsi 'G1' target using the oneapi-construction-kit
# and other oneAPI or LLVM related repos and binaries.
# It demonstrates building the Refsi target, running a simple OpenCL test and then running a SYCL test, showing
# debug output that it is actually running on the device.

# Requires the following env variables to be set prior to running the script
# BLOG_TOP_LEVEL     : Top level directory
# ONEAPI_CON_KIT_DIR : Location of the oneAPI Construction Kit
# DPCPP_DIR          : Location of dpcpp compiler
# LLVM_INSTALL_NAME  : Name of llvm install directory
# LLVM_INSTALL_DIR   : Full path to llvm install directory
# LD_LIBRARY_PATH    : Must include lib directory of $DPCC_DIR
# OCL_ICD_FILENAMES  : Points to the final built OpenCL driver - assuming it is in $BLOG_TOP_LEVEL/refsi_blog
# It also assumes we have cloned the oneapi-construction-kit under $BLOG_TOP_LEVEL

skip_download=0

# Support -s to skip downloading things if we already have done so
while getopts "s" opt; do
  case ${opt} in
    s )
      skip_download=1
      ;;
    \? )
      exit 1
      ;;      
  esac
done
shift $((OPTIND -1))

# Download llvm 16.0.4, oneAPI samples and the dpcpp compiler
if [[ $skip_download -ne 1 ]]
then
  wget https://github.com/llvm/llvm-project/releases/download/llvmorg-16.0.4/$LLVM_INSTALL_NAME.tar.xz
  tar xf $LLVM_INSTALL_NAME.tar.xz
  git clone https://github.com/oneapi-src/oneAPI-samples --depth 1
  wget https://github.com/intel/llvm/releases/download/sycl-nightly%2F20230518/dpcpp-compiler.tar.gz 
  tar xf dpcpp-compiler.tar.gz
fi

cat $ONEAPI_CON_KIT_DIR/scripts/new_target_templates/refsi_g1.json

# Create a new Refsi target
python3 $ONEAPI_CON_KIT_DIR/scripts/create_target.py $ONEAPI_CON_KIT_DIR $ONEAPI_CON_KIT_DIR/scripts/new_target_templates/refsi_g1.json --external-dir $BLOG_TOP_LEVEL/refsi_blog
ls $BLOG_TOP_LEVEL/refsi_blog

# Build it
cd $BLOG_TOP_LEVEL/refsi_blog
cmake -Bbuild \
      -DCA_EXTERNAL_ONEAPI_CON_KIT_DIR=$ONEAPI_CON_KIT_DIR \
      -DCA_EXTERNAL_REFSI_G1_HAL_DIR=$ONEAPI_CON_KIT_DIR/examples/refsi/hal_refsi \
      -DCA_MUX_TARGETS_TO_ENABLE="refsi_g1" \
      -DCA_REFSI_G1_ENABLED=ON \
      -DCA_CL_ENABLE_OFFLINE_KERNEL_TESTS=OFF \
      -DCA_ENABLE_API=cl \
      -DCA_LLVM_INSTALL_DIR=$LLVM_INSTALL_DIR -GNinja .


ninja -Cbuild

# Run a simple UnitCL test
./build/ComputeAorta/bin/UnitCL --gtest_filter=Execution/Execution.Task_01_02_Add/OpenCLC

CA_HAL_DEBUG=1 ./build/ComputeAorta/bin/UnitCL --gtest_filter=Execution/Execution.Task_01_02_Add/OpenCLC

# Build the SYCL vector add test
$DPCPP_DIR/bin/clang++ -fsycl $ONEAPI_SAMPLES_DIR/DirectProgramming/C++SYCL/DenseLinearAlgebra/vector-add/src/vector-add-buffers.cpp -o build/simple_add

# Show the possible devices
$DPCPP_DIR/bin/sycl-ls

# Run the simple_add test
ONEAPI_DEVICE_SELECTOR=opencl:0 ./build/simple_add

# Add some debug to see the instructions being run.
ONEAPI_DEVICE_SELECTOR=opencl:0 SPIKE_SIM_DEBUG=1 ./build/simple_add
