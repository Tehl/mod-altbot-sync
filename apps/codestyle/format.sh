#!/usr/bin/env bash

clang-format -i $(find . -name "*.h" -o -name "*.cpp" -o -name "*.c" -o -name "*.hpp")
