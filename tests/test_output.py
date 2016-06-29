#!/usr/bin/env python
from __future__ import print_function

import os
import subprocess
import sys
import re

binary_filename = sys.argv[1]
source_filename = sys.argv[2]

with open(source_filename) as src:
    test_source = enumerate(src.readlines())
    
parsed_output = [(re.sub(r'^\s*// >>>', '', line).strip(), linenum) for linenum, line in test_source if '// >>>' in line]

output = subprocess.check_output([binary_filename], stderr=subprocess.STDOUT).decode("utf-8").splitlines()

output = [o.strip() for o in output]

fails = 0
for expected, actual in zip(parsed_output, output):
    reg = re.escape(expected[0]).replace(r'\.\.\.', '.*')
    match = re.match(reg, actual)
    if not match:
        fails+=1
        print('Expected output string ({file}.cpp, #{line}): \n"{expected}"\n got: \n"{actual}"'.format(expected=expected[0], file=filename, actual=actual, line=expected[1]))

if fails == 0:
    print('All tests passed successfully ({} lines)'.format(len(parsed_output)))
else:
    print('Number of failed tests: {fails})'.format(fails=fails))
    
exit(fails)
