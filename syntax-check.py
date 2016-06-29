#!/usr/bin/env python
from __future__ import print_function

import fnmatch
import subprocess
import os
import sys

path = os.path.dirname(os.path.realpath(__file__))

filenames = []
for root, dirnames, files in os.walk(os.path.join(path, 'include')):
    for filename in fnmatch.filter(files, '*.hpp'):
        filenames.append(os.path.join(root, filename))
        

target = ""
if sys.platform.startswith('win32'):
	target = "--target=x86_64-w64-windows-gnu"
    
fails = 0
for filename in filenames:
    print(filename, '...')
    c = subprocess.call(["clang", "-fsyntax-only", filename, "-std=c++14", "-I"+os.path.join(path, "include"), "-Wno-pragma-once-outside-header", target])
    if c != 0:
        fails+=1
        
exit(fails)
