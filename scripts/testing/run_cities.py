#! /usr/bin/env python3

# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
