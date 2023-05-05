#!/usr/bin/python3
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


def copy_to_list(filename):
    lines = []
    with open(filename, "r") as fd:
        for line in fd:
            lines.append(line.rstrip("\r\n"))
    return lines


class LazyEval:
    def __init__(self, list_to_yield):
        self._iterable = list_to_yield
        self._size = len(list_to_yield)
        self.idx = 0

    @property
    def iterable(self):
        return self._iterable

    @iterable.setter
    def iterable(self, list_to_yield):
        self._iterable = list_to_yield

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, len):
        self._size = len

    def items(self):
        while self.idx < self.size:
            self.idx += 1
            yield self._iterable[self.idx - 1]

    def reset(self):
        self.idx = 0
