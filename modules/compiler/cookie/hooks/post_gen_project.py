#!/usr/bin/env python
"""
Post hook for the compiler
We use this to copy some files from the riscv target
"""

import os
import shutil

def copy_file_replace_riscv(src_dir, src_file, dst_dir, dst_file=None):
    if dst_file is None:
        dst_file = src_file
    src_path = os.path.join(src_dir, src_file)
    dst_path = os.path.join(dst_dir, dst_file)

    try:
        fin = open(src_path, "rt")

        # Create directory if it does not exist
        os.makedirs(dst_dir, exist_ok=True)
        #output file to write the result to
        fout = open(dst_path, "wt")
        #for each line in the input file
        for line in fin:
            #read replace the string and write to output file
            line = line.replace('riscv', '{{cookiecutter.target_name}}')
            line = line.replace('RISCV',
                                '{{cookiecutter.target_name}}'.upper())
            line = line.replace('Riscv',
                                '{{cookiecutter.target_name}}'.capitalize())
            line = line.replace('RISC-V',
                                '{{cookiecutter.target_name}}'.upper())
            line = line.replace('risc-v', '{{cookiecutter.target_name}}')
            line = line.replace('refsi_m1', '{{cookiecutter.target_name}}')            
            fout.write(line)
        #close input and output files
        fin.close()
        fout.close()

    except Exception as e:
        print("Error: {}".format(str(e)))
        os._exit(1)


def remove_file(dir, file):
    path = os.path.join(dir, file)
    if os.path.isfile(path):
        os.remove(path)

# For now we copy some files from riscv target
base_computeaorta_dir = '{{cookiecutter.base_computeaorta_dir}}'
external_dir = '{{cookiecutter.external_dir}}'
base_target_src_dir = os.path.join(base_computeaorta_dir,
                               'modules/compiler/targets')
base_target_dst_dir = os.path.join(external_dir, 'compiler')

examples_dir = os.path.join(base_computeaorta_dir, 'examples')
examples_refsi_dir = os.path.join(examples_dir, 'refsi')
examples_refsi_m1_dir = os.path.join(examples_refsi_dir, 'refsi_m1')
examples_refsi_m1_compiler_dir = os.path.join(examples_refsi_m1_dir, 'compiler/refsi_m1')
examples_refsi_m1_compiler_source_dir = os.path.join(examples_refsi_m1_compiler_dir, 'source')
examples_refsi_m1_compiler_include_dir = os.path.join(examples_refsi_m1_compiler_dir, 'include/refsi_m1')

src_target_dir = os.path.join(base_target_src_dir, 'riscv')
dst_target_dir = os.path.join(base_target_dst_dir, '{{cookiecutter.target_name}}')

dst_extension_dir = os.path.join(dst_target_dir, 'extension')

src_include_dir = os.path.join(src_target_dir, 'include/riscv')
dst_include_dir = os.path.join(dst_target_dir,
                               'include/{{cookiecutter.target_name}}')

src_source_dir = os.path.join(src_target_dir, 'source')
dst_source_dir = os.path.join(dst_target_dir, 'source')

src_passes_dir = os.path.join(src_source_dir, 'passes')
dst_passes_dir = os.path.join(dst_source_dir, 'passes')

dst_test_dir = os.path.join(dst_target_dir, 'test')

if "refsi_wrapper_pass" in "{{cookiecutter.feature}}".split(";"):
    copy_file_replace_riscv(examples_refsi_m1_compiler_source_dir, "refsi_wrapper_pass.cpp",
                        dst_passes_dir, "refsi_wrapper_pass.cpp")
    copy_file_replace_riscv(examples_refsi_m1_compiler_include_dir, "refsi_wrapper_pass.h",
                        dst_include_dir)
else:
    os.remove(os.path.join(dst_test_dir, "refsi_tutorial_wrapper_pass.ll"))

if not "clmul" in "{{cookiecutter.feature}}".split(";"):
    os.remove(os.path.join(dst_test_dir, "clmul_replace.ll"))
    shutil.rmtree(dst_extension_dir)
