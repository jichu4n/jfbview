#!/bin/bash

version="$1"
dist="$2"
arch="$3"
package_file_prefix="jfbview_${version}~${dist}_${arch}"

set -ex

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y \
  build-essential cmake file \
  libncurses-dev libimlib2-dev \
  libglu1-mesa-dev libxi-dev libxrandr-dev

cd "$(dirname "$0")/.."
mkdir -p build upload
cmake -H. -Bbuild \
  -DPACKAGE_FORMAT=DEB \
  -DPACKAGE_FILE_PREFIX="$package_file_prefix" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_VERBOSE_MAKEFILE=ON
cmake --build build --target package
mv build/*.deb upload/

