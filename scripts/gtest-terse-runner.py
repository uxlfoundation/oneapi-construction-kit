#!/usr/bin/env python

# Copyright (C) Codeplay Software Limited. All Rights Reserved.

from subprocess import PIPE, STDOUT, Popen
from sys import argv, stderr

from colorama import Fore


def run_gtest():
    process = Popen(argv[1:], stdout=PIPE, stderr=STDOUT)
    lines = process.communicate()[0].decode('utf-8').splitlines()
    results = {}

    current_name = None
    sanitizer_line = 0
    for num, line in enumerate(lines):
        if '[ RUN      ]' in line:
            current_name = line[line.find(']') + 2:]
            results[current_name] = {'line': num, 'result': 'CRASH'}
        elif '=================================================================' == line:
            sanitizer_line = num
        else:
            passed = '[       OK ]' in line
            skipped = '[  SKIPPED ]' in line
            failed = '[  FAILED  ]' in line

            if not (passed or skipped or failed):
                continue

            if 'listed below' in line:
                break

            name, time = line[line.find(']') + 2:].split(' ', maxsplit=1)
            if name[-1] == ',':
                name = name[:-1]
                time = time[time.rfind('('):]
            results[name]['time'] = time
            results[name]['output'] = '\n'.join(lines[results[name]['line'] +
                                                      1:num]).strip()
            if passed:
                results[name]['result'] = 'PASSED'
            elif skipped:
                results[name]['result'] = 'SKIPPED'
            elif failed:
                results[name]['result'] = 'FAILED'

    sanitizer = None
    if process.returncode != 0:
        if 'output' not in results[current_name]:
            results[current_name]['output'] = '\n'.join(
                lines[results[current_name]['line'] + 1:-1]).strip()
        else:
            sanitizer = '\n'.join(lines[sanitizer_line:-1])

    return results, sanitizer, process.returncode


def main():
    if len(argv) == 1:
        print(f'usage: {argv[0]} <gtest-executable> [<arg>...]')
        exit(0)
    results, sanitizer, returncode = run_gtest()

    passed = []
    skipped = []
    failed = []
    for name in sorted(results.keys()):
        test = results[name]
        result = '%s %s' % ({
            'PASSED': f'{Fore.GREEN}[  PASSED  ]{Fore.RESET}',
            'SKIPPED': f'{Fore.YELLOW}[  SKIPPED ]{Fore.RESET}',
            'FAILED': f'{Fore.RED}[  FAILED  ]{Fore.RESET}',
            'CRASH': f'{Fore.RED}[  CRASH   ]{Fore.RESET}',
        }[test['result']], name)
        if 'time' in test:
            result = '%s %s\n' % (result,
                                  f'{Fore.BLUE}{test["time"]}{Fore.RESET}')
        if test['result'] == 'PASSED':
            passed.append((result + test['output']).rstrip())
        elif test['result'] == 'SKIPPED':
            skipped.append((result + test['output']).rstrip())
        elif test['result'] == 'FAILED':
            failed.append((result + test['output']).rstrip())
        elif test['result'] == 'CRASH':
            try:
                signal = {
                    -1: 'SIGHUP',
                    -2: 'SIGINT',
                    -3: 'SIGQUIT',
                    -4: 'SIGILL',
                    -5: 'SIGTRAP',
                    -6: 'SIGABRT',
                    -6: 'SIGIOT',
                    -7: 'SIGBUS',
                    -8: 'SIGFPE',
                    -9: 'SIGKILL',
                    -10: 'SIGUSR1',
                    -11: 'SIGSEGV',
                    -12: 'SIGUSR2',
                    -13: 'SIGPIPE',
                    -14: 'SIGALRM',
                    -15: 'SIGTERM',
                }[returncode]
            except KeyError:
                signal = f'EXITED {returncode}'
            failed.append((
                f'{result}\n{test["output"]}\n{Fore.RED}{signal}{Fore.RESET}\n'
            ).rstrip())

    for test in failed:
        print(test, file=stderr)

    rates = []
    if len(passed):
        rates.append(f'{Fore.GREEN}Passed{Fore.RESET} {len(passed)}')
    if len(skipped):
        rates.append(f'{Fore.YELLOW}Skipped{Fore.RESET} {len(skipped)}')
    if len(failed):
        rates.append(f'{Fore.RED}Failed{Fore.RESET} {len(failed)}')
    print(f'{", ".join(rates)} of {len(results)} tests.')
    if sanitizer:
        print(sanitizer, file=stderr)

    exit(returncode)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        exit(130)
