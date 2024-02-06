#!/usr/bin/env python
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

import os
import sys
import shutil
import requests
import argparse
import subprocess

def parse_arguments():
    parser = argparse.ArgumentParser(description='The purpose of this script is to build and execute VGG16 and '
                                     'Resnet50 networks using the oneAPI framework and the oneapi-construction-kit.'
                                     ' It requires specifying paths to the oneapi_construction_kit build directory '
                                     'and oneapi_base_toolkit. Additionally, portBLAS and portDNN are necessary to '
                                     'run and test the networks, but they will be cloned if the paths are not provided.'
                                     ' An input image path should be provided to test using the networks, in the absence'
                                     ' of image, the networks will only be downloaded and run.')

    # Prereqs: oneapi oneapi_construction_kit should be cloned and installed.
    # portDNN and portBLAS will be cloned if paths are not provided.
    parser.add_argument('--portBLAS_source', help='Provide path to portBLAS source.')
    parser.add_argument('--clone_portBLAS', action='store_true', help='Include this flag to clone repo if path to portBLAS_source is not provided.')
    parser.add_argument('--portDNN_source', help='Provide path to portDNN source.')
    parser.add_argument('--clone_portDNN',action='store_true', help='Include this flag to clone repo if path to portDNN is not provided')
    parser.add_argument('--oneapi_construction_kit_build_dir', required=True, help='Path to oneAPI construction kit')
    parser.add_argument('--oneapi_base_toolkit', required=True, help='Path to oneAPI base toolkit')
    parser.add_argument('--input_image', help='Path to the input image to be processed using the VGG16 and Resnet50.')
    return parser.parse_args()

# Set environment variables
def set_env_vars(args):
    os.environ["CMAKE_CXX_COMPILER"] = os.environ.get("CMAKE_CXX_COMPILER", os.path.join(args.oneapi_base_toolkit, "compiler", "linux", "bin-llvm", "clang++"))
    os.environ["CMAKE_C_COMPILER"] = os.environ.get("CMAKE_C_COMPILER", os.path.join(args.oneapi_base_toolkit, "compiler", "linux", "bin-llvm", "clang"))
    os.environ["LD_LIBRARY_PATH"] = f"{os.path.join(args.oneapi_base_toolkit, 'compiler', 'linux', 'lib')}:{os.path.join(args.oneapi_base_toolkit, 'compiler', 'linux', 'compiler', 'lib', 'intel64_lin')}:{os.environ.get('LD_LIBRARY_PATH', '')}"
    os.environ["OCL_ICD_FILENAMES"] = os.environ.get("OCL_ICD_FILENAMES", os.path.join(args.oneapi_construction_kit_build_dir, 'lib', 'libCL.so'))

    # Get a dictionary of all environment variables
    env_variables = os.environ

    # Print each environment variable and its value
    for key, value in env_variables.items():
        print(f"{key}={value}")

    print("Parsed Arguments:")
    print(f"oneapi_base_toolkit: {args.oneapi_base_toolkit}")
    print(f"oneapi-construction-kit_build_dir: {args.oneapi_construction_kit_build_dir}")


def run_cmake_and_ninja(cmake_command, build_dir):
    if os.path.exists(build_dir):
        print(f"{build_dir} exists,skipping building.")
    else:
        try:
            subprocess.run(cmake_command, check=True)
            subprocess.run(["ninja", "-C", build_dir], check=True)
        except subprocess.CalledProcessError as e:
            print(f"Error executing command: {e}", file=sys.stderr)
            sys.exit(1)

# Clone repository if not present
def clone_git_repository(repo_url, destination_directory):
    try:
        subprocess.run(["git", "clone", "--recursive", repo_url, destination_directory], check=True)
        print(f"Repository cloned successfully to {destination_directory}")
    except subprocess.CalledProcessError as e:
        print(f"Error cloning repository: {e}", file=sys.stderr)
        sys.exit(1)

# Build portBLAS
def build_portBLAS(args):
    script_directory = os.getcwd()
    portBlas_build_dir = os.path.join(script_directory, "build_portBlas")

    # Check if path to portBLAS is present, else clone portBLAS
    if args.portBLAS_source is not None:
        portBLAS_source = args.portBLAS_source
    elif args.clone_portBLAS:
        portBLAS_repo_url = "https://github.com/codeplaysoftware/portBLAS.git"
        portBLAS_source = os.path.join(script_directory, "portBLAS")
        if os.path.exists(portBLAS_source):
            print(f"portBLAS exists. Continueing with the cmake.")
        else:
            clone_git_repository(portBLAS_repo_url, portBLAS_source)
    else:
        print(f'Error: portBLAS not found. Pass portBLAS_source or enable clone_portBLAS.', file=sys.stderr)
        sys.exit(1)

    cmake_portBlas_command = [
        "cmake",
        f"-B {portBlas_build_dir}",
        f"{portBLAS_source}",
        "-GNinja",
        "-DSYCL_COMPILER=dpcpp"
    ]

    run_cmake_and_ninja(cmake_portBlas_command, portBlas_build_dir)

# Build portDNN
def build_portDNN(args):
    script_directory = os.getcwd()
    portdnn_build_dir = os.path.join(script_directory, "build_portdnn")
    
    # Check if path to portDNN is present, else clone portDNN
    if args.portDNN_source is not None:
        portDNN_source = args.portDNN_source
    elif args.clone_portDNN:
        portDNN_repo_url = "https://github.com/codeplaysoftware/portDNN.git"
        portDNN_source = os.path.join(script_directory, "portDNN")
        if os.path.exists(portDNN_source):
            print(f"portDNN exists. Continueing with the cmake.")
        else:
            clone_git_repository(portDNN_repo_url, portDNN_source)
    else:
        print(f'Error: portDNN not found. Pass portDNN_source or enable clone_portDNN.', file=sys.stderr)
        sys.exit(1)

    cmake_portdnn_command = [
        "cmake",
        f"-B {portdnn_build_dir}",
        f"{portDNN_source}",
        "-GNinja",
        f"-DCMAKE_CXX_COMPILER={os.environ['CMAKE_CXX_COMPILER']}",
        "-DSNN_BUILD_BENCHMARKS=OFF",
        "-DSNN_BENCH_SYCLBLAS=OFF",
        "-DSNN_BUILD_DOCUMENTATION=OFF",
        f"-DSyclBLAS_DIR='{os.path.join(script_directory, 'build_portBlas')}'"
    ]

    run_cmake_and_ninja(cmake_portdnn_command, portdnn_build_dir)
    return portDNN_source


def download_and_convert_model(dir, file_url, destination_file, h5_to_bin_script):
    # Check if the model dir already exists
    path = os.path.join(os.getcwd(), dir)
    if os.path.exists(path):
        print(f"'{path}' already exists. Skipping creating '{path}'.")
    else:
        os.mkdir(path)
        print(f"Directory '{path}' created successfully.")

    # Check if the file already exists
    if os.path.exists(destination_file):
        print(f"'{destination_file}' already exists. Skipping download.")
    else:
        try:
            response = requests.get(file_url)
            response.raise_for_status()
            with open(destination_file, "wb") as file:
                file.write(response.content)
            print(f"Downloaded '{destination_file}' successfully.")
        except requests.exceptions.RequestException as e:
            print(f"Error downloading file: {e}", file=sys.stderr)
            sys.exit(1)
        try:
            subprocess.run(["python3", h5_to_bin_script, destination_file], check=True, cwd=path)
            print(f"Converted '{destination_file}' to binary successfully.")
        except subprocess.CalledProcessError as e:
            print(f"Error running h5toBin.py: {e}", file=sys.stderr)
            sys.exit(1)


def prepare_image(img_script_path, input_image):
    try:
        subprocess.run(["python3", img_script_path, input_image], check=True)
        print(f"Image prepared successfully.")
    except subprocess.CalledProcessError as e:
        print(f"Error executing the script: {e}", file=sys.stderr)
        sys.exit(1)

# Test on images
def test_image(model_script, data_dir, output_file):
    command_args = [
        model_script,
        data_dir + "/",
        output_file
    ]
    try:
        subprocess.run(command_args, check=True)
        print(f"Image tested successfully with {os.path.basename(model_script)}.")
    except subprocess.CalledProcessError as e:
        print(f"Error executing the command: {e}", file=sys.stderr)
        sys.exit(1)


def main():
    args = parse_arguments()
    set_env_vars(args)
    build_portBLAS(args)
    portDNN_source = build_portDNN(args)
    portdnn_build_dir = os.path.join(os.getcwd(), "build_portdnn")
    vgg_dir = os.path.join(os.getcwd(), "vdata")
    resnet_dir = os.path.join(os.getcwd(), "rdata")

    # Set the environment variables
    os.environ["CA_HAL_DEBUG"] =  os.environ.get("CA_HAL_DEBUG", "1")
    os.environ["CA_PROFILE_LEVEL"] = os.environ.get("CA_PROFILE_LEVEL", "3")
    os.environ["ONEAPI_DEVICE_SELECTOR"] = os.environ.get("ONEAPI_DEVICE_SELECTOR", "opencl::fpga")
    # To whitelist oneapi-construction-kit for the official Intel oneAPI basetoolkit,
    # SYCL_CONFIG_FILE_NAME needs to be overridden
    os.environ["SYCL_CONFIG_FILE_NAME"] = ""

    # VGG16
    vgg_url = "https://storage.googleapis.com/tensorflow/keras-applications/vgg16/vgg16_weights_tf_dim_ordering_tf_kernels.h5"
    vgg_dest_file = os.path.join(vgg_dir, "vgg16_weights_tf_dim_ordering_tf_kernels.h5")
    vgg_h5_to_bin_script = os.path.join(portDNN_source, "samples", "networks", "vgg", "h5toBin.py")

    download_and_convert_model(vgg_dir, vgg_url, vgg_dest_file, vgg_h5_to_bin_script)

    # Resnet50
    resnet_url = "https://storage.googleapis.com/tensorflow/keras-applications/resnet/resnet50_weights_tf_dim_ordering_tf_kernels.h5"
    resnet_dest_file = os.path.join(resnet_dir, "resnet50_weights_tf_dim_ordering_tf_kernels.h5")
    resnet_h5_to_bin_script = os.path.join(portDNN_source, "samples", "networks", "resnet50", "h5toBin.py")

    download_and_convert_model(resnet_dir, resnet_url, resnet_dest_file, resnet_h5_to_bin_script)

    # Prepare and test the image with both networks if the image path is provided
    if args.input_image:
        # Prepare the image
        img_script_path = os.path.join(portDNN_source, "samples", "networks", "img2bin.py")

        prepare_image(img_script_path, args.input_image)

        # Test images
        output_file = args.input_image + ".bin"
        vgg_script = os.path.join(portdnn_build_dir, "samples", "networks", "vgg", "vgg")
        resnet_script = os.path.join(portdnn_build_dir, "samples", "networks", "resnet50", "resnet50")

        test_image(vgg_script, vgg_dir, output_file)
        test_image(resnet_script, resnet_dir, output_file)
    else:
        sys.exit(1)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)  # Terminate the script with a non-zero exit code