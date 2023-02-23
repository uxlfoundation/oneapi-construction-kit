#!/usr/bin/env python
"""
Generate random test cases, see if they are interesting, and try to reduce them.
"""

import subprocess
import concurrent.futures as confu
import os
import sys
import copy
import shutil
import argparse
import tempfile
import logging

# The 'interesting' module checks if a test case is interesting. See the README
# for more details.
import interesting

CLANG = "clang"
CREDUCE = "creduce"
CLSMITH = "CLSmith"
CLSMITH_OPTS = ["--vectors"]


def create_testcase():
    """
    Generates a random OpenCL kernel and returns the file descriptor and the
    filename.

    Returns:
        tuple(file, str): The file descriptor and the filename
        or None on failure
    """
    # Create a temporary file for the output
    (fd, filename) = tempfile.mkstemp(suffix=".c")
    # Call CLSmith
    cmd = [CLSMITH] + CLSMITH_OPTS + ["-o", filename]
    try:
        subprocess.check_call(cmd)
    except Exception:
        logging.exception("CLSmith failed to execute:")
        os.close(fd)
        os.remove(filename)
        return None
    # Return the test file
    logging.info("Created random kernel at %s", filename)
    return (fd, filename)


def preprocess_testcase(infd, infilename):
    """
    Preprocess the file generated from CLSmith. Currently, that includes running
    it through the clang preprocessor. It returns the file descriptor and the
    filename.

    Arguments:
        indf (file)     : The file descriptor of the input kernel
        infilename (str): The filename of the input kernel

    Returns:
        tuple(file, str): The file descriptor and the filename
        or None on failure
    """
    if not infd or not infilename:
        logging.warning("Invalid input file")
        return None
    # Create a temporary file for the output
    (outfd, outfilename) = tempfile.mkstemp(suffix=".c")
    # Call clang's preprocessor
    cmd = [CLANG, "-I", ".", "-E", "-CC", "-o", outfilename, infilename]
    try:
        logging.debug("Starting preprocessing of kernel %s", infilename)
        subprocess.check_call(cmd)
    except Exception:
        logging.exception("Could no preprocess kernel:")
        os.close(infd)
        os.close(outfd)
        os.remove(outfilename)
        os.remove(infilename)
        return None
    # Cleanup and return
    os.close(infd)
    os.remove(infilename)
    logging.debug("Preprocessed kernel %s to %s", infilename, outfilename)
    return (outfd, outfilename)


def remove_barriers(infd, infilename):
    """
    Removes barrier calls from the file. It returns the file descriptor and the
    filename.

    Arguments:
        indf (file)     : The file descriptor of the input kernel
        infilename (str): The filename of the input kernel

    Returns:
        tuple(file, str): The file descriptor and the filename
        or None on failure
    """
    if not infd or not infilename:
        logging.warning("Invalid input file")
        return None
    # Create a temporary file for the output
    (outfd, outfilename) = tempfile.mkstemp(suffix=".c")
    logging.debug("Starting removal of barriers on kernel %s", infilename)
    with open(infilename, "r") as inf, open(outfilename, "w") as outf:
        n = 1
        for line in inf:
            if "barrier" not in line:
                outf.write(line)
            else:
                logging.debug("Removed \"%s\" from %s:%d", line.strip(),
                              infilename, n)
            n += 1
    # Cleanup and return
    os.close(infd)
    os.remove(infilename)
    logging.debug("Removed barriers from %s to %s", infilename, outfilename)
    return (outfd, outfilename)


def create_wrapper(kernel, interesting_py_args, filename):
    """
    Creates a wrapper bash script for running the interestingness test on
    C-Reduce

    Arguments:
        kernel (str)              : Filename for the kernel to reduce
        interesting_py_args (dict): Arguments to forward to interesting.py
        filename (str)            : Filename for the wrapper
    """
    if not kernel:
        logging.critical("Invalid input kernel")
        raise ValueError("kernel has to be a filename")
    # Delete existing wrapper
    if os.path.isfile(filename):
        os.remove(filename)
    # We need to update some of the arguments
    interesting_py_args['loglevel'] = logging.ERROR
    interesting_py_args['logfile'] = None
    # Create the wrapper and set the permissions
    with open(filename, "w") as outf:
        try:
            outf.write("#!/usr/bin/env python\n")
            outf.write("import sys\n")
            outf.write("sys.path.append(\'%s\')\n" %
                       os.path.dirname(os.path.abspath(__file__)))
            outf.write("import interesting\n")
            outf.write("is_interesting = interesting.run(**%s)\n" %
                       str(interesting_py_args))
            outf.write("sys.exit(0) if is_interesting else sys.exit(1)\n")
        except Exception as e:
            logging.critical("Could not create the wrapper file: %s", e)
            raise e
    os.chmod(filename, 0o750)
    logging.debug("Created wrapper for kernel %s at %s", kernel, filename)


def reduce_testcase(kernel, interesting_py_args):
    """
    Calls C-Reduce on the kernel. The file should be preprocessed before calling
    this function.

    Arguments:
        kernel (str)              : Filename for the kernel to reduce
        interesting_py_args (dict): Arguments to forward to interesting.py

    Returns:
        str: The filename of the reduced kernel
        or None on failure
    """
    if not kernel:
        logging.critical("Invalid input kernel")
        raise ValueError("kernel has to be a filename")
    # Create a wrapper script for the interestingness test
    wrapper_filename = "_wrapper_%s_ag.py" % kernel.split('.')[0]
    create_wrapper(kernel, interesting_py_args, wrapper_filename)
    # Call C-Reduce
    cmd = [CREDUCE, wrapper_filename, kernel]
    try:
        logging.info("Starting reduction of kernel %s", kernel)
        subprocess.check_call(cmd, stderr=subprocess.STDOUT)
    except Exception:
        logging.exception("Could not reduce kernel %s:", kernel)
        return None

    logging.info("Kernel %s reduced successfully", kernel)
    return kernel


def main(argv):
    """ For running this script from the command line """
    global CLANG
    global CLSMITH
    global CLSMITH_OPTS
    global CREDUCE

    argparser = argparse.ArgumentParser(
        description="Find interesting OpenCL test cases and try to reduce them.")
    argparser.add_argument(
        '--loglevel',
        action='store',
        choices=['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
        default='INFO')
    argparser.add_argument('--logfile', action='store')
    argparser.add_argument('--reducer-threads',
                           action='store',
                           nargs='?',
                           const=1,
                           metavar="MAX THREADS",
                           help="Run the reducer in a separate thread")
    argparser.add_argument('--clsmith-path', action='store')
    argparser.add_argument(
        '--clsmith-options',
        action='store',
        help="\" --opt1 --opt2\" (note the space at the beginning)")
    argparser.add_argument('--clang-path', action='store')
    argparser.add_argument('--creduce-path', action='store')
    argparser.add_argument(
        '--stop',
        action='store_true',
        help="Stop after finding the first interesting case")
    argparser.add_argument(
        '--no-reduce',
        action='store_true',
        help="Don't try to reduce the interesting test cases")
    interesting.register_fnr_options(argparser)
    args = argparser.parse_args(argv[1:])

    log_level = getattr(logging, args.loglevel.upper(), None)
    if log_level == logging.DEBUG:
        fmt = "%(asctime)s - %(levelname)s @ [%(funcName)s:%(lineno)s] %(message)s"
    else:
        fmt = "%(asctime)s : %(levelname)s : %(message)s"
    logging.basicConfig(level=log_level, format=fmt, filename=args.logfile)

    CLANG = args.clang_path or CLANG
    CLSMITH = args.clsmith_path or os.path.join(os.getcwd(), CLSMITH)
    CLSMITH_OPTS = args.clsmith_options.split(
    ) if args.clsmith_options else CLSMITH_OPTS
    CREDUCE = args.creduce_path or CREDUCE
    interesting_py_args = vars(args)
    if not 'cl_launcher_path' in interesting_py_args or not interesting_py_args[
            'cl_launcher_path']:
        interesting_py_args['cl_launcher_path'] = os.path.join(os.getcwd(),
                                                               "cl_launcher")
    logging.debug("%r", argv)
    logging.debug("CLANG: %r", CLANG)
    logging.debug("CLSMITH: %r", CLSMITH)
    logging.debug("CLSMITH_OPTS: %r", CLSMITH_OPTS)
    logging.debug("CREDUCE: %r", CREDUCE)

    if args.reducer_threads and not args.no_reduce:
        args.reducer_threads = int(args.reducer_threads)
        reducers = confu.ThreadPoolExecutor(args.reducer_threads)
        logging.debug("Started a process pool with %d processes",
                      args.reducer_threads)

    found_it = False
    i = 0

    while not found_it:
        fd, filename = create_testcase() or (None, None)
        fd, filename = preprocess_testcase(fd, filename) or (None, None)
        fd, filename = remove_barriers(fd, filename) or (None, None)

        if fd and filename:
            logging.info("Testing %s for interestingness", filename)
            interesting_py_args['clprogram'] = filename
            if interesting.run(**interesting_py_args):
                logging.info("%s (#%d) is interesting", filename, i)
                kernel_filename = "CLProg_a%d.c" % i
                shutil.copyfile(filename, kernel_filename)
                print("Interesting testcase found at iteration %d!" % i)
                # Skip the reduction steps
                if not args.no_reduce:
                    interesting_py_args['clprogram'] = kernel_filename
                    if args.reducer_threads:
                        reducers.submit(reduce_testcase,
                                        copy.deepcopy(kernel_filename),
                                        copy.deepcopy(interesting_py_args))
                    else:
                        reduce_testcase(kernel_filename,
                                        copy.deepcopy(interesting_py_args))
                if args.stop:
                    found_it = True

            # Cleanup
            os.close(fd)
            os.remove(filename)

            i += 1
            if i % 10 == 0:
                print("At iteration %d..." % i)

    # Wait for all the background jobs to finish
    if args.reducer_threads:
        reducers.shutdown(True)


if __name__ == "__main__":
    main(sys.argv)
