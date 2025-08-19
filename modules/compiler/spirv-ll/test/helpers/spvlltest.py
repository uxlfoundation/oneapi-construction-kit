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
"""Python configuration file for lit."""

import os
import lit.formats

class SpvllTestFormat(lit.formats.ShTest):
    """SpvllTestFormat extends the built-in ShTest, but provides a special
    per-test substitution which resolves to the spv file matching the source
    file.
    """
    def __init__(self, execute_external):
        super().__init__(execute_external)

    def execute(self, test, litConfig):
        fileName, _ = os.path.splitext(test.getExecPath())
        fileSub = ('%spv_file_s', f'{fileName}.spv')
        return lit.TestRunner.executeShTest(test, litConfig,
                                            self.execute_external,
                                            self.extra_substitutions + [fileSub],
                                            self.preamble_commands)
