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
from city_runner.profile import Profile
from city_runner.cts import CTSTestRun

DEFAULT_BINARY_PATH = "/data/local/tmp"
"""
Path to a directory where executables can be copied to and executed without
root access to the device.
"""

def create_profile():
    """ Create a new instance of a profile. This is usually called once."""
    return AndroidProfile()

class AndroidProfile(Profile):
    """ This profile executes tests remotely on Android devices using ADB. """
    def create_run(self, test, worker_state=None):
        """ Create a new test run from a test description.  """
        return AndroidTestRun(self, test)

    def add_options(self, parser):
        """ Add command-line options that can be used with the profile to the
         argument parser. """
        super(AndroidProfile, self).add_options(parser)
        parser.add_argument("--adb", type=str, default="adb",
            help="Path to the adb executable to use to run tests remotely.")
        parser.add_argument("--device-serial", type=str,
            help="Optional serial of the device to connect to.")
        self.set_default(parser, "jobs", 1)
        self.set_default(parser, "binary_path", DEFAULT_BINARY_PATH)

class AndroidTestRun(CTSTestRun):
    """ Represent the execution of a particular test. """
    def __init__(self, profile, test):
        super(AndroidTestRun, self).__init__(profile, test)

    def execute(self):
        """ Execute the test and wait for completion. """
        exe = self.test.executable
        args = self.profile.args

        # Build the list of library paths.
        lib_dirs = [args.binary_path]
        for user_path in args.lib_paths:
            lib_dirs.append(user_path)

        # Build the 'inner' adb command we want to execute.
        exe_path = os.path.join(args.binary_path, exe.path)
        inner_cmd = [exe_path] + self.test.arguments
        parts = []
        parts.append("cd %s" % args.binary_path)
        if lib_dirs:
            env_var_text = ":".join(lib_dirs)
            parts.append("export LD_LIBRARY_PATH='%s'" % env_var_text)
        for key, value in self.profile.args.env_vars:
            parts.append("export %s='%s'" % (key, value))
        parts.append(" ".join(inner_cmd))
        remote_cmd = " && ".join(parts)

        # Build the 'outer' adb command.
        adb_args = []
        if args.device_serial:
            adb_args.append("-s %s" % args.device_serial)
        adb_args.append("shell")
        adb_args.append(remote_cmd)

        # Execute the 'outer' adb command.
        self.create_process(args.adb, adb_args)
        self.analyze_process_output()
