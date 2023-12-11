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

# This script creates a fake construction kit from a oneapi-construction-kit
# directory and runs the create target script to produce an out of tree
# tutorial start or end, ready for compiling.

run_script="none"
external_dir=$PWD
do_clean=0
do_overwrite_tutorial=0

while getopts ":s:c:e:f:do" opt; do
  case ${opt} in
    s )
      run_script=$OPTARG
      ;;
    e )
      external_dir=$OPTARG
      ;;
    f )
      feature_enable="--override"
      feature_enable_name="feature"
      feature_enable_val="$OPTARG"
      ;;
    d )
      do_clean=1
      ;;
    c )
      copyright_enable="--override"
      copyright_enable_name="copyright_name"
      copyright_enable_val="$OPTARG"
      ;;      
    o )
      do_overwrite_tutorial=1
      ;;
    \? )
      echo "Invalid Option: -$OPTARG" 1>&2
      exit 1
      ;;
    : )
      echo "Invalid Option: -$OPTARG requires an argument" 1>&2
      exit 1
      ;;
  esac
done
shift $((OPTIND -1))  
if [[ $# -ne 1 ]]; 
then
     echo "usage:  [-s <tutorial_point>] [-e <external_dir] [-f <feature_element>] [-c <copyright_name][-d] [-o] <dir_oneapi-construction-kit>"
     echo "  note:"
     echo "    -s will run the create_target.py script"
     echo "       <tutorial_point> must be one of 'start' or 'end'"
     echo "       'start' will fail most tests"
     echo "    -e refers to the external directory we wish to produce our own code in (defaults to $PWD)"
     echo "    -d removes directories before creating"
     echo "    -o overwrites the tutorial only"
     echo "    -f allows overriding of \"feature\" elements"
     echo "    -c adds copyright_name to copyrights"
     exit 1
fi
if [[ $run_script == "start" ]]
then
  json_file="refsi.json"
elif [[ $run_script == "end" ]]
then
  json_file="refsi_with_wrapper.json"
elif [[ $run_script != "none" ]]
then
  echo "<tutorial_point> must be one off 'start' or 'end'"
  exit 1
fi

ock_dir=$1

if [[ $do_clean -eq 1 ]]
then
  rm -rf $external_dir
fi

if [[ $do_overwrite_tutorial -eq 1 ]]
then
  overwrite=" --overwrite-if-file-exists"
fi

# make the external directory
mkdir -p $external_dir
ock_dir=`realpath $ock_dir`
external_dir=`realpath $external_dir`

cd $external_dir

if [[ $run_script != "none" ]]
then
  # Create the new target
  $ock_dir/scripts/create_target.py $ock_dir \
    $ock_dir/scripts/new_target_templates/$json_file \
      --external-dir $external_dir $overwrite \
      $feature_enable $feature_enable_name $feature_enable_val \
      $copyright_enable $copyright_enable_name $copyright_enable_val
  if [[ $do_overwrite_tutorial -eq 0 ]]
  then
    if [[ -d hal_refsi_tutorial ]]
    then
      echo "Warning: $PWD/hal_refsi_tutorial already exists - no patches applied"
    else
      cp -r $ock_dir/examples/hals/hal_refsi_tutorial hal_refsi_tutorial
      cd hal_refsi_tutorial
      patch -s -p1 < $ock_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step1.patch
      patch -s -p1 < $ock_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step2.patch
      patch -s -p1 < $ock_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step3.patch
      patch -s -p1 < $ock_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step4.patch
      patch -s -p1 < $ock_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step5_sub1.patch
      patch -s -p1 < $ock_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step5_sub2.patch
      patch -s -p1 < $ock_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step5_sub3.patch
      patch -s -p1 < $ock_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step5_sub4.patch
      cd ..
    fi
    if [[ -L hal_refsi_tutorial/external/refsidrv && -e hal_refsi_tutorial/external/refsidrv ]]
    then
      echo "Note: link already exists from hal_refsi_tutorial/external/refsidrv
         to $ock_dir/examples/refsi/hal_refsi/external/refsidrv"
    else
      ln -s -i $ock_dir/examples/refsi/hal_refsi/external/refsidrv hal_refsi_tutorial/external
    fi
  fi
fi
