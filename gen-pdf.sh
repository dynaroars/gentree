#!/bin/bash

set -xeuo pipefail

DEST=$1
mkdir -p $DEST

gen() {
    pandoc ../artifact-eval/$1.md -sf gfm -o $DEST/$1.pdf -V geometry:margin=1.5in
}

gen INSTALL
gen README
gen REQUIREMENTS
gen STATUS

cp LICENSE $DEST/LICENSE
