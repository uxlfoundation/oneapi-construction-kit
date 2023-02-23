# This file is NOT licensed under the GPLv3, which is the license for the rest
# of YouCompleteMe.
#
# Here's the license text for this file:
#
# This is free and unencumbered software released into the public domain.
#
# Anyone is free to copy, modify, publish, use, compile, sell, or
# distribute this software, either in source code form or as a compiled
# binary, for any purpose, commercial or non-commercial, and by any
# means.
#
# In jurisdictions that recognize copyright laws, the author or authors
# of this software dedicate any and all copyright interest in the
# software to the public domain. We make this dedication for the benefit
# of the public at large and to the detriment of our heirs and
# successors. We intend this dedication to be an overt act of
# relinquishment in perpetuity of all present and future rights to this
# software under copyright law.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# For more information, please refer to <http://unlicense.org/>
"""YouCompleteMe extra configuration."""

import os
from sys import stderr
import ycm_core  # pylint: disable=import-error

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))

# These are the compilation flags that will be used in case there's no
# compilation database set (by default, one is not set).
# CHANGE THIS LIST OF FLAGS. YES, THIS IS THE DROID YOU HAVE BEEN LOOKING FOR.
FLAGS = [
    '-Wall',
    '-Wextra',
    '-Wdocumentation',
    '-std=c++11',
    '-x',
    'c++',
    # Add your project specific flags here
    '-Iexternal/googletest/include',
    '-Imodules/spirv-ll/external/SPIRV-Headers/include',
    '-Imodules/vecz/source/include',
    '-Isource/cl/include',
    '-Isource/cl/external/OpenCL-Headers',
    '-Isource/cl/source/compiler/include',
    '-Isource/cl/source/compiler_offline/include',
    '-Isource/cl/source/extension/include',
    '-Isource/cl/test/UnitCL/include',
    '-Isource/vk/include',
    '-Isource/vk/external/Khronos/include',
]

# Set this to the path to the directory containing the compile_commands.json
# file to use that instead of 'flags'. See here for more details:
# http://clang.llvm.org/docs/JSONCompilationDatabase.html
BUILD_DIRECTORY = os.environ.get('BUILD_DIR', 'build')

if os.path.exists(BUILD_DIRECTORY):
    # Load the compilation database.
    DATABASE = ycm_core.CompilationDatabase(BUILD_DIRECTORY)
    # Add build include directory to FLAGS.
    FLAGS.append('-I%s/include' % BUILD_DIRECTORY)
    FLAGS.append('-I%s/source/cl/include' % BUILD_DIRECTORY)
    FLAGS.append('-I%s/source/vk/test/UnitVK/source/shaders' % BUILD_DIRECTORY)
    # Look up the CMakeCache for additional information.
    with open(os.path.join(BUILD_DIRECTORY, 'CMakeCache.txt'), 'r') as cache:
        for line in cache.readlines():
            # Find the external LLVM include directory and add it to FLAGS.
            if line.startswith('CA_LLVM_INSTALL_DIR'):
                include = os.path.join(line[line.find('=') + 1:-1], 'include')
                FLAGS += [
                    '-D__STDC_LIMIT_MACROS', '-D__STDC_CONSTANT_MACROS',
                    '-isystem', include
                ]
            if line.startswith('CA_CL_STANDARD'):
                cl_standard = line[line.find('=') + 1:-1]
                if cl_standard == '1.2':
                    FLAGS += [
                        '-DCL_TARGET_OPENCL_VERSION=120',
                    ]
                if cl_standard == '3.0':
                    FLAGS += [
                        '-DCL_TARGET_OPENCL_VERSION=300',
                    ]
else:
    DATABASE = None
    stderr.write("Could not load '%s/copmile_commands.json'" % BUILD_DIRECTORY)

# Add module include directories to FLAGS.
MODULES_DIR = os.path.join(SCRIPT_DIR, 'modules')
for module in os.listdir(MODULES_DIR):
    include = os.path.join(MODULES_DIR, module, 'include')
    if os.path.isdir(include):
        FLAGS.append('-I%s' % include)
# Add in-tree ComputeMux runtime target include directories to FLAGS.
MUX_RUNTIME_TARGETS_DIR = os.path.join(MODULES_DIR, 'mux', 'targets')
for target in os.listdir(MUX_RUNTIME_TARGETS_DIR):
    target_dir = os.path.join(MUX_RUNTIME_TARGETS_DIR, target)
    if os.path.isdir(target_dir):
        include = os.path.join(target_dir, 'include')
        if os.path.isdir(include):
            FLAGS.append('-I%s' % include)
# Add in-tree ComputeMux compiler target include directories to FLAGS.
MUX_COMPILER_TARGETS_DIR = os.path.join(MODULES_DIR, 'compiler', 'targets')
for target in os.listdir(MUX_COMPILER_TARGETS_DIR):
    target_dir = os.path.join(MUX_COMPILER_TARGETS_DIR, target)
    if os.path.isdir(target_dir):
        include = os.path.join(target_dir, 'include')
        if os.path.isdir(include):
            FLAGS.append('-I%s' % include)


def make_relative_paths_absolute(flags, working_directory):
    """Make relative paths in flags absolute."""
    if not working_directory:
        return list(flags)
    new_flags = []
    make_next_absolute = False
    path_flags = ['-isystem', '-I', '-iquote', '--sysroot=']
    for flag in flags:
        new_flag = flag
        if make_next_absolute:
            make_next_absolute = False
            if not flag.startswith('/'):
                new_flag = os.path.join(working_directory, flag)
        for path_flag in path_flags:
            if flag == path_flag:
                make_next_absolute = True
                break
            if flag.startswith(path_flag):
                path = flag[len(path_flag):]
                new_flag = path_flag + os.path.join(working_directory, path)
                break
        if new_flag:
            new_flags.append(new_flag)
    return new_flags


def is_header_file(filename):
    """Returns True if filename has a header extension, False otherwise."""
    extension = os.path.splitext(filename)[1]
    return extension in ['.h', '.hxx', '.hpp', '.hh']


def get_compilation_info_for_file(filename):
    """The compilation_commands.json file generated by CMake does not have
    entries for header files. So we do our best by asking the db for flags for
    a corresponding source file, if any. If one exists, the flags for that file
    should be good enough."""
    if is_header_file(filename):
        basename = os.path.splitext(filename)[0]
        for extension in ['.cpp', '.cxx', '.cc', '.c', '.m', '.mm', '.h',
                          '.hpp', '.hh', '.hxx']:
            replacement_file = basename + extension
            if os.path.exists(replacement_file):
                compilation_info = DATABASE.GetCompilationInfoForFile(
                    replacement_file)
                if compilation_info.compiler_flags_:
                    return compilation_info
        return None
    return DATABASE.GetCompilationInfoForFile(filename)


# pylint: disable=invalid-name,unused-argument
def FlagsForFile(filename, **kwargs):
    """Returns compilation flags to YouCompleteMe."""
    if DATABASE and not is_header_file(filename):
        # Bear in mind that compilation_info.compiler_flags_ does NOT return a
        # python list, but a "list-like" StringVec object
        compilation_info = get_compilation_info_for_file(filename)
        if not compilation_info:
            return None
        final_flags = make_relative_paths_absolute(
            compilation_info.compiler_flags_,
            compilation_info.compiler_working_dir_)
    else:
        relative_to = os.path.dirname(os.path.abspath(__file__))
        final_flags = make_relative_paths_absolute(FLAGS, relative_to)
    return {'flags': final_flags, 'do_cache': True}


def Settings(**kwargs):
    """ycm-core/YouCompleteMe C Family language settings."""
    if kwargs['language'] != 'cfamily':
        return {}
    filename = kwargs['filename']
    _, extension = os.path.splitext(filename)
    if extension in ['.h', '.hh', '.hpp', '.hxx'] or not DATABASE:
        return {
            'flags': FLAGS,
            'include_paths_relative_to_dir': SCRIPT_DIR,
            'override_filename': filename,
        }
    compilation_info = DATABASE.GetCompilationInfoForFile(filename)
    if not compilation_info.compiler_flags_:
        return {}
    final_flags = list(compilation_info.compiler_flags_)
    return {
        'flags': final_flags,
        'include_paths_relative_to_dir':
        compilation_info.compiler_working_dir_,
        'override_filename': filename,
    }
