#!/usr/bin/env python

# Copyright (C) 2016 D Levin (http://www.kfrlib.com)
# This file is part of KFR
# 
# KFR is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# KFR is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with KFR.


from __future__ import print_function

import os
import subprocess
import sys

path = os.path.dirname(os.path.realpath(__file__))
build_dir = os.path.join(path, 'build')

try:
    os.makedirs(build_dir)
except:
    pass

print('Checking clang...', end=' ')
if subprocess.call(['clang', '--version'], stdout=subprocess.PIPE): 
    raise Exception('clang is not on your PATH')
print('ok')
print('Checking clang++...', end=' ')
if subprocess.call(['clang++', '--version'], stdout=subprocess.PIPE): 
    raise Exception('clang++ is not on your PATH')
print('ok')

if sys.platform.startswith('win32'):
    generator = 'MinGW Makefiles'    
else:
    generator = 'Unix Makefiles'

options = [
    '-DCMAKE_BUILD_TYPE=Release',
    ]

if subprocess.call(['cmake', '-G', generator, '..'] + options, cwd=build_dir): raise Exception('Can\'t make project')
if subprocess.call(['cmake', '--build', '.'], cwd=build_dir): raise Exception('Can\'t build project')
if subprocess.call(['ctest'], cwd=os.path.join(build_dir, 'tests')): raise Exception('Can\'t test project')
