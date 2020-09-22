#!/bin/bash

if [ "$EUID" -eq 0 ]; then
  sudo=
else
  sudo='sudo'
fi

set -ex

$sudo pacman -Syq \
  > /dev/null  # -q doesn't actually silence pacman -Sy.
# Needed by smoke-test.sh
$sudo pacman -Sq --noconfirm which \
  > /dev/null  # -q doesn't actually silence pacman -S.

cd "$(dirname "$0")/.."

tar tvf upload/*.pkg.tar.*
$sudo pacman -U --noconfirm upload/*.pkg.tar.*

tests/smoke-test.sh

