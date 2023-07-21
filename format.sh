#!/bin/bash

find lib apps -name '*.[ch]' -exec clang-format -i {} -style="{BasedOnStyle: Google, DerivePointerAlignment: false, PointerAlignment: Left}" \;
find lib test -name '*.cpp' -exec clang-format -i {} -style="{BasedOnStyle: Google, DerivePointerAlignment: false, PointerAlignment: Left}" \;
