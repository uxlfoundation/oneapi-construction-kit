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

REPEAT_COUNT="1"
VERBOSE=false

if [ ! -z $1 ] && [ $1 != "-h" ] && [ $1 != "--help" ]; then 
    while getopts k:c:b:l:r:a:v option
    do
        case "${option}" in
            k) KERNEL_DIR=${OPTARG};;
            c) COMPARISON_DIR=${OPTARG};;
            b) BINARY_DIR=${OPTARG};;
            l) LIT_DEST_DIR=${OPTARG};;
            a) ALT_KERNEL_DIR=${OPTARG};;
            r) REPEAT_COUNT=${OPTARG};;
            v) VERBOSE=true ;;
        esac
    done
fi

INT_RE='^[0-9]+$'
BATCH_SIZE=128

if [ -z $1 ] || [ $1 == "-h" ] || [ $1 == "--help" ] || [[ ! $REPEAT_COUNT =~ $INT_RE ]] ||
   [ -z $KERNEL_DIR ] || [ -z $BINARY_DIR ] || [ -z $LIT_DEST_DIR ]; then
    echo "Usage: create_tests.sh [-h] [--help] [-v] [-r REPEAT_COUNT] [-a ALT_KERNEL_DIR]"
    echo "                       [-c COMPARISON_DIR] -k KERNEL_DIR -b BINARY_DIR -l LIT_DEST_DIR"
    echo ""
    echo "Generate and buld lit tests from OpenCL kernels"
    echo ""
    echo "mandatory arguments:"
    echo "-k KERNEL_DIR      Path to OpenCL kernels."
    echo "-b BINARY_DIR      Path to OCLC and OpenCL dynamic library."
    echo "-l LIT_DEST_DIR    Path to GeneratedTests directory."
    echo ""
    echo "optional arguments:"
    echo "-v                 Run with verbose output, and save generated files."
    echo "-c COMPARISON_DIR  Path to alternative OpenCL driver. If not specified,"
    echo "                   the driver in BINARY_DIR will be used."
    echo "-a ALT_KERNEL_DIR  Path to the OpenCL kernels directory relative to"
    echo "                   LIT_DEST_DIR/Tests. This only needs to be set if the"
    echo "                   generated lit tests are meant to be run on a machine"
    echo "                   separate from the one that this script is being run on."
    echo "                   For tests to be run on Jenkins, set this to"
    echo "                   'Inputs/KernelsCL/kernels/PerfCL'."
    echo "-r REPEAT_COUNT    Number of times to run each kernel. Higher values"
    echo "                   increase run time, but decrease the chances of"
    echo "                   generating a lit test that crashes or is inconsistent"
    echo "                   default value is 1. This flag is only needed for debugging."
    exit 0
fi

#get kernels from source files, and generate oclc arguments
rm -fr files
mkdir files/
mkdir files/out_analyze/

if [ "$VERBOSE" = true ]; then
    echo "generating oclc arguments"
fi

JOB_ID=0
for FILE in $(find $KERNEL_DIR -name "*.cl"); do
    # spawn batches of $BATCH_SIZE subprocesses
    ((JOB_ID=JOB_ID%BATCH_SIZE)); ((JOB_ID++==0)) && wait
    ( KERNELS=$(cat $FILE | tr -d '\n'    | grep -oP '(__)?kernel[^{]+' | \
        grep -oP 'void\s*[^(]+'    | sed -e 's/void\s*//g')
    BASENAME=$(basename -s .cl $FILE)
    for KERNEL in $KERNELS; do
        if [ -z $ALT_KERNEL_DIR ]; then
            python analyze_kernels.py $FILE --binary_dir $BINARY_DIR --kernel \
            $KERNEL >& 'files/out_analyze/'$BASENAME'_k_'$KERNEL'_out.txt' || true
        else
            ALT_FILE=$ALT_KERNEL_DIR/${FILE#$KERNEL_DIR}
            python analyze_kernels.py $FILE --binary_dir $BINARY_DIR \
            --kernel $KERNEL --alt_filename $ALT_FILE \
            >& 'files/out_analyze/'$BASENAME'_k_'$KERNEL'_out.txt' || true
        fi
        if [ "$VERBOSE" = true ]; then
            echo "generating arguments for kernel "$KERNEL" in file "$FILE
        fi
    done ) &
done
wait

# filter out tests which don't produce suitable oclc arguments
find files/out_analyze -type f -exec grep -niIl "\-print" {} \; > files/valid
if [ "$VERBOSE" = true ]; then
    # categorise failing kernels by common error messages
    # to aid the debugging of analyze_kernels.py
    mkdir files/failures/
    find files/out_analyze -type f -exec grep -niIl \
        "Build program failed" {} + > files/failures/buildfail
    find files/out_analyze -type f -exec grep -niIl \
        "KeyError" {} + > files/failures/keyerror
    find files/out_analyze -type f -exec grep -niIl \
        "maximum recursion depth exceeded" {} + > files/failures/maxrec
    find files/out_analyze -type f -exec grep -niIl \
        "setting kernel argument failed" {} + > files/failures/badarg
    find files/out_analyze -type f -exec grep -niIl \
        "SyntaxError\|NameError" {} + > files/failures/fail
    GENERIC="setting kernel argument failed\|-print\|SyntaxError\|NameError\|"
    GENERIC+="Build program failed\|maximum recursion depth exceeded\|KeyError"
    find files/out_analyze -type f -exec grep -niIL \
        GENERIC {} + > files/failures/generic
    echo "done"
    echo $(ls files/out_analyze/ | wc -l)" kernels ("$(cat files/valid | wc -l) \
         " valid) found in "$(ls $(find $KERNEL_DIR -name "*.cl") | wc -l)" files"
    echo "generating kernel outputs"
fi

# run each test `REPEAT_COUNT` times on the alternate driver,
# filtering out tests which crash, or whose outputs are inconsistent
mkdir files/Inputs/

if [ ! -z $COMPARISON_DIR ]; then
    mkdir files/bin/
    cp -r $BINARY_DIR/* files/bin/
    rm -f $BINARY_DIR/libOpenCL*
    cp -r $COMPARISON_DIR/* $BINARY_DIR
fi

for i in $(eval echo {1..$REPEAT_COUNT} ); do
    rm -f files/valid.tmp
    JOB_ID=0
    while read line; do
        ((JOB_ID=JOB_ID%BATCH_SIZE)); ((JOB_ID++==0)) && wait
        (PREFIX="files/out_analyze/"
        RESULT='files/Inputs/'${line:${#PREFIX}}
        # filter out tests which crash
        CRASH=false
        $BINARY_DIR/oclc $(cat $line | tail -1 | tr -d \") > $RESULT.tmp
        if [[ $? -eq 0 ]]; then
            echo $line > $line.tmp
        else
            CRASH=true
            if [ "$VERBOSE" = true ]; then
                echo $line >> files/failures/crash
            fi
            rm -f $RESULT.tmp
            rm -f $RESULT
            rm -f $line.tmp
        fi

        # filter out tests which don't produce consistent output
        if [[ "$CRASH" = false ]]; then
            if ([ -e $RESULT.tmp ] && ( [ ! -e $RESULT ] || \
                [[ -z $(cmp $RESULT.tmp $RESULT) ]])); then
                    mv $RESULT.tmp $RESULT
            else
                if [ "$VERBOSE" = true ]; then
                    echo $RESULT >> files/failures/inconsistent
                fi
                rm -f $RESULT.tmp
                rm -f $RESULT
                rm -f $line.tmp
            fi
        fi
        if [ "$VERBOSE" = true ]; then
            echo "generating "$RESULT
        fi
        if [[ -e $line.tmp ]]; then
            cat $line.tmp >> files/valid.tmp
        fi) &
    done < files/valid
    wait
    if [ "$VERBOSE" = true ]; then
        OLD=$(cat files/valid | wc -l)
        NEW=$(cat files/valid.tmp | wc -l)
        echo $(($OLD-$NEW))" failing kernels removed in pass "$i \
            " ("$(cat files/valid.tmp | wc -l)" remaining)"
    fi
    mv files/valid.tmp files/valid
done
wait

if [ "$VERBOSE" = true ]; then
    echo "done"
    echo "generating lit tests"
fi
if [ ! -z $COMPARISON_DIR ]; then
    rm -fr $BINARY_DIR/*
    cp -r files/bin/* $BINARY_DIR
    rm -fr files/$BINARY_DIR
fi

# create lit tests
rm -fr $LIT_DEST_DIR/Tests/
mkdir $LIT_DEST_DIR/Tests/

JOB_ID=0
while read line; do
    ((JOB_ID=JOB_ID%BATCH_SIZE)); ((JOB_ID++==0)) && wait
    (namestrip=$(basename $line)
    cmd=$(cat $line | tail -1 | tr -d \")
    cmp_regex='s/-print\([^,]\+\),[0-9]\+/-compare\1:%p\/Inputs\/'$namestrip'/g'
    default_oclc_args=$(echo $cmd | sed -e $cmp_regex)
    if [ -z $ALT_KERNEL_DIR ]; then
        #identity regex if no alternate directory is given
        oclc_args=$default_oclc_args
    else
        oclc_args=${default_oclc_args%\ *}$(cat $line | tail -2 | head -1)
    fi
    lit=$LIT_DEST_DIR/Tests/${namestrip%.*}.test
    COUNTER=$((COUNTER+1))
    echo "// Copyright (C) Codeplay Software Limited" > $lit
    echo "//" >> $lit
    echo "// Licensed under the Apache License, Version 2.0 (the \"License\") with LLVM" >> $lit
    echo "// Exceptions; you may not use this file except in compliance with the License." >> $lit
    echo "// You may obtain a copy of the License at" >> $lit
    echo "//" >> $lit
    echo "//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt" >> $lit
    echo "//" >> $lit
    echo "// Unless required by applicable law or agreed to in writing, software" >> $lit
    echo "// distributed under the License is distributed on an \"AS IS\" BASIS, WITHOUT" >> $lit
    echo "// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the" >> $lit
    echo "// License for the specific language governing permissions and limitations" >> $lit
    echo "// under the License." >> $lit
    echo "//" >> $lit
    echo "// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception" >> $lit
    echo "" >> $lit
    echo "RUN: %oclc "$oclc_args" > %t" >> $lit
    echo "RUN: %filecheck < %t %s" >> $lit
    echo "" >> $lit
    echo $cmd | grep -oP '\-print([[:space:]][^,]+)' \
        | sed -e 's/-print[ ]\+\([^ ]\+\)\+$/CHECK-DAG: \1 - match/g' >> $lit
    if [ "$VERBOSE" = true ]; then
        echo "generating lit test "$lit
    fi) &
done < files/valid
wait
cp -r files/Inputs/ $LIT_DEST_DIR/Tests/

if [ "$VERBOSE" = true ]; then
    echo "done"
else
    rm -fr files/
fi

# needed to prompt CMake to rebuild the lit tests
touch $LIT_DEST_DIR/CMakeLists.txt
