#!/bin/bash

package_file_prefix="${1:-jfbview}"

set -ex

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y \
  build-essential cmake file \
  libncurses5-dev libimlib2-dev \
  libglu1-mesa-dev libxi-dev libxrandr-dev

cd "$(dirname "$0")/.."
mkdir -p build upload
cmake -H. -Bbuild \
  -DPACKAGE_FILE_PREFIX="$package_file_prefix" \
  -DCMAKE_VERBOSE_MAKEFILE=ON
cmake --build build --target package
mv build/*.deb upload/

