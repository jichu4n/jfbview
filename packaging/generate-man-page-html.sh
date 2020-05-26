#!/bin/bash

set -ev

cd "$(dirname "$0")/.."
nroff -man doc/jfbview.1 | \
  man2html -nohead -nodepage > \
    doc/jfbview.1.html

