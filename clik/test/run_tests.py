#! /usr/bin/env python3

# Copyright (C) Codeplay Software Limited. All Rights Reserved.

import sys
from runner import TestRunner

def main():
    runner = TestRunner()
    try:
        runner.load_profile()
        return runner.execute()
    except Exception as e:
        runner.print_exception(e)
        return 1

if __name__ == "__main__":
    sys.exit(main())
