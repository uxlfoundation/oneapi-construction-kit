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

# Build this file with:
# docker build --build-arg USER=$USER  \
#              --build-arg UID=`id -u` \
#              --build-arg GID=`id -g` \
#              -t ca:18.04 scripts

ARG VERSION=18.04
FROM ubuntu:$VERSION

# List of required build arguments, specify with --build-arg <name>=<value>
ARG USER
ARG UID
ARG GID

# Ensure that build arguments are set by the user.
RUN test -n "$USER"
RUN test -n "$UID"
RUN test -n "$GID"

# Install all required packages.
RUN dpkg --add-architecture i386
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install --yes tzdata
RUN apt-get update && apt-get install --yes apt-transport-https ca-certificates gnupg software-properties-common wget
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
RUN apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
RUN apt-get update && apt-get install --yes \
  clang-9 clang-format-9 clang-tidy-9 \
  clinfo \
  cmake \
  doxygen \
  freeglut3-dev \
  g++-7 g++-7-multilib g++-7-arm-linux-gnueabihf g++-7-aarch64-linux-gnu \
  gcc-7 gcc-7-multilib gcc-7-arm-linux-gnueabihf gcc-7-aarch64-linux-gnu \
  gfortran \
  git \
  latexmk \
  libboost-all-dev \
  libc6-dev \
  libfftw3-dev \
  libgl1-mesa-dev \
  libgoogle-glog-dev \
  libjpeg-dev libpng-dev \
  libprotobuf-dev \
  libtinfo-dev lib32tinfo-dev \
  libxml2-dev \
  lcov \
  linux-libc-dev:i386 \
  make \
  ninja-build \
  ocl-icd-opencl-dev \
  opencl-clhpp-headers \
  openjdk-8-jdk \
  pkg-config \
  protobuf-compiler \
  python python-dev python-pip \
  python3 python3-dev python3-pip \
  subversion \
  texlive-full \
  vulkan-icd \
  zlib1g-dev

RUN wget -nv https://github.com/bazelbuild/bazel/releases/download/0.16.0/bazel_0.16.0-linux-x86_64.deb
RUN apt-get install -y ./bazel_0.16.0-linux-x86_64.deb
RUN rm bazel_0.16.0-linux-x86_64.deb
RUN bazel version

RUN pip2 install setuptools lit virtualenv junit-xml gcovr enum34 wheel
RUN pip3 install setuptools lit virtualenv junit-xml gcovr conan wheel

# Setup USER so that files written to a shared volume aren't owned by root.
RUN groupadd -g $GID $USER
RUN useradd -m -u $UID -g $GID -s /bin/bash $USER
USER $USER
WORKDIR /home/$USER
