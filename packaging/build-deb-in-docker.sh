#!/bin/bash

docker_image="$1"
package_file_prefix="$2"

set -ex

docker run \
  --rm \
  -v "$PWD":/work \
  "$docker_image" \
  /work/packaging/build-deb.sh "$package_file_prefix"

