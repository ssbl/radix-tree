radix-tree
==========

This repository houses an implementation of the radix tree data structure, a
memory-efficient and cache-friendly alternative to tries (prefix trees). The
main reason for writing this was to create a replacement for the trie
implementation used by [libzmq](https://github.com/zeromq/libzmq).

[![Build Status](https://travis-ci.org/ssbl/radix-tree.svg?branch=master)](https://travis-ci.org/ssbl/radix-tree)

## Features

- Reduced memory footprint using a packed data layout for tree nodes
- Faster lookups due to cache-friendly design
- Relatively well-tested

## Caveats

- Lacks the simplicity of regular trie implementations
- Performance claims above don't have any supporting benchmarks
- API is harder to use since strings are treated as pointer-length tuples

## References

- The libzmq issue which spawned this idea:
  https://github.com/zeromq/libzmq/issues/1400.
- [wikipedia article](https://en.wikipedia.org/wiki/Radix_tree) and
  [rax](https://github.com/antirez/rax).
