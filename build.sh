#!/bin/bash

git submodule init
git submodule update
rm -rf build
rm -rf executable
mkdir -p build

for var in "$@"; do
  CMAKE_ARGS="${CMAKE_ARGS} -D $var=true"
done

cmake -S . -B build $CMAKE_ARGS
make -C build -j 8

# args is SAFE_FREE and\or THREAD_SAFE
# install_name_tool -add_rpath ./ ./a.out <- through static link
# DYLD_LIBRARY_PATH=$PWD DYLD_INSERT_LIBRARIES=libft_malloc_x86_64_Darwin.so DYLD_FORCE_FLAT_NAMESPACE=1 ./a.out <- through dynamic link
