#!/usr/bin/env python

# Copyright (C) Codeplay Software Limited. All Rights Reserved.
""" Import. """
import os
import subprocess

for f in os.listdir("../modules/"):
    if (os.path.isfile(os.path.join("../modules/", f)) and
            f.split(".")[-1] == "py"):
        subprocess.call(["pep8", os.path.join("../modules", f)])
