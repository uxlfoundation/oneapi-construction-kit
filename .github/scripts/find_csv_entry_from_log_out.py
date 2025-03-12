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
"""Rudimentary script to take the fail list output of run_cities and original csv file
to produce an override file. Writes to stdout"""

import sys
import re
if len(sys.argv) < 4:
    print("usage:", sys.argv[0], "<log_file> <csv_file> <extra_text_per_column>")
    sys.exit(1)
try:
    log_file = open(sys.argv[1], "r")
    for line in log_file:
        m = re.match(r".*/([^/]*)/(.*)", line)
        if m:
            csv_file = open(sys.argv[2], "r")
            for csv_line in csv_file:
                if m.group(1) in csv_line and m.group(2) in csv_line:
                    print(csv_line.strip('\n') + sys.argv[3])
except Exception as e:
    print('Error: ', e)
