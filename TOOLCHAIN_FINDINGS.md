# Toolchain observations during MaiaGlob validation

2026-10-21. These are reduced observations, not fixes to the owner compilers.
Pinned MaiaCpp 09db55c, MaiaC 32ebd46, MaiaCC 1c7bbc5, Regex b66a74b.
No principal compiler/Regex or its pinned submodule was edited.

## MaiaCC character-class spelling

The initial CHAR token excluded punctuation with this valid EBNF class:

```
CHAR ::= [^#x0#x2A#x3F#x5B#x5D#x21#x2D#x5C#x2F]
```

The JavaScript output rendered the hex hyphen as an unescaped hyphen in the
middle of the regex class. The resulting unintended range rejected dot; both
native generated C++98 and JavaScript rejected `.*` and `*.c`. A positive union
of intervals ending at #x10FFFF also omitted its final interval in JavaScript
output, rejecting `a`. These generator cases need a separate owner regression
and correction; no generated source was patched here.

The authoritative MaiaGlob EBNF now uses the identical exclusion set with the
hyphen first. That spelling produces a literal hyphen rather than an accidental
regex range. The generated JavaScript ASCII-literal coverage and semantic-program
comparison cover this spelling along with Unicode. This is not a general repair
to MaiaCC character-class emission.

## MaiaC aggregate state in the probe

Global aggregate objects in the first probe yielded invalid/null addresses and
were rejected by the library's argument validation in WASM. Pointer-backed
caller storage avoids relying on that unsupported global-aggregate path. The
production library has no mutable global execution state. This must not be
reported as proof that arbitrary global C/C++ aggregate programs work.

## MaiaC pointer arithmetic on a member array

The generated frontend adapter originally used this ordinary source expression:

```cpp
GlobNode* node = s->nodes + s->count;
```

GlobNode has five 32-bit fields on WASM (20 bytes). Calling the exported guest
append function twice demonstrated a second node starting at an offset of four
bytes, corrupting the first node. The same native function used the proper
stride. A one-node append/end worked, and checkpoint/restore arithmetic separately
round-tripped 0, 65537, 131074 and 196611 correctly. This localizes the divergence
to member-array pointer arithmetic, rather than an unbounded grammar recursion.

The new source uses explicit address-of indexing, `&s->nodes[s->count]`, and the
same equivalent idiom for the library's other typed member-buffer accesses. This
preserves the API and generated lexer/parser. Do not manually alter emitted C89
or generated parser output. The final native/WASM test results belong in
IMPLEMENTATION_STATUS.md; generation alone is not successful execution.

## UTF-16 lexical units versus Unicode scalars

The pinned generated JavaScript and C++98 lexers deliberately use UTF-16 units.
A CHAR token matching one such unit splits an astral scalar: C++ byte offsets
for the first surrogate cover zero bytes. This was caught by a 256-byte pattern
containing 64 globe emoji, rather than by matching an emoji name with `?` alone.
The EBNF now explicitly pairs high/low surrogates in CHAR and ESCAPED, excludes
unpaired surrogates from the single-unit alternatives, and generates both targets
again. Generated AST token values then contain whole UTF-8 scalars for bounded
semantic construction. No lexer/decoder output was hand-edited.

The probe uses explicit object sizes and memcpy snapshots for storage/unchanged-
output checks; it does not rely on `sizeof(*function_returning_pointer())` or
aggregate assignment from a pointer-return expression being lowered correctly.
These are test-harness portability choices, not fixes to general MaiaC semantics.
