# malloc-similar-macos-implementation

My implementation of the malloc function, which only works in single-threaded mode.

I use 3 types of zones, two of which are pre-allocated for small objects less than 128 bytes and for medium objects less than 512 bytes.

for more efficient use of memory, I use such a thing as a freed list.
