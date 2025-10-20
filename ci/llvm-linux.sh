#!/bin/bash
set -e

ver=21

wget -O - https://apt.llvm.org/llvm.sh | sudo bash -s - $ver
sudo apt install -y llvm-$ver

for tool in clang clang++ llvm-ar ld.lld llvm-nm llvm-objcopy llvm-objdump llvm-ranlib; do
  sudo update-alternatives --install /usr/bin/$tool $tool /usr/lib/llvm-$ver/bin/$tool 180
  sudo update-alternatives --set $tool /usr/lib/llvm-$ver/bin/$tool
done
