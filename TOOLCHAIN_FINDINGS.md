# Toolchain observations during MaiaGlob validation

2026-10-21. These are reduced observations, not fixes to the owner compilers.
Pinned MaiaCpp 09db55c, MaiaC 32ebd46, MaiaCC 1c7bbc5, Regex b66a74b.
No principal compiler/Regex was edited during the initial owner validation.
The separately authorized indexed-copy correction during consumer integration is
recorded below; this does not repair all earlier toolchain observations.

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

## Indexed struct assignment discovered by the real shell consumer

The consumer exposed corruption in `code[i] = program->code[j]`, which had been
lowered to a scalar load/store rather than a complete object copy. A native C89
reduction returned the expected sum 56, but the old MaiaC returned 1048; direct
field writes/readback returned 56 in both. The wrong value depends on addresses
and prior memory contents. Isolated passing tests are insufficient evidence for
this embedding case.

The owner explicitly authorized fixing MaiaC and propagating it. MaiaC f7f76bd
now retains indexed struct-valued rvalue addresses and emits a full struct copy
for indexed aggregate destinations. Native/guest reduction and added regressions
pass, along with its 43-group full bundle and 13 extra diagnostics. MaiaCpp
01a2fcc consumes that published fix. This update changes MaiaGlob's compiler
pin, not its runtime or generated frontend sources. Re-run the complete owner
validation with that pin before publication and downstream use.

## Member-array stride correction and walker coverage

The real shell exposed corrupted component paths (`child/t.txt` instead of
`child/deep.txt`). A reduced byte-array member expression advanced 20 bytes for
`text + 5`, while a 12-byte struct-array member advanced only four bytes for +1.
Published MaiaC 196f8ba resolves field element metadata and pointer classification;
its focused regressions and full bundle plus 13 extra diagnostics pass. The
existing address-of-index idiom remains valid source, but was insufficient to
avoid the defect in byte-buffer concatenation elsewhere.

Review also found that the original WASM walker harness did not execute its 18
`WALK_CASE` macro scenarios. MaiaC preprocessing left these calls unexpanded
after a later `#undef`; their guest calls did not establish coverage. Native
execution did run them. The replacement harness uses explicit calls, an execution
counter (exactly 18), and a deliberately wrong path expectation that must fail.
This change strengthens tests without changing expectations, library runtime,
grammar or API. A general preprocessor/unknown-call diagnostic repair remains a
separate owner issue; this harness no longer depends on that macro behavior.

## Reused walker storage and sizeof a dereferenced struct

After the stride fix, explicit guest cases reached scenario 8 and failed `.`
after `/`, although a fresh isolated `.` traversal passed. Inspecting poisoned
walker storage showed that `maia_glob_walk_begin` cleared only the first word:
MaiaC measured `sizeof(*w)` as the pointer size rather than the struct size.
The runtime's C89 memset expression is correct; no runtime workaround is applied.
Published MaiaC e173259 corrects this aggregate sizeof path; its complete
regression suite and repeated-reset test passed. Published MaiaCpp f800a2f
consumes the fix. Full MaiaGlob verification now passes all 18 guest scenarios
and repeated traversal with these pins. Diagnostics now expose the actual
case count, last status and joined paths when the guest walker assertion fails.
