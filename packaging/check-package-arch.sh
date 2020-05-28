#!/bin/bash

set -ex

pacman -Syq \
  > /dev/null  # -q doesn't actually silence pacman -Sy.
# Needed by smoke-test.sh
pacman -Sq --noconfirm which \
  > /dev/null  # -q doesn't actually silence pacman -S.

cd "$(dirname "$0")/.."

tar tvf upload/*.pkg.tar.xz
pacman -U --noconfirm upload/*.pkg.tar.xz

tests/smoke-test.sh

