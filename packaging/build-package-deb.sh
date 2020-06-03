#!/bin/bash

version="$1"
dist="$2"
arch="$3"
package_file_prefix="jfbview_${version}~${dist}_${arch}"

if [ "$EUID" -eq 0 ]; then
  sudo=
else
  sudo='sudo'
fi

if grep -qi ubuntu /etc/os-release; then
  libjpeg=libjpeg8
  libjpeg_dev=libjpeg8-dev
else
  libjpeg=libjpeg62-turbo
  libjpeg_dev=libjpeg62-turbo-dev
fi

function install_build_deps() {
  export DEBIAN_FRONTEND=noninteractive
  $sudo apt-get -qq update
  $sudo apt-get -qq install -y \
    build-essential cmake file pkg-config \
    libncurses-dev libncursesw5-dev ${libjpeg_dev} \
    libharfbuzz-dev libfreetype6-dev zlib1g-dev\
    > /dev/null  # -qq doesn't actually silence apt-get install.
}

function build_package() {
  cd "$(dirname "$0")/.."
  mkdir -p build upload
  cmake -H. -Bbuild \
    -DPACKAGE_FORMAT=DEB \
    -DLIBJPEG_PACKAGE_NAME=${libjpeg} \
    -DPACKAGE_FILE_PREFIX="$package_file_prefix" \
    -DBUILD_TESTING=OFF \
    -DCMAKE_BUILD_TYPE=Release
  cmake --build build --target package
  mv build/*.deb upload/
}

function install_test_deps() {
  export DEBIAN_FRONTEND=noninteractive
  $sudo apt-get -qq install -y \
    libgtest-dev \
    > /dev/null  # -qq doesn't actually silence apt-get install.

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
    "${gtest_root_flag[@]}" \
    -DBUILD_TESTING=ON \
    -DCMAKE_BUILD_TYPE=Debug
  cmake --build build_tests
  env CTEST_OUTPUT_ON_FAILURE=1 \
    cmake --build build_tests --target test
}

set -ex

install_build_deps
install_test_deps
run_tests
build_package

