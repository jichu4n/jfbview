#!/bin/bash

src_dir="$(realpath "$(dirname "$0")/..")"

function budo() {
  sudo -u builduser "$@"
}

function install_build_deps() {
  pacman -Syq --needed --noconfirm \
    base-devel git sudo rsync \
    > /dev/null  # -q doesn't actually silence pacman -Sy.

  useradd -m builduser
  passwd -d builduser
  echo -e '\nbuilduser ALL=(ALL) NOPASSWD:ALL\n' | tee -a /etc/sudoers

  budo git clone https://aur.archlinux.org/jfbview-git.git /home/builduser/jfbview-git
  cd /home/builduser/jfbview-git
  budo mkdir -p src
  budo rsync -a "${src_dir}"/ src/jfbview/
}

function build_package() {
  cd /home/builduser/jfbview-git

  budo makepkg --syncdeps --noconfirm --noextract

  mkdir -p "${src_dir}"/upload
  mv *.pkg.tar.xz "${src_dir}"/upload/

  # For caching in Travis CI.
  rsync -a src/jfbview/vendor/mupdf/build/ "${src_dir}"/vendor/mupdf/build/
}

function install_test_deps() {
  pacman -Syq --needed --noconfirm \
    gtest which \
    > /dev/null  # -q doesn't actually silence pacman -Sy.
}

function run_tests() {
  cd /home/builduser/jfbview-git/src/jfbview

  budo cmake -H. -Bbuild_tests \
    -DBUILD_TESTING=ON \
    -DCMAKE_BUILD_TYPE=Debug
  budo cmake --build build_tests
  budo env CTEST_OUTPUT_ON_FAILURE=1 \
    cmake --build build_tests --target test
}

set -ex

install_build_deps
# build_package must run before run_tests as it will install build
# dependencies.
build_package
install_test_deps
run_tests

