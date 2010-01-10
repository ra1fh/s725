#!/bin/sh

set -e

# libtoolize --copy --force
AUTOMAKE_VERSION=1.9 AUTOCONF_VERSION=2.62 aclocal
AUTOMAKE_VERSION=1.9 AUTOCONF_VERSION=2.62 autoheader
AUTOMAKE_VERSION=1.9 AUTOCONF_VERSION=2.62 automake --foreign --add-missing --copy
AUTOMAKE_VERSION=1.9 AUTOCONF_VERSION=2.62 autoconf

