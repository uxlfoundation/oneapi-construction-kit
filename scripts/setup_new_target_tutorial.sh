#!/bin/bash

# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# This script creates a fake DDK from a ComputeAorta directory and
# runs the create target script to produce an out of tree tutorial start
# or end, ready for compiling.

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
     echo "usage:  [-s <tutorial_point>] [-e <external_dir] [-f <feature_element>] [-c <copyright_name][-d] [-o] <dir_ComputeAorta>"
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

ddk_dir=$1

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
ddk_dir=`realpath $ddk_dir`
external_dir=`realpath $external_dir`

cd $external_dir

if [[ $run_script != "none" ]]
then
    # Create the new target
    $ddk_dir/scripts/create_target.py $ddk_dir \
      $ddk_dir/scripts/new_target_templates/$json_file \
         --external-dir $external_dir $overwrite \
         $feature_enable $feature_enable_name $feature_enable_val \
         $copyright_enable $copyright_enable_name $copyright_enable_val
  if [[ $do_overwrite_tutorial -eq 0 ]]
  then
    cp -r $ddk_dir/examples/hals/hal_refsi_tutorial hal_refsi_tutorial
    cd hal_refsi_tutorial
    git apply $ddk_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step1.patch
    git apply $ddk_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step2.patch
    git apply $ddk_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step3.patch
    git apply $ddk_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step4.patch
    git apply $ddk_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step5_sub1.patch
    git apply $ddk_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step5_sub2.patch
    git apply $ddk_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step5_sub3.patch
    git apply $ddk_dir/doc/overview/tutorials/creating-new-hal/patches/tutorial1_step5_sub4.patch 
    cd ..
    ln -s $ddk_dir/examples/refsi/hal_refsi/external/refsidrv hal_refsi_tutorial/external
  fi
fi
