#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Cleaning build directories and artifacts..."
rm -rf build build_tests vendor/mupdf/build upload tests/qemu/out

