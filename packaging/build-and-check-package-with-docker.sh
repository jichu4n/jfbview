#!/bin/bash

package_format="$1"
docker_image="$2"
version="$3"
dist="$4"
arch="$5"
platform="$6"

set -ex

platform_flag=()
if [ -n "$platform" ]; then
  platform_flag=(--platform "$platform")
fi

docker pull "${platform_flag[@]}" "$docker_image" > /dev/null

docker run \
  --rm \
  "${platform_flag[@]}" \
  -v "$PWD":/work \
  "$docker_image" \
  "/work/packaging/build-package-${package_format}.sh" \
    "$version" "$dist" "$arch"

docker run \
  --rm \
  "${platform_flag[@]}" \
  -v "$PWD":/work \
  "$docker_image" \
  "/work/packaging/check-package-${package_format}.sh"

