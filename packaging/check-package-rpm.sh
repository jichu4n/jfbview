#!/bin/bash

if [ "$EUID" -eq 0 ]; then
  sudo=
else
  sudo='sudo'
fi

set -ex

$sudo yum install -y -q epel-release
$sudo yum install -y -q which  # Needed by smoke-test.sh

cd "$(dirname "$0")/.."

rpm -qpil upload/*.rpm
$sudo yum install -y ./upload/*.rpm

tests/smoke-test.sh

