#!/bin/bash

set -ex

export DEBIAN_FRONTEND=noninteractive
apt-get update

cd "$(dirname "$0")/.."

dpkg-deb -I upload/*.deb
dpkg-deb -c upload/*.deb
apt install -y ./upload/*.deb

packaging/post-install-smoke-test.sh

