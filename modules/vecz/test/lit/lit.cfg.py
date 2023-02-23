# Copyright (C) Codeplay Software Limited. All Rights Reserved.
"""Python configuration file for lit."""

import os
import lit.formats

# Name of the test suite.
config.name = 'Vecz'

# File extensions for testing.
config.suffixes = ['.hlsl', '.ll']

# The test format used to interpret tests.
config.test_format = lit.formats.ShTest(execute_external=False)

# The root path where tests are located.
config.test_source_root = os.path.dirname(__file__)
