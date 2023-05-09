# Copyright (C) Codeplay Software Limited. All Rights Reserved.
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
        execPath = test.getExecPath()
        execBaseName = os.path.basename(execPath)
        fileName, _ = os.path.splitext(execBaseName)
        fileSub = ('%spv_file_s', f'{fileName}.spv')
        return lit.TestRunner.executeShTest(test, litConfig,
                                            self.execute_external,
                                            self.extra_substitutions + [fileSub],
                                            self.preamble_commands)
