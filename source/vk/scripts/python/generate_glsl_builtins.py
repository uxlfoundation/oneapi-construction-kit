#!/usr/bin/env python

# Copyright (C) Codeplay Software Limited. All Rights Reserved.
"""Generate a header containing all the types and built-in functions in GLSL"""

import argparse
import json
import os

MAT_TYPES = ["float", "double"]
VEC_TYPES = MAT_TYPES + ["int", "uint", "bool"]
VEC_NAMES = {
    "float": "vec",
    "double": "dvec",
    "int": "ivec",
    "uint": "uvec",
    "bool": "bvec"
}
VEC_ELEMENTS = ["x", "y", "z", "w"]
SIZES = [2, 3, 4]
HANDLES = ["image", "texture", "sampler"]
HANDLE_TYPES = [
    "1D", "2D", "3D", "Cube", "2DRect", "1DArray", "2DArray", "CubeArray",
    "Buffer", "2DMS", "2DMSArray"
]

FUNCTIONS = []


def header(out_file):
    """Write the copyright stuff and include guards"""
    out_file.write(
        "// Copyright (C) Codeplay Software Limited. All Rights Reserved.\n\n"
    )
    out_file.write("#ifndef GL_BUILTINS_H_INCLUDED\n")
    out_file.write("#define GL_BUILTINS_H_INCLUDED\n\n")
    out_file.write("#include <cstdint>\n\n")


def types(out_file):
    """write the typedefs"""
    out_file.write("typedef unsigned int uint;\n")
    handle_types(out_file)
    vec_types(out_file)
    mat_types(out_file)


def handle_types(out_file):
    """typedef handle types"""
    handle_prefixes = ["", "i", "u"]
    for h in HANDLES:
        for t in HANDLE_TYPES:
            for p in handle_prefixes:
                out_file.write("typedef int64_t " + p + h + t + ";\n")
    out_file.write("typedef int64_t sampler;\n")
    out_file.write("typedef int64_t samplerShadow;\n\n")


def vec_types(out_file):
    """typedef vector types"""
    for t in VEC_TYPES:
        for s in SIZES:
            out_file.write("typdef struct " + VEC_NAMES[t] + str(s) + " {\n")
            for i in range(0, s):
                out_file.write("\t" + t + " " + VEC_ELEMENTS[i] + ";\n")
            out_file.write("} " + VEC_NAMES[t] + str(s) + ";\n\n")


def mat_types(out_file):
    """typedef matrix types"""
    for t in MAT_TYPES:
        prefix = ""
        if t == "double":
            prefix = "d"
        for s in SIZES:
            basename = prefix + "mat" + str(s)
            for n in SIZES:
                typename = basename + "x" + str(n)
                out_file.write("typedef struct " + typename + " {\n")
                out_file.write("\t" + VEC_NAMES[t] + str(n) + "[" + str(s) +
                               "];\n")
                out_file.write("} " + typename + ";\n\n")
            out_file.write("typedef " + basename + "x" + str(s) + " " +
                           basename + ";\n\n")


def functions(out_file):
    """write the functions from the json file"""
    for f in FUNCTIONS:
        out_file.write(f["return"] + " " + f["name"] + "(")
        if f["args"]:
            for arg in f["args"]:
                out_file.write(arg[1] + " " + arg[0] + ", ")
                out_file.seek(-2, os.SEEK_END)
                out_file.truncate()
        out_file.write(");\n")


def footer(out_file):
    """finish off the include guard"""
    out_file.write("\n#endif // GL_BUILTINS_H_INCLUDED\n")


def main():
    """main entry point into the program"""
    parser = argparse.ArgumentParser(prog="generate_glsl_builtins")
    parser.add_argument(
        "builtin_json", help="GLSL builtin functions json file")
    parser.add_argument("output_file", help="File to write the header to")
    args = parser.parse_args()
    with open(args.builtin_json, "r") as function_file:
        FUNCTIONS.extend(json.load(function_file))
    with open(args.output_file, "w+") as out_file:
        header(out_file)
        types(out_file)
        functions(out_file)
        footer(out_file)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
