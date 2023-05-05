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

def create_profile():
    """ Create a new instance of a profile. This is usually called once."""
    return QEMUProfile()

class QEMUProfile(Profile):
    """ This profile executes tests built for another architecture by executing
    them with QEMU """
    def create_run(self, test, worker_state=None):
        """ Create a new test run from a test description.  """
        return QEMUTestRun(self, test)

    def add_options(self, parser):
        """ Add command-line options that can be used with the profile to the
         argument parser. """
        super(QEMUProfile, self).add_options(parser)

        parser.add_argument("--qemu", dest="qemu", type=str,
            help="Path to the QEMU executable to emulate tests.")
        parser.add_argument("--ld", dest="ld_path", type=str,
            help="Path to the dynamic loader executable to use.")

class QEMUTestRun(CTSTestRun):
    """ Represent the execution of a particular test. """
    def __init__(self, profile, test):
        super(QEMUTestRun, self).__init__(profile, test)

    def execute(self):
        """ Execute the test and wait for completion. """
        exe = self.test.executable
        args = self.profile.args
        working_dir = args.binary_path
        exe_path = os.path.join(os.path.realpath(working_dir), exe.path)

        # Add user-defined environment variables.
        env = {}
        for key, value in self.profile.args.env_vars:
            env[key] = value

        # Add user-defined library paths, making them absolute in the process.
        lib_paths = []
        for user_path in args.lib_paths:
            lib_paths.append(os.path.realpath(user_path))

        # Support `qemu-<target> -L <sysroot>` invocations by splitting the
        # value of the --qemu option, the first item being the path to
        # `qemu-<target>` binary and subsequent items optional arguments.
        qemu = args.qemu.split()[0]
        qemu_args = args.qemu.split()[1:]
        if args.ld_path:
            # Specifying the dynamic loader is not required when `-L <sysroot>`
            # is passed to `qemu-<target>`.
            qemu_args.append(args.ld_path)
            if lib_paths:
                # The `--library-path` option is provided by the dynamic loader
                # and will result in an error if it is not present.
                qemu_args.append("--library-path")
                qemu_args.append(":".join(lib_paths))
        qemu_args.append(os.path.join(args.binary_path, exe_path))
        qemu_args.extend(self.test.arguments)

        self.create_process(qemu, qemu_args, working_dir, env)
        self.analyze_process_output()
