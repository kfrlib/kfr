#!/usr/bin/env python
from __future__ import print_function

import fnmatch
import os
import subprocess
import sys
import glob

def list_sources(name, searchpath, masks, exclude = []):
    global cmake
    path = os.path.join(os.path.dirname(os.path.realpath(__file__)), searchpath)
    filenames = []
    for root, dirnames, files in os.walk(path, path):
        for mask in masks:
            for filename in fnmatch.filter(files, mask):
                if filename not in exclude:
                    filenames.append(os.path.relpath(os.path.join(root, filename), path).replace('\\','/'))

    cmake += """
set(
    """ + name + """
    """ + "\n    ".join(['${PROJECT_SOURCE_DIR}/' + searchpath + '/' + f for f in filenames]) + """
)

    """

cmake = """
# Auto-generated file. Do not edit
# Use update-sources.py
"""

list_sources("KFR_SRC", "include", ['*.hpp', '*.h', '*.i', '*.inc'])
list_sources("KFR_DFT_SRC", "include/kfr/dft", ['*.cpp'], ["dft-src.cpp"])
list_sources("KFR_IO_SRC", "include/kfr/io", ['*.cpp'])

list_sources("KFR_UNITTEST_SRC", "tests/unit", ['*.cpp'])

with open(os.path.join(os.path.dirname(os.path.realpath(__file__)), 'sources.cmake'), "w") as f:
    f.write(cmake)
