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

FROM ubuntu:22.04

# Create the working directory
RUN mkdir -p /home/demo
ENV BLOG_TOP_LEVEL=/home/demo

# Copy the one-api-construction-kit from the current directory.
COPY oneapi-construction-kit $BLOG_TOP_LEVEL/oneapi-construction-kit
WORKDIR $BLOG_TOP_LEVEL

ENV DEBIAN_FRONTEND=noninteractive
# Requirements
RUN apt update -y && apt install -y build-essential git cmake libtinfo-dev python3 \
  ninja-build doxygen python3-pip
RUN apt install -y wget libzstd-dev libtinfo5
RUN pip3 install lit virtualenv cmakelint cookiecutter

# Set the appropriate env variables
ENV ONEAPI_CON_KIT_DIR=$BLOG_TOP_LEVEL/oneapi-construction-kit
ENV LLVM_INSTALL_NAME=clang+llvm-16.0.4-x86_64-linux-gnu-ubuntu-22.04
ENV LLVM_INSTALL_DIR=$BLOG_TOP_LEVEL/$LLVM_INSTALL_NAME
ENV DPCPP_DIR=$BLOG_TOP_LEVEL/dpcpp_compiler
ENV LD_LIBRARY_PATH=$DPCPP_DIR/lib:$LD_LIBRARY_PATH
ENV OCL_ICD_FILENAMES=$BLOG_TOP_LEVEL/refsi_blog/build/oneAPIConstructionKit/lib/libCL.so
ENV ONEAPI_SAMPLES_DIR=$BLOG_TOP_LEVEL/oneAPI-samples
