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

archs = [
    ('32bit_x87',   ['-DARCH_FLAGS=-m32 -mno-sse -mno-sse2 -static']),
    ('32bit_SSE2',  ['-DARCH_FLAGS=-m32 -msse2 -static']),
    ('32bit_SSE41', ['-DARCH_FLAGS=-m32 -msse4.1 -static']),
    ('32bit_AVX',   ['-DARCH_FLAGS=-m32 -mavx -static']),
    ('32bit_AVX2',  ['-DARCH_FLAGS=-m32 -mavx2 -static']),
    ('64bit_SSE2',  ['-DARCH_FLAGS=-m64 -msse2 -static']),
    ('64bit_SSE41', ['-DARCH_FLAGS=-m64 -msse4.1 -static']),
    ('64bit_AVX',   ['-DARCH_FLAGS=-m64 -mavx -static']),
    ('64bit_AVX2',  ['-DARCH_FLAGS=-m64 -mavx2 -static'])
    ]

path = os.path.dirname(os.path.realpath(__file__))

build_dir = os.path.join(path, 'build')
    

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
    
for arch, flags in archs:
    print(arch, ' ', flags,'...')
    
    arch_build_dir = os.path.join(build_dir, arch)

    try:
        os.makedirs(arch_build_dir)
    except:
        pass

    if subprocess.call(['cmake', '-G', generator, '../..'] + flags + options, cwd=arch_build_dir): raise Exception('Can\'t make project')
    if subprocess.call(['cmake', '--build', '.', '--', '-j12'], cwd=arch_build_dir): raise Exception('Can\'t build project')
    if subprocess.call(['ctest'], cwd=os.path.join(arch_build_dir, 'tests')): raise Exception('Can\'t test project')
