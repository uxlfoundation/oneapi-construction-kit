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
import sys
from city_runner.profile import Profile
from city_runner.cts import CTSTestRun


def create_profile():
    """ Create a new instance of a profile. This is usually called once."""
    return DefaultProfile()


class DefaultProfile(Profile):
    """ This profile executes tests locally. """
    def create_run(self, test, worker_state=None):
        """ Create a new test run from a test description.  """
        return DefaultTestRun(self, test)
    def add_options(self, parser):
        """ Add command-line options that can be used with the profile to the
         argument parser. """
        super(DefaultProfile, self).add_options(parser)

        parser.add_argument("--prepend-path", dest="prepend_path", type=str,
            help="Prepend to path e.g. qemu-riscv64 -L /usr/riscv64-linux-gnu.")

class DefaultTestRun(CTSTestRun):
    """ Represent the execution of a particular test. """
    def __init__(self, profile, test):
        super(DefaultTestRun, self).__init__(profile, test)

    def execute(self):
        """ Execute the test and wait for completion. """
        exe = self.test.executable
        working_dir = self.profile.args.binary_path

        # Technically this if statement is redundant, the 'else' branch should
        # be correct in all cases.  However, we had an issue with
        # os.path.realpath on Cygwin python where it was merging Unix style and
        # Windows style paths illegally.  Given that when using Cygwin we were
        # always using absolute paths we just skip os.path.realpath for those
        # cases, as the function (should be) a no-op.  Thus relative paths on
        # Cygwin are known to be broken.  Note: os.path.isabs is also "broken"
        # on Cygwin, hence the explicit string check.
        if working_dir[1:3] == ":\\":
            exe_path = os.path.join(working_dir, exe.path)
        else:
            exe_path = os.path.join(os.path.realpath(working_dir), exe.path)

        if not sys.platform.startswith('win'):
            # Add -b/--binary-dir to PATH when not executing on Windows. This
            # is useful when testing binary or spir-v offline compilation mode
            # in the OpenCL-CTS.
            paths = os.environ['PATH'].split(':')
            if working_dir not in paths:
                paths.insert(0, working_dir)
                os.environ['PATH'] = ':'.join(paths)

        # Add user-defined environment variables.
        env = {}
        for key, value in self.profile.args.env_vars:
            env[key] = value

        # Parse the list of library paths from the environment.
        env_var_name = None
        env_var_separator = None
        lib_dirs = []
        if sys.platform.startswith("win"):
            env_var_name = "PATH"
            env_var_separator = ";"
        else:
            env_var_name = "LD_LIBRARY_PATH"
            env_var_separator = ":"
        if env_var_name:
            try:
                orig_path = os.environ[env_var_name]
                if orig_path:
                    lib_dirs.extend(orig_path.split(env_var_separator))
            except KeyError:
                pass

        # Add user-defined library paths, making them absolute in the process.
        for user_path in self.profile.args.lib_paths:
            lib_dirs.append(os.path.realpath(user_path))
        if lib_dirs:
            env[env_var_name] = env_var_separator.join(lib_dirs)

        # Support `<command> <args>` invocations by splitting the
        # value of the --prepend_args option, the first item being the path to
        # `<command>` binary and subsequent items optional arguments.
        test_arguments = self.test.arguments
        if self.profile.args.prepend_path:
            original_exe_path = exe_path
            exe_path = self.profile.args.prepend_path.split()[0]
            test_arguments = self.profile.args.prepend_path.split()[1:]
            test_arguments.append(os.path.join(self.profile.args.binary_path, original_exe_path))
            test_arguments.extend(self.test.arguments)
        self.create_process(exe_path, test_arguments, working_dir, env)
        self.analyze_process_output()
