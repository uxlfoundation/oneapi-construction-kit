# Copyright (C) Codeplay Software Limited. All Rights Reserved.
""" Dynamically find scripts in extensions folder

We can't do this statically since we want to hide customer details.
"""

from os.path import dirname, basename, isfile, join
from glob import glob

python_file_pattern = join(dirname(__file__), "*.py")

extension_scripts = glob(python_file_pattern)

__all__ = [
    basename(f)[:-3] for f in extension_scripts
    if isfile(f) and not f.endswith("__init__.py")
]
