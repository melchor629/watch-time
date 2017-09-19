#!/usr/bin/env bash

os=$(uname)
if [[ "$os" == "Darwin" ]]; then
    clang time.mm -framework Cocoa -framework Foundation -Os -o time -mmacosx-version-min=10.6 -fobjc-arc -lc++
elif [[ "$os" == "Linux" ]]; then
    g++ --std=c++11 `pkg-config --cflags gtk+-3.0`-Os -o time time-gtk3.cpp `pkg-config --libs gtk+-3.0`
else
    echo "Unsupported OS: $os"
fi