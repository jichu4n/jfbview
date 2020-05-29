#!/bin/bash

# Script to build a source archive including all transitive dependencies needed
# to build jfbview.

version="$1"

if ! hash git-archive-all > /dev/null 2>&1; then
  echo 'git-archive-all not found on $PATH. Please install with'
  echo
  echo '    pip install git-archive-all'
  echo
  exit 1
fi

set -ev

cd "$(dirname "$0")/.."
mkdir -p upload
git-archive-all ./upload/jfbview-${version}-full-source.zip

