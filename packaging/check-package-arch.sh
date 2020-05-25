#!/bin/bash

set -ex

pacman -Sy
pacman -S --noconfirm which  # Needed by post-install-smoke-test.sh

cd "$(dirname "$0")/.."

tar tvf upload/*.pkg.tar.xz
pacman -U --noconfirm upload/*.pkg.tar.xz

packaging/post-install-smoke-test.sh

