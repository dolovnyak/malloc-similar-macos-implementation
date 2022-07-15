#!/bin/bash

git submodule update
rm -rf build
rm -rf executable
mkdir -p build
if [[ "$1" == "SAFE_FREE" ]]; then
cmake -S . -B build -D SAFE_FREE=true
else
cmake -S . -B build
fi
make -C build

# install_name_tool -add_rpath ./ ./a.out <- through static link
# DYLD_LIBRARY_PATH=$PWD DYLD_INSERT_LIBRARIES=libft_malloc_x86_64_Darwin.so DYLD_FORCE_FLAT_NAMESPACE=1 ./a.out <- through dynamic link
