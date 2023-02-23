#!/usr/bin/env python
"""Execute commands in parallel."""

from __future__ import print_function

from argparse import ArgumentParser, RawTextHelpFormatter
from subprocess import PIPE, STDOUT, Popen
from threading import Lock, Thread

lock = Lock()


def call(command):
    """Call a command and capture all output and print on completion."""
    process = Popen(command, shell=True, stdout=PIPE, stderr=STDOUT)
    lock.acquire()
    print('starting: {}'.format(command))
    lock.release()
    out, _ = process.communicate()
    lock.acquire()
    print('$ {}\n'.format(command), end=out.decode())
    lock.release()


def main():
    """Command line entry point."""
    cli = ArgumentParser(formatter_class=RawTextHelpFormatter,
                         epilog='''\
Example usage on a POSIX system, which includes setting environment variables:

$ scripts/parallel.py \\
'MYENVVAR=option1 env $MYENVVAR|grep MYENVVAR' \\
'MYENVVAR=option2 env $MYENVVAR|grep MYENVVAR' \\
'MYENVVAR=option3 env $MYENVVAR|grep MYENVVAR'
starting: MYENVVAR=option2 env $MYENVVAR|grep MYENVVAR
starting: MYENVVAR=option1 env $MYENVVAR|grep MYENVVAR
starting: MYENVVAR=option3 env $MYENVVAR|grep MYENVVAR
$ MYENVVAR=option2 env $MYENVVAR|grep MYENVVAR
MYENVVAR=option2
$ MYENVVAR=option3 env $MYENVVAR|grep MYENVVAR
MYENVVAR=option3
$ MYENVVAR=option1 env $MYENVVAR|grep MYENVVAR
MYENVVAR=option1
''')
    cli.add_argument('command', nargs='+', help='command to execute')
    args = cli.parse_args()
    processes = [
        Thread(target=call, args=[command]) for command in args.command
    ]
    for process in processes:
        process.start()
    while processes:
        for process in processes:
            if not process.is_alive():
                process.join()
                processes.remove(process)
                break


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        exit(130)
