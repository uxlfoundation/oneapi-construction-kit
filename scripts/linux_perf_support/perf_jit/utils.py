#!/usr/bin/python3
# Copyright (c) Codeplay Software Ltd. All rights reserved.


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
