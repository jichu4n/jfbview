#!/bin/bash

version="$1"
dist="$2"
arch="$3"
package_file_prefix="jfbview-${version}-${dist}.${arch}"

if [ "$EUID" -eq 0 ]; then
  sudo=
else
  sudo='sudo'
fi

function install_build_deps() {
  $sudo dnf install -y -q \
    cmake make gcc-c++ rpm-build \
    ncurses-devel libjpeg-turbo-devel \
    harfbuzz-devel freetype-devel zlib-devel \
    openjpeg2-devel
}

function build_package() {
  cd "$(dirname "$0")/.."
  mkdir -p build upload
  cmake -H. -Bbuild \
    -DPACKAGE_FORMAT=RPM \
    -DLIBJPEG_PACKAGE_NAME=libjpeg-turbo \
    -DPACKAGE_FILE_PREFIX="$package_file_prefix" \
    -DBUILD_TESTING=OFF \
    -DCMAKE_BUILD_TYPE=Release
  cmake --build build --target package -- -j$(nproc)
  mv build/*.rpm upload/
}

function install_test_deps() {
  $sudo dnf install -y -q gtest-devel which
}

function run_tests() {
  cd "$(dirname "$0")/.."
  mkdir -p build_tests
  cmake -H. -Bbuild_tests \
    -DBUILD_TESTING=ON \
    -DCMAKE_BUILD_TYPE=Debug
  cmake --build build_tests -- -j$(nproc)
  env CTEST_OUTPUT_ON_FAILURE=1 \
    cmake --build build_tests --target test
}

set -ex

install_build_deps
install_test_deps
run_tests
build_package

