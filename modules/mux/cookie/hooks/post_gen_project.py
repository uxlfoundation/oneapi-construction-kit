#!/usr/bin/env python
"""
Post hook for mux
We use this to copy some files from the riscv target and delete some files
"""

import os

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
            line = line.replace('Riscv',
                                '{{cookiecutter.target_name}}'.capitalize())         
            line = line.replace('RISCV',
                                '{{cookiecutter.target_name}}'.upper())
            line = line.replace('RISC-V',
                                '{{cookiecutter.target_name}}'.upper())
            if "Copyright (C) Codeplay Software" in line:
                line = ("/// Copyright (C) Codeplay Software Limited. All Rights Reserved.\n")
                if "{{cookiecutter.copyright_name}}" != "":
                    line += ("/// Additional changes Copyright (C) {{cookiecutter.copyright_name}}. All Rights\n/// Reserved.\n")
            # Replace 64 bit for device_info.cpp since we currently copy risc-v base file
            if "{{cookiecutter.capabilities_atomic64}}" == "false" and "device_info.cpp" in src_path:
                line = line.replace(" | mux_atomic_capabilities_64bit", "")
            fout.write(line)
        #close input and output files
        fin.close()
        fout.close()

    except Exception as e:
        print("Error: {}".format(str(e)))
        os._exit(1)


# For now we copy some files from riscv target
base_computeaorta_dir = '{{cookiecutter.base_computeaorta_dir}}'
external_dir = '{{cookiecutter.external_dir}}'

base_target_src_dir = os.path.join(base_computeaorta_dir, 'modules/mux/targets')
base_target_dst_dir = os.path.join(external_dir, 'mux')

src_target_dir = os.path.join(base_target_src_dir, 'riscv')
dst_target_dir = os.path.join(base_target_dst_dir, '{{cookiecutter.target_name}}')

src_include_dir = os.path.join(src_target_dir, 'include/riscv')
dst_include_dir = os.path.join(dst_target_dir,
                               'include/{{cookiecutter.target_name}}')

src_sources_dir = os.path.join(src_target_dir, 'source')
dst_sources_dir = os.path.join(dst_target_dir, 'source')

src_external_dir = os.path.join(src_target_dir, 'external')
dst_external_dir = os.path.join(dst_target_dir, 'external')

copy_file_replace_riscv(src_include_dir, "device_info_get.h", dst_include_dir)
copy_file_replace_riscv(src_include_dir, "buffer.h", dst_include_dir)
copy_file_replace_riscv(src_include_dir, "command_buffer.h", dst_include_dir)
copy_file_replace_riscv(src_include_dir, "memory.h", dst_include_dir)
copy_file_replace_riscv(src_include_dir, "queue.h", dst_include_dir)
copy_file_replace_riscv(src_include_dir, "semaphore.h", dst_include_dir)
copy_file_replace_riscv(src_include_dir, "device.h", dst_include_dir)
copy_file_replace_riscv(src_include_dir, "executable.h", dst_include_dir)
copy_file_replace_riscv(src_include_dir, "kernel.h", dst_include_dir)
copy_file_replace_riscv(src_include_dir, "query_pool.h", dst_include_dir)
copy_file_replace_riscv(src_include_dir, "device_info.h", dst_include_dir)
copy_file_replace_riscv(src_include_dir, "hal.h", dst_include_dir)
copy_file_replace_riscv(src_include_dir, "riscv.h", dst_include_dir,
                        "{{cookiecutter.target_name}}.h")
copy_file_replace_riscv(src_include_dir, "fence.h", dst_include_dir)

copy_file_replace_riscv(src_sources_dir, "semaphore.cpp", dst_sources_dir)
copy_file_replace_riscv(src_sources_dir, "image.cpp", dst_sources_dir)
copy_file_replace_riscv(src_sources_dir, "queue.cpp", dst_sources_dir)
copy_file_replace_riscv(src_sources_dir, "memory.cpp", dst_sources_dir)
copy_file_replace_riscv(src_sources_dir, "command_buffer.cpp", dst_sources_dir)
copy_file_replace_riscv(src_sources_dir, "executable.cpp", dst_sources_dir)
copy_file_replace_riscv(src_sources_dir, "query_pool.cpp", dst_sources_dir)
copy_file_replace_riscv(src_sources_dir, "hal.cpp", dst_sources_dir)
copy_file_replace_riscv(src_sources_dir, "device_info.cpp", dst_sources_dir)
copy_file_replace_riscv(src_sources_dir, "device.cpp", dst_sources_dir)
copy_file_replace_riscv(src_sources_dir, "buffer.cpp", dst_sources_dir)
copy_file_replace_riscv(src_sources_dir, "fence.cpp", dst_sources_dir)
