#!/bin/bash

src_dir="$(realpath "$(dirname "$0")/..")"

function budo() {
  sudo -u builduser "$@"
}

function install_build_deps() {
  pacman -Sy --needed --noconfirm \
    base-devel git sudo

  useradd -m builduser
  passwd -d builduser
  echo -e '\nbuilduser ALL=(ALL) NOPASSWD:ALL\n' | tee -a /etc/sudoers

  budo git clone https://aur.archlinux.org/jfbview-git.git /home/builduser/jfbview-git
}

function build_package() {
  cd /home/builduser/jfbview-git
  budo makepkg --syncdeps --noconfirm

  mkdir -p "${src_dir}"/upload
  mv *.pkg.tar.xz "${src_dir}"/upload/
}

function install_test_deps() {
  pacman -Sy --needed --noconfirm \
    gtest
}

function run_tests() {
  cd "$(dirname "$0")/.."
  mkdir -p build_tests
  cmake -H. -Bbuild_tests \
    -DBUILD_TESTING=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_VERBOSE_MAKEFILE=ON
  cmake --build build_tests
  env CTEST_OUTPUT_ON_FAILURE=1 \
    cmake --build build_tests --target test
}

set -ex

install_build_deps
# build_package must run before run_tests as it will install build
# dependencies.
build_package
install_test_deps
run_tests

