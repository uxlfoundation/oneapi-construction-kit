#!/usr/bin/python3
# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

import sys
import os
import re
import optparse
from . import utils as util

#Specify help text in specified format. The Command-line class constructor
#will parse and automatically set-up the command-line parser. The format is
# -<short-opt>,--<long-opt> | help description | default list
# | and , are used as a delimiter.
# at the very end the default can be specified as -all or -<csv list of default values>
g_option_str = """\
-p,--prog|[optional] csv - programs to search | default -all
-f,--func|[optional] csv - (fully namespace qualified/ mangled) functions to search | default -all
-e,--event|[optional] csv - interesting perf events to search for | default -all"""

#list of symbols that have to pruned by default. This can
#be used to filter out extraneous section symbols. If symbol
#is named the same across multiple object files, the script
#will find a mismatch when looking up data within map.
#eg. <.rodata.cst16>  - static data stored within object
g_symbol_prune = ['.rodata.cst16']


#NOTE : This function is not  currently used, but depending
#on the object file it could potentially be used to reduce
#overall memory used by script
def add_to_prune_list(sym):
    """Function to dynamically add symbols to the prune list"""
    global g_symbol_prune
    if sym not in g_symbol_prune:
        g_symbol_prune.append(sym)


def to_prune(sym):
    """Utility function to determine whether a symbol
       should be within the search list"""
    global g_symbol_prune
    if sym in g_symbol_prune:
        return True
    return False


class CmdLine_Options:
    """Parse Command Line options specified by user"""

    def __init__(self, help_string):
        self.prog_options = None
        self.prog_args = None
        self.help_str = help_string
        self.opts = {}

        self._init_opts()
        self.opts = self.__extract_final_opts(self.prog_args,
                                              self.prog_options)

        #indexes to be used with generator functions
        self._map_idx = 0
        self._obj_idx = 0

    def _split_opt_help(self, line):
        return line.split('|')

    def _split_opt_forms(self, fq_opts):
        return fq_opts.split(',')

    def _split_opt_default(self, line, sep):
        default = line.split(sep)
        if len(default) > 1:
            return default[1]
        else:
            return None

    def __extract_final_opts(self, mandatory, optional):
        if len(mandatory) <= 1:
            print(
                "Both map and object files mandatory. Run script with -h to see options"
            )
            sys.exit(-1)

        #store Commandline positional arguments as list of objects and map files
        options = {}
        options['obj'] = self.__serialize_csv_chunks([mandatory[0]])
        options['map'] = self.__serialize_csv_chunks([mandatory[1]])

        for key in vars(optional).keys():
            options[key] = getattr(optional, key, None)
        return options

    def __serialize_csv_chunks(self, arg_list):
        if arg_list is None:
            return arg_list

        new_list = []
        for item in arg_list:
            new_list += item.split(',')
        return new_list

    def _init_opts(self):
        opt_parser = optparse.OptionParser(
            "usage: perf-annotate.py [options] <object-file> <map-file>")
        #Parses help string and sets-up command-line
        #parser object automatically
        key_list = {}
        for line in self.help_str.split("\n"):
            _opt_help = self._split_opt_help(line)
            _short_long_opts = self._split_opt_forms(_opt_help[0])
            opt_parser.add_option(
                _short_long_opts[0],
                _short_long_opts[1],
                action="append",
                help=_opt_help[1] + "," + _opt_help[2])
            _opt_default = self._split_opt_default(_opt_help[2], '-')
            key_list[_short_long_opts[1].lstrip(" -")] = _opt_default

        #Obtain both positional and optional arguments
        self.prog_options, self.prog_args = opt_parser.parse_args(sys.argv[1:])
        #Populate map of optional arguments
        for key in key_list.keys():
            setattr(self.prog_options, key,
                    self.__serialize_csv_chunks(
                        getattr(self.prog_options, key, None)))
            if not getattr(self.prog_options, key, None):
                setattr(self.prog_options, key, [key_list[key]])

    def display_options(self):
        for key in self.opts.keys():
            print("option {0:s} = {1}".format(key, self.opts[key]))

    def get_option(self, key):
        return self.opts[key]

    #one-time function to be used to lazily eval (disassemble)
    #object files until we get to the symbol that we are looking for
    def iter_object_files(self):
        self._obj_idx = 0
        while self._obj_idx < len(self.opts['obj']):
            yield self.opts['obj'][self._obj_idx]
            self._obj_idx += 1

    #one-time function to lazily eval (search for symbol)
    #object files until we get to the symbol that we are looking for
    def iter_map_files(self):
        self._map_idx = 0
        while self._map_idx < len(self.opts['map']):
            yield self.opts['map'][self._map_idx]
            self._map_idx += 1

    #returns a unified map of all the symbols
    #in a list of all the map functions.
    #{ key = function-name : ( abs-address, size )
    def unified_jit_map(self):
        unified_map = {}
        map_re = re.compile(
            r'^\s*(?P<address>[0-9a-fA-F]+)\s+(?P<size>[0-9a-fA-F]+)\s+(?P<symbol>[_a-zA-Z][_a-zA-Z0-9]*)$'
        )
        for _file in self.iter_map_files():
            lines = util.copy_to_list(_file)
            for line in lines:
                jit_map = map_re.match(line)
                if not jit_map:
                    continue

                _sym_addr = int(jit_map.group('address'), 16)
                _sym_size = int(jit_map.group('size'), 16)
                _symbol = jit_map.group('symbol')
                if unified_map.get(_symbol, None):
                    print("potential duplicate of symbol {0} in map files".
                          format(_symbol))
                    sys.exit(-1)

                unified_map[_symbol] = (_sym_addr, _sym_size)

        return unified_map


cmdline = CmdLine_Options(g_option_str)


def main():
    """Main program"""
    global cmdline
    cmdline.display_options()


if __name__ == '__main__':
    main()
