#!/bin/bash

formatter="/usr/bin/clang-format"

[ ! -x "$formatter" ] && exit 0

echo "Formatting source"

pushd lib
find ./ -type f \( -iname \*.c -o -iname \*.h \) | \
  xargs "$formatter" -i -style="{BasedOnStyle: Google, DerivePointerAlignment: false, PointerAlignment: Left}"
popd

pushd apps
find ./ -type f \( -iname \*.c -o -iname \*.h \) | \
  xargs "$formatter" -i -style="{BasedOnStyle: Google, DerivePointerAlignment: false, PointerAlignment: Left}"
popd

pushd test
find ./ -type f \( -iname \*.c -o -iname \*.h \) | \
  xargs "$formatter" -i -style="{BasedOnStyle: Google, DerivePointerAlignment: false, PointerAlignment: Left}"
popd
