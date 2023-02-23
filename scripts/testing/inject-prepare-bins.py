#!/usr/bin/env python
# Copyright (C) Codeplay Software Limited. All Rights Reserved.
"""Prepare dumped program binaries for injection.

The OpenCL-Intercept-Layer supports injecting previously dumped program
binaries into an OpenCL application. This is a three step process:

1.  Run the application with :envvar:`CLI_DumpProgramBinaries=1` and
    :envvar:`CLI_DumpDir=/path/to/dump/binaries`.
2.  Run this script, details below.
3.  Run the application with :envvar:`CLI_InjectProgramBinaries=1`
    and :envvar:`CLI_DumpDir=/path/to/dump/binaries`, the binaries to be
    injected must exist in the ``Inject`` subdirectory of
    :envvar:`CLI_DumpDir`.

Given the requirements for step 3, step 2 ensures that the ``Inject``
subdirectory exists, then copies the binaries dumped in step 1 whilst also
renaming them so the OpenCL-Intercept-Layer's search will be successful.
"""

from __future__ import print_function

import os
import shutil
from argparse import ArgumentParser
from sys import exit


def main():
    """Command line script entry point."""
    cli = ArgumentParser()
    cli.add_argument('dump_dir')
    cli.add_argument('--clean', action='store_true')
    args = cli.parse_args()
    dump_dir = os.path.abspath(args.dump_dir)
    inject_dir = os.path.join(dump_dir, 'Inject')
    if args.clean and os.path.exists(inject_dir):
        shutil.rmtree(inject_dir)
    if not os.path.exists(inject_dir):
        os.makedirs(inject_dir)
    for entry in os.listdir(dump_dir):
        name, ext = os.path.splitext(entry)
        if ext == '.bin':
            # The OpenCL-Intercept-Layer does not search for files to inject
            # with have the <Compile Count> portion of the filename which is
            # present in the dumped program binaries. To fix this, split the
            # filename apart and delete the second to last element.
            target = name.split('_')
            del target[-2]
            shutil.copyfile(os.path.join(dump_dir, entry),
                            os.path.join(inject_dir, '_'.join(target)) + ext)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        exit(130)
