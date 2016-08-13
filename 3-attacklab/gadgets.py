#!/usr/bin/env python
# encoding: utf-8
"""
Created by misaka-10032 (longqic@andrew.cmu.edu).
All rights reserved.

Get gadgets from asm
"""
__author__ = 'misaka-10032'

import argparse
import re
import subprocess


def get_bytes(line):
    try:
        return re.search('.*:\t(([0-9a-f]{2,} )*).*', line).group(1)
    except AttributeError, e:
        return ''

def main(args):
    f = open(args.asm, 'rb')

    bytes = ''
    for line in f:
        bytes += get_bytes(line)

    segs = filter(lambda s: len(s) > 0, bytes.split('c3 '))
    out = ''
    for seg in segs:
        bytes = seg.split(' ')[:-1]
        gs = [bytes[-i:] for i in xrange(1, len(bytes)+1)]
        for g in gs:
            bytes = ' '.join(g) + ' c3'

            prog = ("echo '%s' | udcli -64 -x -att" % bytes)
            popen = subprocess.Popen(prog, shell=True, stdout=subprocess.PIPE)

            result = popen.stdout.read()
            if 'invalid' not in result:
                out += result
                out += '\n'

    print out

    f.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('asm', help='Input asm')
    main(parser.parse_args())
