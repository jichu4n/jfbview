#!/bin/bash

set -ex

yum install -y -q epel-release
yum install -y -q which  # Needed by smoke-test.sh

cd "$(dirname "$0")/.."

rpm -qpil upload/*.rpm
yum install -y ./upload/*.rpm

tests/smoke-test.sh

