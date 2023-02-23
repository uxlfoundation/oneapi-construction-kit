#!/usr/bin/env python3
"""
Run cmakelint on CMakeLists.txt and *.cmake files found in the source tree
which do not contain substrings matching the following regular expression:

    (build|external|test.*OpenCL|platform.*android)
"""

import argparse
import os
import pathlib
import platform
import re
import subprocess
import sys

PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
CMAKELINTRC = os.path.join(PROJECT_DIR, '.cmakelintrc')


def find_cmakelint():
    """Find cmakelint on the PATH."""
    program = 'cmakelint'
    if platform.system() == 'Windows':
        program += '.exe'
    for path in os.environ['PATH'].split(os.path.pathsep):
        filepath = os.path.join(path, program)
        if os.path.isfile(filepath) and os.access(filepath, os.X_OK):
            return filepath
    raise FileNotFoundError(
        'could not find cmakelint on the PATH, run: pip install cmakelint')


def find_cmake_files():
    """Find CMakeLists.txt and *.cmake files to check."""
    project = pathlib.Path(PROJECT_DIR)
    files = []
    matcher = re.compile(r'(build|external|test.*OpenCL|platform.*android)')
    for path in [*project.rglob('CMakeLists.txt'), *project.rglob('*.cmake')]:
        if matcher.search(str(path)):
            continue
        files.append(os.path.relpath(path, PROJECT_DIR))
    return files


def main():
    """Command line entry point."""
    cli = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    cli.add_argument('-v',
                     '--verbose',
                     action='store_true',
                     help='output more information')
    args = cli.parse_args()
    command = [
        find_cmakelint(),
        '--config=%s' % os.path.relpath(CMAKELINTRC),
        *find_cmake_files(),
    ]
    if args.verbose:
        print('$', ' '.join(command), file=sys.stderr)
    process = subprocess.Popen(command,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
    out, _ = process.communicate()
    if process.returncode != 0:
        print(end=out.decode())
    sys.exit(process.returncode)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(130)
    except FileNotFoundError as error:
        print('error:', error, file=sys.stdout)
