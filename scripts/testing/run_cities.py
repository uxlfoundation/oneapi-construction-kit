#! /usr/bin/env python3

# Copyright (C) Codeplay Software Limited. All Rights Reserved.

import sys

from city_runner.runner import CityRunner
from city_runner.profile import ProfileError


def main():
    runner = CityRunner()
    try:
        runner.load_profile()
    except ProfileError as error:
        print(f'error: {error}', file=sys.stderr)
        return 1
    try:
        return runner.execute()
    except Exception as error:
        runner.print_exception(error)
        return 1


if __name__ == "__main__":
    sys.exit(main())
