#!/bin/bash

if [ "$EUID" -eq 0 ]; then
  sudo=
else
  sudo='sudo'
fi

set -ex

export DEBIAN_FRONTEND=noninteractive
$sudo apt-get -qq update

cd "$(dirname "$0")/.."

dpkg-deb -I upload/*.deb
dpkg-deb -c upload/*.deb
$sudo apt install -y ./upload/*.deb

tests/smoke-test.sh

