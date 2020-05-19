#!/bin/bash

version="$1"
dist="$2"
arch="$3"
package_file_prefix="jfbview-${version}-${dist}.${arch}"

set -ex

yum install -y epel-release
yum install -y \
  cmake make gcc-c++ rpm-build \
  ncurses-devel imlib2-devel \
  mesa-libGLU-devel libXi-devel libXrandr-devel

cd "$(dirname "$0")/.."
mkdir -p build upload
cmake -H. -Bbuild \
  -DPACKAGE_FORMAT=RPM \
  -DPACKAGE_FILE_PREFIX="$package_file_prefix" \
  -DCMAKE_VERBOSE_MAKEFILE=ON
cmake --build build --target package
mv build/*.rpm upload/

