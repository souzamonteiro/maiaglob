# MaiaGlob

Portable filename-pattern library for the Maia Suite: bounded C89 matcher and
callback directory walker, with a C++98 pattern compiler generated from EBNF by
MaiaCC. The owner project is published at
https://github.com/souzamonteiro/maiaglob. Shell integration consumes a published
commit through its own submodule, as required by the ecosystem protocol.
No npm release or version change is part of the current work.

The original `../glob-1.0` is preserved unchanged. MaiaGlob is a new Apache 2.0
implementation, without recursive star backtracking, a native dirent dependency,
or hidden list allocation. See [provenance](PROVENANCE.md).

## Build and verify

Requirements: Git, Node.js, make, Clang/Clang++, ar. Compiler/generator versions
come from this repository's own published MaiaCpp submodule and recursive pins.

```sh
git submodule update --init --recursive
make test        # generate; JavaScript grammar oracle; native; reference/parity
make sanitize    # same native functional tests under ASan and UBSan
make test-wasm   # own MaiaCpp -> strict C89 check -> own MaiaC/MaiaWASM; execute
```

`make native` produces `build/libmaiaglob.a`. Include `maia_glob.h`; link the
archive for text compilation and matching. Runtime-only consumers can compile
`maia_glob.c` without C++, Regex or a filesystem implementation. The full compiler
uses the pinned MaiaRegex guest runtime. No C++ standard-library dependency is
required. Objects, emitted C89, corpus transport files and test WASM stay ignored
under `build/`.

The `generated/` directory is a reproducible source artifact: untouched MaiaCC
C++98/JavaScript parser output, XML, the matching pinned Regex header and an
identifier-only prefix include adapter. This project versions those generated
source artifacts alongside their authoritative EBNF and generation script so
consumers can reproduce exactly the tested grammar. Do not edit generated files.
The prefix adapter avoids generated identifier collisions when C89 outputs are
combined; it does not change grammar, tokenization or matching behavior.

See [API and limits](API.md), [approved architecture](DESIGN.md), and
[implementation/test status](IMPLEMENTATION_STATUS.md). Directory tests use an
explicit deterministic callback provider. They are not claims of MaiaOS VFS or
LVGL integration. Host JavaScript in the WASM harness only transports raw bytes
and memory operations; the guest performs compilation, matching and traversal.
