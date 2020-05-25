#!/bin/bash

package_format="$1"
docker_image="$2"
version="$3"
dist="$4"
arch="$5"

set -ex

docker run \
  --rm \
  -v "$PWD":/work \
  "$docker_image" \
  "/work/packaging/build-package-${package_format}.sh" \
    "$version" "$dist" "$arch"

docker run \
  --rm \
  -v "$PWD":/work \
  "$docker_image" \
  "/work/packaging/check-package-${package_format}.sh"

