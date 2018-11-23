#!/bin/bash
set -e

build_dir=$1
shift
echo Preparing directories...
rm -r $build_dir || true
mkdir $build_dir
pushd $build_dir
echo Running cmake -GNinja -DENABLE_TESTS=ON "$@" ..
cmake -GNinja -DENABLE_TESTS=ON "$@" ..
echo Running ninja...
ninja
echo Running tests...
cd tests && ctest -V
popd
