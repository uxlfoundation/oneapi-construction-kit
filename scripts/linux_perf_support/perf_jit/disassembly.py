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

import sys
import os
import subprocess
import re
from . import utils as util


class Disassembly(object):
    """Base class to check and set up dependencies of the annotation
       script ie the text assembly file from object file if required."""
    def __init__(self, filename):
        self.template = filename
        self.asm_file = self._asm_filename(self.template)
        self.obj_file = self._obj_filename(self.template)
        self.map_file = self._map_filename(self.template)

        #Set up a dependency list for each different type of file
        #format for each record --> { key : ( action* , list-of-dependencies)}
        #*action is a textual name of an object method to call, if a dependency
        #is not found and needs to be generated
        #The dependecy checker will lazily eval and call the requisite generator
        self.depends = { self.asm_file : ("_obj_to_asm",[self.obj_file]) ,\
                         self.obj_file :(None,None)                         ,\
                         self.map_file : (None,None)}

        self.prerequisites_avail = self._check_dependencies(self.asm_file)

    #Text assembly from object file. The object file is output by ComputeAorta
    def _obj_to_asm(self, trgt, args):
        _cmd = ['objdump', '-D'] + args
        with open(trgt, "w") as asm_fd:
            subprocess.call(_cmd, stdout=asm_fd)

    # Recursively check dependency chain and
    # generate dependency if action is specified
    def _check_dependencies(self, root):
        """check if the root-file exists - if True then return Success
		   If it does not, check if each dependency exists
		   For those dependencies that do not exist, call it's actions"""
        if self._check_existence(root):
            return True

        failed_depend = None
        action, dependencies = self.depends[root]
        for dependence in dependencies:
            if self._check_existence(dependence):
                continue
            else:
                ret = self._check_dependencies(dependence)
                if not ret:
                    failed_depend = dependence
                    break
        else:
            if failed_depend:
                print("Could not create dependency[{0}] for node [{1}]".format(
                    failed_depend, root))
                return False

        # if we reach this point,
        # 1) We have created all our dependencies
        # 2) we have to create this node from our dependencies, because we do not yet exist
        self._create_func = getattr(self, action, None)
        if self._create_func:
            self._create_func(root, dependencies)
        del self._create_func
        return self._check_existence(root)

    #Check whether a file exists
    def _check_existence(self, path):
        if not path:
            return True  # for terminating recursion
        elif os.path.isfile(path):
            return True
        return False

    def _assert_file_extn(self, asm_file_name, extn='.o'):
        """Take asm file-name format: filename.oldextn or filename
           change to format:filename.extn names the asm file to be a .asm file"""
        _basename = self.template.rsplit('.', 1)
        return _basename[0] + extn

    def _obj_filename(self, file_name):
        """Take file-name format:filename.* and change to
           format:filename.s names the asm file to be a .s file"""
        return self._assert_file_extn(file_name, '.o')

    def _asm_filename(self, file_name):
        """Take file-name format:filename.*
		   change to format:filename.asm"""
        return self._assert_file_extn(file_name, '.asm')

    def _map_filename(self, file_name):
        """Take file-name format:filename.*
		   change to format:filename.map"""
        return self._assert_file_extn(file_name, '.map')


class Extract_Asm(Disassembly):
    """Extract Assembly from an assembly file in text
       and create a map in memory of function-name : assembly
       to be used for annotation"""
    def __init__(self, file_name):
        self._filename = file_name
        self._asm = []
        self._asm_units = {}

        # python-3/2 dis-ambiguation
        if sys.version[0] == '3':
            super().__init__(self._filename)
        elif sys.version[0] == '2':
            super(Extract_Asm, self).__init__(self._filename)

    @property
    def filename(self):
        return self._filename

    @filename.setter
    def filename(self, file_name):
        self._filename = file_name

    @property
    def asm(self):
        return self._asm

    @asm.setter
    def asm(self, asm_txt):
        self._asm = asm_txt

    @property
    def asm_units(self):
        return self._asm_units

    @asm_units.setter
    def asm_units(self, function_list):
        self._asm_units = function_list

    def add_func(self, name, body, addr, offset=False):
        if type(body) is not list:
            self._asm_units[name] = [(addr, offset, body)]
        else:
            self._asm_units[name] = (addr, offset, body)

    def del_func(self, name):
        return self._asm_units.pop(name, None)

    def copy_to_mem(self, filename=None):
        if filename is None:
            filename = self._filename

        self._asm = util.copy_to_list(filename)
        # append new line so that pickle function recognizes delimiter
        self._asm.append(None)
        return self._asm

    #Parser to pick up each file within object file
    #and create a map with key being function name and value being a
    #list of lines for that function from the object file
    def pickle_asm(self):
        func_hdr_match = re.compile(
            '^\\s*(?P<address>[0-9a-fA-F]+)\\s+<(?P<symbol>[^>]*)>:\\s*$')
        search_for_function = True
        func_name = None
        func_address = None
        func_body = []

        for line in self._asm:
            if search_for_function:
                func_name = None
                func_address = None
                func_body = []

                match = func_hdr_match.match(line)
                if not match:
                    continue
                else:
                    func_name = match.group("symbol")
                    func_address = int(match.group("address"), 16)
                    search_for_function = False
            else:
                # We are within a function -
                # 1) write contents to list
                #      - or -
                # 2) exit function
                if line:
                    func_body.append(line)
                else:
                    search_for_function = True
                    is_offset = lambda x: True if x == 0 else False
                    self.add_func(
                        func_name, func_body, func_address, offset=True)

    # utility function to return a list of functions found within the object file
    def function_list(self):
        return [idx for idx in self._asm_units.keys()]


if __name__ == '__main__':
    xtrct_asm = Extract_Asm(sys.argv[1])
    _asm = xtrct_asm.copy_to_mem(xtrct_asm.asm_file)
    xtrct_asm.pickle_asm()
    func_names = [key for key in xtrct_asm.asm_units.keys()]
    print("function names = {0}".format(func_names))
    for name in func_names:
        print("=============== asm unit [ {0} ] =============".format(name))
        addr, is_offset, _txt = xtrct_asm.asm_units[name]
        print("{0:016x} : <{1}>".format(addr, name))
        for line in _txt:
            print("{0}".format(line))
        print("============= asm unit end [ {0} ] ===========".format(name))
