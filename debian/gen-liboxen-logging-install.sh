#!/bin/bash

set -e

# Keep just the major/minor version for the library package version:
LIB_VER="$(echo "$1" | sed -e 's/^\([0-9]\+\.[0-9]\+\).*/\1/')"
if ! grep -q "^Package: liboxen-logging$LIB_VER\$" debian/control; then
    echo -e "\nError: debian/control doesn't contain the correct liboxen-logging$LIB_VER version; you should run:\n\n    ./debian/update-lib-version.sh\n"
    exit 1
fi

if ! [ -f debian/liboxen-logging$LIB_VER ]; then
    rm -f debian/liboxen-logging[0-9]*.install
    sed -e "s/@LIB_VER@/$LIB_VER/" debian/liboxen-logging.install.in >debian/liboxen-logging$LIB_VER.install
fi
