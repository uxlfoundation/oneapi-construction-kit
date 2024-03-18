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

# Imports.
import os
from check import *
from output import *
import variables as v

def gcovrGenerateXmlFile(args, obj_path, source_path):
    """ For a module, generate a gcovr cobertura-format .xml file in the 
        current directory. For coverage runs, cwd is "build" with source 
        therefore in "../*"

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            obj_path (str): path to objects folder of the current module.
            source_path (str): path to sources folder of the current module.
    """

    if checkIfModuleExcluded(args, source_path):
        return None

    # Give gcovr o/p files short tag names (from "..") not absolute (from "/")
    gcovr_file_tag = os.path.relpath(source_path, '..').replace('/', '-')

    # Create source path -f 'include' filter from source_path
    if source_path.startswith(os.getcwd()):
        # edge case: for source in "build" itself set source filter from cwd
        source_path_filter = source_path.replace(os.getcwd() + "/", "", 1) 
        source_path_filter = '^' + source_path_filter + '/.*'
    else:
        # else set the source filter from ".." where all other source lives
        source_path_filter = os.path.relpath(source_path, '..')
        source_path_filter = '^\\.\\./' + source_path_filter + '/.*'

    gcovr_output_file = os.path.join('coverage_' + gcovr_file_tag + '_' +
                 v.TEST_SUITE_NAME + '-' + str(v.TEST_SUITE_NAME_COUNTER) +
                 '.gcovr.xml')

    # Call gcovr:
    # -o : Output file in cwd - as pretty xml in our case.
    # -r : Sets root dir for source filenames in coverage data - and ensures
    #      output data contains relative paths from there.
    #      From cwd ("<workspace>/build"), root for all source is '..'
    # -f : Relative path to source files from cwd - aka 'source path filter'.
    #      obj_path is also a relative path to corresponding object files.
    # Relative paths are fiddly, but allow coverage output data to be archived,
    # ported and compared later. In addition, one set of data can be used
    # for both "trend/threshold" and "diff" coverage test models.

    pOut('*****  gcovr -o ' + gcovr_output_file + ' -r .. -f ' +
         source_path_filter + ' -j ' + str(v.NB_THREADS) + ' --xml-pretty ' +
         obj_path)

    gcovr_command = [
        'gcovr', '-o', gcovr_output_file, '-r', '..', '-f', source_path_filter,
                 '-j', str(v.NB_THREADS), '--xml-pretty', obj_path
    ]

    #pOut('***** gcovGenerateXmlFile: ' + ' '.join(gcovr_command))

    callProcess(gcovr_command)

    # housekeeping: delete the o/p file if it contains no cobertura data
    if checkIfOutputFileCanBeDeleted(gcovr_output_file):
        #pOut('*****            deleting: ' + gcovr_output_file)
        os.remove(gcovr_output_file)
