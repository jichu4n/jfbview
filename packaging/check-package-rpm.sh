#!/bin/bash

if [ "$EUID" -eq 0 ]; then
  sudo=
else
  sudo='sudo'
fi

set -ex

$sudo dnf install -y -q which  # Needed by smoke-test.sh

cd "$(dirname "$0")/.."

rpm -qpil upload/*.rpm
$sudo dnf install -y ./upload/*.rpm

tests/smoke-test.sh

