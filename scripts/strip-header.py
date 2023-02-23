#!/usr/bin/env python
"""Strip metadata header from binaries output from clc or clGetProgramInfo."""

from argparse import ArgumentParser
from os.path import splitext


def main():
    """Command line entry point."""
    cli = ArgumentParser(description=__doc__)
    cli.add_argument('bin')
    args = cli.parse_args()
    with open(args.bin, 'rb') as binary:
        content = binary.read()
    elf = content.find(b'ELF')
    name, ext = splitext(args.bin)
    stripped = '%s-stripped%s' % (name, ext)
    with open(stripped, 'wb') as binary:
        binary.write(content[elf - 1:])


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        exit(130)
