#!/bin/bash

version="$1"
dist="$2"
arch="$3"
package_file_prefix="jfbview_${version}~${dist}_${arch}"

function install_build_deps() {
  export DEBIAN_FRONTEND=noninteractive
  apt-get update
  apt-get install -y \
    build-essential cmake file \
    libncurses-dev libncursesw5-dev libimlib2-dev \
    libglu1-mesa-dev libxi-dev libxrandr-dev
}

function build_package() {
  cd "$(dirname "$0")/.."
  mkdir -p build upload
  cmake -H. -Bbuild \
    -DPACKAGE_FORMAT=DEB \
    -DPACKAGE_FILE_PREFIX="$package_file_prefix" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DCMAKE_VERBOSE_MAKEFILE=ON
  cmake --build build --target package
  mv build/*.deb upload/
}

function install_test_deps() {
  export DEBIAN_FRONTEND=noninteractive
  apt-get install -y \
    libgtest-dev

  # In earlier Debian-based releases (Debian Stretch or older, Ubuntu Xenial or
  # older), libgtest-dev doesn't actually ship the gtest libraries in compiled
  # form. Instead, we have to compile them ourselves. The following is derived
  # from https://stackoverflow.com/a/44649433/3401268.
  if dpkg -L libgtest-dev | grep -q 'libgtest\.a$'; then
    gtest_root_flag=()
  else
    cd "$(dirname "$0")/.."
    mkdir -p vendor/gtest
    ( \
      cd vendor/gtest && \
      cmake /usr/src/gtest && \
      cmake --build . \
    )
    gtest_root_flag=("-DGTEST_ROOT=${PWD}/vendor/gtest")
  fi
}

function run_tests() {
  cd "$(dirname "$0")/.."
  mkdir -p build_tests
  cmake -H. -Bbuild_tests \
    -DBUILD_TESTING=ON \
    "${gtest_root_flag[@]}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_VERBOSE_MAKEFILE=ON
  cmake --build build_tests
  env CTEST_OUTPUT_ON_FAILURE=1 \
    cmake --build build_tests --target test
}

set -ex

install_build_deps
install_test_deps
run_tests
build_package

