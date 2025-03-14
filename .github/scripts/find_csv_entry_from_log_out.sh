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

# rudimentary script that attempts to use the output from run_cities log to find original
# csv line to help with creating an override file with ,XFail at the end
if [[ $# != 3 ]];
then
  echo "usage: find_csv_entry_from_log_out.sh <log> <orig csv> <extend>"
  exit 1
fi

cat $1 | while read line
do
    line_strip_to_slash=$(echo $line | sed -e 's/[^\/]*\///' -e 's/\\/\\\\\\\\/g')
    test_args=`echo $line_strip_to_slash | sed -e 's/[^\/]*\///'`
    test_bin=`echo $line_strip_to_slash | sed 's/\/.*//'`
    out=`grep "$test_bin" $2 | grep "$test_args"| head -1`
    if [[ ! -z "$out" ]]; then
      echo ${out}${3}
    fi
done
