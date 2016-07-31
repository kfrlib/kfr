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
    
options = [
    '-DCMAKE_BUILD_TYPE=Release',
    ]
    
CMAKE_C_COMPILER = ''
CMAKE_CXX_COMPILER = ''
    
for v in ('clang-3.9', 'clang-3.8', 'clang-3.7', 'clang'):
    print('Checking ', v, '...', end=' ')
    try:
        if subprocess.call([v, '--version'], stdout=subprocess.PIPE) == 0:
            CMAKE_C_COMPILER = v
            break
    except:
        pass
        
if not CMAKE_C_COMPILER:
    raise Exception('clang is not on your PATH')
print('ok')
        
for v in ('clang++-3.9', 'clang++-3.8', 'clang++-3.7', 'clang++'):
    print('Checking ', v, '...', end=' ')
    try:
        if subprocess.call([v, '--version'], stdout=subprocess.PIPE) == 0:
            CMAKE_CXX_COMPILER = v
            break
    except:
        pass
        
if not CMAKE_CXX_COMPILER:
    raise Exception('clang++ is not on your PATH')
    
print('ok')
        
options.append('-DCMAKE_C_COMPILER='+CMAKE_C_COMPILER)
options.append('-DCMAKE_CXX_COMPILER='+CMAKE_CXX_COMPILER)

if sys.platform.startswith('win32'):
    generator = 'MinGW Makefiles'    
else:
    generator = 'Unix Makefiles'


if subprocess.call(['cmake', '-G', generator, '..'] + options, cwd=build_dir): raise Exception('Can\'t make project')
if subprocess.call(['cmake', '--build', '.', '--', '-j4'], cwd=build_dir): raise Exception('Can\'t build project')
if subprocess.call(['ctest'], cwd=os.path.join(build_dir, 'tests')): raise Exception('Can\'t test project')
