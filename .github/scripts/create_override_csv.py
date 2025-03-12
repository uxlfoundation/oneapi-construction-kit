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
"""Create combined override csv file based on a list of keys and a directory, using the keys
to combine together to create possible filename combinations, all starting with overload_.
The final overload file will be a concatation of any files found with those combinations,
prioritising the least keys first."""

import argparse
import os

def get_args():
    parser = argparse.ArgumentParser() 
    parser.add_argument(
        "-k",
        "--key", nargs="+", default=[],
        help="list of keys")
    parser.add_argument(
        "-d",
        "--dir", nargs="+", default=['.'],
        help="list of dirs")
    parser.add_argument(
        "-o",
        "--output", default='override_combined.csv',
        type=str,        
        help="output file (default override_combined.csv)")
    
    parser.add_argument(
        "-v",
        "--verbose",
        action="count",
        default=0,
        help="Make the output more verbose, -v shows csv files, -vv also cats the final output")
    args = parser.parse_args()
    return args

# We want to create combinations of keys based on maintaining the order passed
# in and combining with any keys which are beyond them in the list. Also we want
# to make combinations of different numbers of keys each at a distinct level.
# e.g. if we have keys one,two,three, we'd have keys L 1: one, two, three, L 2:
# one_two, one_three, two_three L 3: one_two_three.
# make_combos uses a list of `keys`` and a `base`` for the filename as well as
# `file_list` which is a list of lists. It also takes a level which tells it
# which sub-list to add to. It then calls make_combos recursively to work up the
# levels but without the first key in it's list which it uses to extend the `base`.
def make_combos(keys, base, file_list, level):
    for i,_ in enumerate(keys):
        new_combo = base + "_" + keys[i]
        file_list[level].append(new_combo + ".csv")
        # Combine to the next level up but starting from the next key
        # this avoids recreating combos and keeping the order
        make_combos(keys[i+1:], new_combo, file_list, level + 1)

# Search through the file list and directories to find files in the file_list.
# Concat them if found onto the output_file
def concat_files(output_file, dirs, file_list, verbose):
    for f in file_list:
        for d in dirs:
            filename = os.path.join(d, f)
            if os.path.exists(filename):
                if verbose > 0:
                    print("Found: ", filename)
                with open(filename, "r") as fp:
                    for line in fp:
                        if verbose > 1:
                            print("override: ", line)
                        output_file.write(line)

args = get_args()

# Set up the list of list, the bottom level is pre-done with "override_all.csv"
file_list = [["override_all.csv"] if i == 0 else [] for i in range(len(args.key)+1)]

# Create the list of combinations based on the keys passed in
make_combos(args.key, "override", file_list, 1)

# flatten list of lists
new_list = [f for fs in file_list for f in fs]

# Open the output file and concat in order.
try:
    f = open(args.output, "w+")
    concat_files(f, args.dir, new_list, args.verbose)
except Exception as e:
    print('Error: ', e)
