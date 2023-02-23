#!/usr/bin/env python

# Copyright (C) Codeplay Software Limited. All Rights Reserved.
"""Generate the Vulkan ICD JSON manifest file."""

from argparse import ArgumentParser
from os.path import abspath, basename, dirname, join
from re import search
import json


def parse_vulkan_api_version(vulkan_header_path):
    """Parse the vulkan header version number."""
    with open(vulkan_header_path, 'r') as vulkan_header:
        for line in vulkan_header.readlines():
            if '#define VK_API_VERSION_1_0' in line:
                version_string = search(r'(\d, \d, \d)', line)
                return '.'.join(version_string.group().split(', '))


def generate_manifest(args):
    """Generate the Vulkan ICD JSON manifest file."""
    api_version = parse_vulkan_api_version(args.vulkan_header_path)
    manifest = {
        'file_format_version': '1.0.0',
        'ICD': {
            'library_path': '../../../lib/%s' % basename(args.library_path),
            'api_version': api_version
        }
    }
    with open(args.manifest_path, 'w') as manifest_file:
        json.dump(manifest, manifest_file, indent=2)


def main():
    """Main entry point."""
    parser = ArgumentParser(
        description='A utility to generate the Vulkan ICD JSON manifest, '
        'it includes the Vulkan header to determine the "api_version" to '
        'be used in the manifest file.')
    parser.add_argument(
        'library_path',
        help='absolute path to Vulkan driver including filename')
    parser.add_argument(
        'manifest_path',
        help='absolute path to the output manifest file to')
    args = parser.parse_args()

    script_dir = abspath(join(dirname(__file__), '..'))
    args.vulkan_header_path = join(script_dir, 'external', 'Khronos',
                                   'include', 'vulkan', 'vulkan.h')

    generate_manifest(args)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
