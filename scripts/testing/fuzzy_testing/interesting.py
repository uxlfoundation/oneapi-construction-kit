#! /usr/bin/env python
""" See if a test case is interesting """

import subprocess, threading
import os, sys
import argparse
import logging
import pyopencl as cl

CL_LAUNCHER = "cl_launcher"
CL_LAUNCHER_OPTS = ["-l", "1,1,1", "-g", "1,1,1"]
OCLGRIND = "oclgrind"
OCLGRIND_OPTS = ["--max-errors", "16", "--build-options", "-O0", "-Wall",
                 "--uninitialized"]
VECTORIZATION_FAILED_IGNORE_KEYWORDS = ["memcpy", "memset"]

REFERENCE_PLATFORMS = {}
CODEPLAY_PLATFORM = 0
DEVICE = 0
CHECK_IF_VECTORIZED = False
STDOUT_MUST_CONTAIN = ['']
STDERR_MUST_CONTAIN = ['']

TIMEOUT = 30.0  # seconds


class RunInThread(object):
    """
    This class is responsible for running a process in a separate thread
    and collecting it's output. It is also responsible for making sure that
    we do not deadlock by killing the process after a set timeout.
    """
    stdout = None
    stderr = None
    timed_out = False
    process = None

    def __init__(self, command, timeout=30):
        # Start the thread
        thread = threading.Thread(target=self.__run, args=(command, ))
        thread.start()

        # Wait for it to finish or to timeout
        thread.join(timeout)
        if thread.is_alive():
            self.process.terminate()
            thread.join(5)
            if thread.is_alive():
                self.process.kill()
                thread.join()
            self.timed_out = True

    def __run(self, command):
        """
        Thread entry point

        Arguments:
            command ([str]): The command to execute in a process
        """
        # Start the process
        self.process = subprocess.Popen(command,
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE)
        # Get its output (blocking call)
        self.stdout, self.stderr = self.process.communicate()

def populate_platforms():
    """
    Populate the Reference Platforms map and return the index of the Codeplay
    platform

    Returns:
        int: The ID for the Codeplay platform
    """
    platforms = cl.get_platforms()
    codeplay_index = None
    for index, platform in enumerate(platforms):
        if "Codeplay Software Ltd." in platform.vendor:
            codeplay_index = index
        else:
            REFERENCE_PLATFORMS[platform.name] = index

    if codeplay_index is None:
        logging.critical("Could not find the Codeplay OpenCL platform")
        raise RuntimeError("No Codeplay OpenCL implementation found")
    if len(REFERENCE_PLATFORMS) < 1:
        logging.critical("No reference platforms found")

    return codeplay_index


def truncate(name, max_len=20):
    """
    Truncate the given string

    Arguments:
        name (str)   : The string to truncate
        max_len (int): The maximum length after which to truncate
    """
    return (name[:max_len] + '..') if len(name) > max_len else name


def verify_with_oclgrind(clprogram):
    """
    Run the kernel in Oclgrind and check for errors

    Arguments:
        clprogram (str): The path to the kernel to run

    Returns:
        bool: True if no problems were detected
    """
    # Execute Oclgrind in a separate thread
    oclgrind = RunInThread(
        [OCLGRIND] + OCLGRIND_OPTS + [CL_LAUNCHER, "-f", clprogram, "-p", "0",
                                      "-d", "0"] + CL_LAUNCHER_OPTS,
        TIMEOUT * 30)

    # Check to see if Oclgrind actually completed successfully
    if oclgrind.timed_out:
        logging.info("Oclgrind: Timed out")
        return False

    out, err = oclgrind.stdout, oclgrind.stderr
    logging.debug("Oclgrind:\nOut: %r\nErr: %r\n", out, err)
    # Check if the compilation process was successful
    compiled = b"compilation terminated successfully" in out.lower()
    if not compiled:
        logging.info("Oclgrind: Compilation failed")
        return False

    # Check for issues in the Oclgrind output
    if b"uninitialized value" in err.lower():
        logging.info("Oclgrind: Uninitialized value detected")
        return False
    if b"uninitialized address" in err.lower():
        logging.info("Oclgrind: Uninitialized address detected")
        return False
    if b"invalid read" in err.lower():
        logging.info("Oclgrind: Invalid read detected")
        return False

    # We didn't find any issues so everything must be okay
    return True


def get_reference_run(clprogram):
    """
    Run the OpenCL kernel in all the reference implementation and check if the
    results match.

    Arguments:
        clprogram (str): The path to the kernel to run

    Returns:
        str: The output of any reference run if they all match
        or None otherwise
    """
    result = {}
    for platform, index in REFERENCE_PLATFORMS.items():
        result[platform] = None

        reference = RunInThread(
            [CL_LAUNCHER, "-f", clprogram, "-p", str(index), "-d", str(DEVICE)
             ] + CL_LAUNCHER_OPTS, TIMEOUT)

        out, err = reference.stdout, reference.stderr
        logging.debug("Reference[%d - %s]:\nOut: %r\nErr: %r\n", index,
                      truncate(platform), out, err)

        compiled = b"compilation terminated successfully" in out.lower()

        if compiled and not reference.timed_out:
            result[platform] = out.split(b"\n")
        else:
            if reference.timed_out:
                logging.info("Reference[%d - %s]: Timed out", index,
                             truncate(platform))
                result[platform] = platform + " timed out!"
            else:
                logging.info("Reference[%d- %s]: Compilation failed", index,
                             truncate(platform))
                result[platform] = platform + " did not compile!"
            return None

        # Accumulate all the results and check if they all match
        # This is done in the loop so that we can exit immediatelly after the
        # first mismatch
        random_result = result[next(iter(result))]
        if len(random_result) < 2 or not random_result[1]:
            logging.info("Reference: No result")
            return None
        if len(random_result) > 1:
            if b"error" in random_result[1].lower():
                logging.info("\"error\" found in output")
                return False
            if random_result[1] == "0,":
                logging.info("Zero output is not interesting")
                return False
        all_match = all(result[x] == random_result for x in result)
        if not all_match:
            logging.info("Reference: Mismatched results (\n%r\n)", result)
            return None

    # They are all the same anyway
    random_result = result[next(iter(result))]
    return random_result


def get_ocl_run(clprogram):
    """
    Run the OpenCL kernel in ComputeAorta and check for errors

    Arguments:
        clprogram (str): The path to the kernel to run

    Returns:
        (str, str): The stdout and stderr from the execution
        or None if an issue was detected
    """
    result = None
    pl_name = "Codeplay"
    ocl = RunInThread(
        [CL_LAUNCHER, "-f", clprogram, "-p", str(0), "-d", str(DEVICE)
         ] + CL_LAUNCHER_OPTS, TIMEOUT)

    out, err = ocl.stdout, ocl.stderr
    logging.debug("Codeplay:\nOut: %r\nErr: %r\n", out, err)

    compiled = b"compilation terminated successfully" in out.lower()
    vectorized = b"cannot vectorize" not in err.lower()
    vectorized &= b"failed to vectorize" not in err.lower()
    vectorized &= b"could not scalarize" not in err.lower()

    # We will return both the stdout and the stderr, since we need them for
    # various tests
    result = {}
    if err:
        result['err'] = err.split(b"\n")
    if out:
        result['out'] = out.split(b"\n")
    # Sometimes we want to get test cases with specific output.
    # This is particurarly useful when doing reductions, in order to
    # maintain the original reason why this case is interesting
    if STDOUT_MUST_CONTAIN:
        for filtered_word in STDOUT_MUST_CONTAIN:
            if filtered_word not in out:
                logging.info("Test case filtered out")
                return None
    if STDERR_MUST_CONTAIN:
        for filtered_word in STDERR_MUST_CONTAIN:
            if filtered_word not in err:
                logging.info("Test case filtered out")
                return None
    if compiled and not ocl.timed_out:
        if CHECK_IF_VECTORIZED and not vectorized:
            logging.info("Codeplay: Vectorization failed")
            result['vecz_failed'] = True
        return result
    else:
        result = {'out' : "", 'err' : ""}
        if ocl.timed_out:
            logging.info("Codeplay: Timed out")
            result['out'] = pl_name + " timed out!"
            result['timed_out'] = True
            return None
        else:
            logging.info("Codeplay: Compilation failed")
            result['out'] = pl_name + " did not compile!"
            result['compilation_failed'] = True

    return result


def register_fnr_options(argparser):
    """
    Register the options that need to be given from `find_and_reduce.py`

    Arguments:
        argparser: The argparser to add the arguments to
    """
    argparser.add_argument('--vecz', action='store_true')
    argparser.add_argument('--vecz-boscc', action='store_true')
    argparser.add_argument('--cl-launcher-path', action='store')
    argparser.add_argument(
        '--cl-launcher-options',
        action='store',
        help="\" --opt1 --opt2\" (note the space at the beginning)")
    argparser.add_argument('--oclgrind-path', action='store')
    argparser.add_argument('--no-oclgrind', action='store_true')
    argparser.add_argument('--no-references', action='store_true')
    argparser.add_argument('--check-if-vectorized', action='store_true')
    argparser.add_argument('--check-if-vectorized-ignore',
                           action='store',
                           nargs='*',
                           help="Ignore failures caused by the given keywords")
    argparser.add_argument('--stdout-must-contain', action='store', nargs='*')
    argparser.add_argument('--stderr-must-contain', action='store', nargs='*')
    argparser.add_argument('--no-compilation-failed',
                           action='store_true',
                           help="Ignore test cases that did not compile")


def run(**kwargs):
    """
    The entry point for testing if a testcase is interesting or not

    Keyword Arguments:
        cl_launcher_path (str)            : Path to the cl_launcher binary
        cl_launcher_opts (str)            : Options to pass to cl_launcher
        oclgrind_path (str)               : Path to the oclgrind binary
        clprogram (str)                   : Path to the kernel to be tested
        no_oclgrind (bool)                : Do not verity the kernel with Oclgrind
        check_if_vectorized (bool)        : Test if the kernel was vectorized or not
        check_if_vectorized_ignore ([str]): Ignore vectorization failures with
                                            these keywords
        no_references (bool)              : Do not use any reference implementations
        stdout_must_contain ([str])       : The output of the Codeplay run must
                                            contain these keywords or the test
                                            case is not interesting
        stderr_must_contain ([str])       : Similar to 'stdout_must_contain' but
                                            for the error output
        vecz (bool)                       : Enable the vectorizer
        vecz-boscc (bool)                 : Enable the boscc vectorizer opti
        no_compilation_failed (bool)      : Do not count tests that failed to
                                            compile as interesting

    Returns:
        bool: True is the test case is interesting
    """
    global CL_LAUNCHER
    global CL_LAUNCHER_OPTS
    global OCLGRIND
    global VECTORIZATION_FAILED_IGNORE_KEYWORDS
    global REFERENCE_PLATFORMS
    global CODEPLAY_PLATFORM
    global CHECK_IF_VECTORIZED
    global STDOUT_MUST_CONTAIN
    global STDERR_MUST_CONTAIN

    CL_LAUNCHER = kwargs['cl_launcher_path'] or os.path.join(os.getcwd(),
                                                             CL_LAUNCHER)
    CL_LAUNCHER_OPTS = kwargs['cl_launcher_opts'].split(
    ) if 'cl_launcher_opts' in kwargs else CL_LAUNCHER_OPTS
    OCLGRIND = kwargs['oclgrind_path'] or OCLGRIND
    clprogram = kwargs['clprogram']
    no_oclgrind = 'no_oclgrind' in kwargs and kwargs['no_oclgrind'] or False
    CHECK_IF_VECTORIZED = kwargs['check_if_vectorized']
    no_references = kwargs['no_references']
    if kwargs['check_if_vectorized_ignore']:
        VECTORIZATION_FAILED_IGNORE_KEYWORDS = kwargs[
            'check_if_vectorized_ignore']
    STDOUT_MUST_CONTAIN = kwargs['stdout_must_contain'] or STDOUT_MUST_CONTAIN
    STDERR_MUST_CONTAIN = kwargs['stderr_must_contain'] or STDERR_MUST_CONTAIN
    no_compilation_failed = kwargs['no_compilation_failed']

    CODEPLAY_PLATFORM = populate_platforms()

    logging.debug("CL_LAUNCHER: %r", CL_LAUNCHER)
    logging.debug("CL_LAUNCHER_OPTS: %r", CL_LAUNCHER_OPTS)
    logging.debug("OCLGRIND: %r", OCLGRIND)
    logging.debug("OCLGRIND_OPTS: %r", OCLGRIND_OPTS)
    logging.debug("CODEPLAY PLATFORM: %r", CODEPLAY_PLATFORM)
    logging.debug("REFERENCE PLATFORMS: %r", REFERENCE_PLATFORMS)

    if kwargs['vecz']:
        os.environ['CA_EXTRA_COMPILE_OPTS'] = '-cl-wfv=always'
    if kwargs['vecz-boscc']:
        os.environ['CODEPLAY_VECZ_CHOICES'] = 'LinearizeBOSCC'

    reference = None
    # Get the reference run
    if not no_references:
        reference = get_reference_run(clprogram)
        # We couldn't get a reference, this test case is useless
        if not reference:
            return False
    # Get the OCL run
    ocl = get_ocl_run(clprogram)
    if not ocl:
        return False

    # OCL and references match, this test case is not interesting.
    # It should be noted that at this point ocl is always not None while if the
    # references are None then it means that we didn't ask to get any in the
    # first place.
    if ocl['out'] == reference:
        return False

    # Check if OCL managed to vectorize the kernel
    if CHECK_IF_VECTORIZED:
        # We already know some of the cases that don't work, ignore them
        vecz_failed = 'vecz_failed' in ocl and ocl['vecz_failed']
        if not no_compilation_failed:
            vecz_failed |= 'compilation_failed' in ocl and ocl['compilation_failed']
        for keyword in VECTORIZATION_FAILED_IGNORE_KEYWORDS:
            if any([keyword in line for line in ocl['err']]):
                logging.info("Ignoring filtered vectorization failure")
                vecz_failed = False
                break
        if no_references and not vecz_failed:
            return False

    # Verify with oclgrind
    if not no_oclgrind:
        return verify_with_oclgrind(clprogram)

    # We didn't fail any of the checks, so everything must be fine
    return True


def main(argv):
    """
    Main function for running this module as a standalone script

    Returns:
        Exits with 0 if the test case is interesting
    """
    argparser = argparse.ArgumentParser(
        description="Check if the OpenCL kernel is interesting.")
    argparser.add_argument('--loglevel',
                           action='store',
                           choices=['DEBUG', 'INFO', 'WARNING', 'ERROR',
                                    'CRITICAL'],
                           default='INFO')
    argparser.add_argument('--logfile', action='store')
    argparser.add_argument(
        'clprogram',
        metavar="KERNEL",
        help="The kernel file to run (CLProgram.cl by default)")
    register_fnr_options(argparser)
    args = argparser.parse_args(argv[1:])

    log_level = getattr(logging, args.loglevel.upper(), None)
    if log_level == logging.DEBUG:
        fmt = "%(asctime)s - %(levelname)s@[%(funcName)s:%(lineno)s] %(message)s"
    else:
        fmt = "%(asctime)s : %(levelname)s : %(message)s"
    logging.basicConfig(level=log_level, format=fmt, filename=args.logfile)
    logging.debug("%r\n", argv)

    if run(**vars(args)):
        sys.exit(0)
    else:
        sys.exit(1)


if __name__ == "__main__":
    main(sys.argv)
