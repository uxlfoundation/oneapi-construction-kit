#!/usr/bin/python3
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
"""
Driver script to utilise linux perf data that is dumped out of ComputeAorta
when run with linux perf.

This script takes is invoked as
perf-annotate <optional arguments> <object-file(s)> <map-file(s)>

The <object-file(s)> and <map-file(s)> are dumped in /tmp by ComputeAorta
when run with CA_ENABLE_PERF_INTERFACE=1 set on the command line.
Both <object-file(s)> and <map-file(s)> are csv delineated lists

If a disassembled file with the same name as the object file already exists,
this will be used to map addresses to code text. Otherwise an object file
will be generated using objdump.
The optional arguments are essentially filters that limit what is displayed
(By default every event, in every function, in every program will be displayed) :
1) -p , --prog=<program-name>
     a csv of program names that the user is interested in searching for
     within the perf-logs. The perf logs will contain samples from all
     executables that triggered a particular event during the perf tool's
     sampling window. Multiple -p options on the command-line will be merged to
     make one single csv option list.

2) -f , --func=<function-names>
     a csv of function names generated while JITing. Usually ComputeAorta will
     JIT function names of the form <__mux_host_%u>. Only names names found in
     the <map-file> can be annotated.
     If this option is not specified, then by default, all functions found
     within perf-logs will be annotated and displayed (Beware : potentially large)
     Multiple -f options on the command-line will be merged to make one single
     csv option list.

3) -e , --event=<event-names>
       a csv of event names that the user is interested in parsing. This option
       is useful to filter only the events the user is interested in viewing.
       Multiple -e options on the command-line will be merged to make one single
       csv option list. All events that are listed as filters should match
       exactly the same as the options provided when perf record was invoked
       (to record samples). for eg. if cache-references:u was requested while
       recording the samples, mention cache-references:u and not just cache-references

       Some potentially useful events (perf list on linux command-line)
       will give an exhaustive list )
       a) branch-misses
       b) branch-instructions
       c) cache-references
       d) cache-misses 
       e) cpu-cycles
       f) instructions
       g) mem-stores
       h) mem-load
"""

import subprocess
import sys
from perf_jit import command_line as cmd
from perf_jit import utils as util
from perf_jit import perf_parser as perf_parser


def main():
    #Display all user specified options as parsed by script
    cmd.cmdline.display_options()

    #Script is only used for linux perf
    #hence, check for OS compatibility
    if sys.platform != 'linux':
        print("{0}".format("Linux specific script. Please use a Linux Host"))
        sys.exit(-1)

    #Open a pipe with the data source coming from perf script stdout
    #Pipe the data to the PerfParser() object to collect and normalize
    perf_process = subprocess.Popen(
        ['perf', 'script'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    perf_parse = perf_parser.PerfParser(perf_process.stdout,
                                        cmd.cmdline.get_option('event'),
                                        cmd.cmdline.get_option('func'),
                                        cmd.cmdline.unified_jit_map(),
                                        cmd.cmdline.get_option('obj'))
    #collect and parse each event,in each function,
    #in each program as specified by the user filters
    perf_parse.parse()
    perf_parse.annotate_jit_asm()

    #clean-up dangling file descriptor
    perf_process.stdout.close()


if __name__ == '__main__':
    main()
