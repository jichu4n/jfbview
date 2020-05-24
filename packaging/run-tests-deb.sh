#!/bin/bash

set -ex

# This script assumes it's running AFTER packaging/build-deb.sh and that the
# regular build dependencies are already installed.
export DEBIAN_FRONTEND=noninteractive
apt-get install -y \
  libgtest-dev

cd "$(dirname "$0")/.."

# In earlier Debian-based releases (Debian Stretch or older, Ubuntu Xenial or
# older), libgtest-dev doesn't actually ship the gtest libraries in compiled
# form. Instead, we have to compile them ourselves. The following is derived
# from https://stackoverflow.com/a/44649433/3401268.
if dpkg -L libgtest-dev | grep -q 'libgtest\.a$'; then
  gtest_root_flag=()
else
  mkdir -p vendor/gtest
  ( \
    cd vendor/gtest && \
    cmake /usr/src/gtest && \
    cmake --build . \
  )
  gtest_root_flag=("-DGTEST_ROOT=${PWD}/vendor/gtest")
fi

mkdir -p build_tests
cmake -H. -Bbuild_tests \
  -DBUILD_TESTING=ON \
  "${gtest_root_flag[@]}" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_VERBOSE_MAKEFILE=ON
cmake --build build_tests
env CTEST_OUTPUT_ON_FAILURE=1 \
  cmake --build build_tests --target test

