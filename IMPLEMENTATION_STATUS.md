# MaiaGlob implementation status

2026-10-21. Owner implementation and strengthened native/WASM validation are
complete. Public repository creation, initial publication, submodule integration
and tested compiler-fix propagation are explicitly authorized. No npm release,
version change or MaiaOS/cat publication is included.

## Implemented

- Portable C89 validated instruction/range API, strict UTF-8 scalar matching,
  iterative two-row NFA, finite storage/work limits and caller-owned state.
- MaiaCC-generated JavaScript/C++98 pattern lexer/parser from grammar/glob.ebnf;
  bounded semantic construction and protected-byte masks for caller quoting.
- Explicit-stack resumable callback walker, yields, cancellation, pending-output
  retry, provider errors and finite traversal quotas. No host matching or dirent.
- Own pinned MaiaCpp/MaiaC/Regex/WASM toolchain, native/sanitizer and actual WASM
  execution, independent matching oracle and generated-source reproduction.

## Final verification

- `make verify`: PASS. Strict C89/C++98 ASan/UBSan, JavaScript grammar coverage,
  **10638 native reference cases**, **10621 JavaScript/native frontend semantic
  programs**, and **10638 actual native/WASM cases** with identical statuses,
  complete instruction/range programs and match work counters.
- Native and guest execute **18 callback traversal scenarios**, assert their
  execution count, and reject a deliberately wrong pathname expectation. This
  includes component paths, root/dot forms, yields, limits, faults and cleanup.
  An additional 100 cancellation cycles run per full walker check.
- 500 repeated guest operations, including a full walker rerun every 25 rounds:
  stack restored, memory stable at **18 pages**. Compiler workspace **94912 bytes**.
  Ignored test WASM is **1149947 bytes**.
- `node scripts/check-generated.mjs`: PASS. All **5 generated source artifacts**
  reproduce byte for byte; **16 recursive dependency pins** match.
- Adversarial 49-byte pattern / 32-byte input: NOMATCH after 3217 finite steps;
  a 100-step quota returns LIMIT at exactly 100. This is not a general benchmark.

## Coverage correction and compiler findings

The first guest walker claim was incomplete: MaiaC preprocessing left the 18
WALK_CASE macro calls unexpanded after a later undef. Native execution did run
those cases; the guest result did not prove their execution. The matching/program
corpus, API checks and explicit cancellation loop remain valid earlier evidence.

The corrected harness uses explicit calls, an exact execution counter, a negative
control, and last-status/path diagnostics. Its actual execution exposed retained
state after root traversal. No expected result was weakened to pass.

Consumer integration exposed three separately reduced MaiaC aggregate defects:
indexed struct copy, member-array/pointer-field strides, and sizeof a dereferenced
struct returning pointer size (incomplete memset). Published MaiaC **e173259** and
MaiaCpp **f800a2f** contain the authorized corrections and are pinned here. Final
validation above passed with those published dependencies. Library API, runtime
and generated frontend sources are unchanged. See TOOLCHAIN_FINDINGS.md for the
remaining general preprocessor and generator observations.

## Publication and consumer boundary

Initial owner publication is c262104; compiler propagation began at 3191731.
This update records the fully tested compiler pin, stronger harness and corrected
evidence. MaiaOS must consume the published owner commit through its own
submodule before completing real VFS/stream/kernel/LVGL integration tests.
The dirty MaiaOS shell/cat work remains local and unpublished.
