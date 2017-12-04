#!/bin/bash

cd "$(dirname "$0")"/..

version_file=src/rvt_lib/version.hpp

get_version () { sed -E '/#d/!d;s/.*"([^"]+)".*/\1/' "$version_file"; }
set_version () { echo -e "#pragma once\n#define RVT_LIB_VERSION \"$1\"" > "$version_file"; }

PACKAGER_DIR=./modules/packager
PACKAGER_PATH=$PACKAGER_DIR/packager.py
source $PACKAGER_DIR/tagger.sh
