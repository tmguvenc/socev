#!/bin/bash

find lib test apps -name '*.[ch]' -exec clang-format -i {} -style="{BasedOnStyle: Google, DerivePointerAlignment: false, PointerAlignment: Left}" \;
