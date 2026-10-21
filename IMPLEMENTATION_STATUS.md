# MaiaGlob implementation status

2026-10-21. The owner approved DESIGN.md implementation and subsequently
authorized public souzamonteiro/maiaglob creation, initial commit/push and a pinned
MaiaOS submodule. This initial publication contains the validated owner library.
No npm release, version change or MaiaOS/cat publication is authorized in this
step. Consumer integration follows owner publication through its own submodule.

## Implemented locally

- C89 validated instruction/range API, strict UTF-8 scalar decoding, iterative
  two-row NFA matcher and finite input/storage/work limits.
- MaiaCC-generated JavaScript/C++98 lexer/parser from grammar/glob.ebnf, bounded
  C++98 event collection and semantic instruction construction. Protected-byte
  masks preserve a caller's quoting decisions without introducing a shell parser.
- Explicit-stack resumable filesystem callback walker; no dirent/host matching.
  Cancellation, pending output retry, provider errors and traversal quotas.
- Strict native library build, sanitizer build, generated frontend comparison,
  independent matching oracle, invalid API tests, callback fixtures, own pinned
  MaiaCpp/MaiaC/Regex/WASM build and raw-byte execution harness.

## Verified milestones

- `node scripts/test-grammar.mjs`: PASS, ASCII literal coverage and valid/invalid
  class/escape/path/Unicode syntax with the generated JavaScript parser.
- `node scripts/build-native.mjs --sanitize` and
  `node scripts/test-matching.mjs`: PASS, strict C89/C++98, ASan/UBSan; 10638 cases
  including exhaustive reference-DP comparisons, Unicode, protected masks,
  invalid encodings/classes, finite adversarial matching and API bounds.
  Native driver also runs 18 walker scenarios and 100 cancellation repetitions.
- `node scripts/test-frontends.mjs`: PASS, 10621 accepted cases with identical
  JavaScript-generated AST semantics and native C++98 instruction/range output.
- `make verify`: PASS (exit 0), final log /tmp/maiaglob-verify-final.log.
  Generated JavaScript grammar, strict C89/C++98 sanitizer build, native reference
  corpus and frontend parity passed. Full frontend/runtime compiled through the
  pinned MaiaCpp -> C89 -> MaiaC/MaiaWASM path, with strict emitted C89 validation.
  Actual WASM: **10638 cases identical** to native, including statuses, complete
  instruction/range programs and match work counters. Guest walker and API
  contract checks also passed. 500 repetitions (with repeated full walker tests)
  restored stack and kept memory stable at 18 pages. Compiler scratch is 94912
  bytes on WASM. Test binary with the corrected compiler is 1144603 bytes and is ignored under build/.
- `node scripts/check-generated.mjs`: PASS; all 5 generated source artifacts
  reproduce byte for byte and all 16 recursive dependency pins match.
- The old adversarial 49-byte pattern on 32 input bytes finishes with NOMATCH
  after 3217 matcher steps. With a 100-step quota it returns LIMIT at exactly 100.
  This is a bounded-work result, not a general timing benchmark.

## Publication / consumer work pending

Owner-library functional validation is complete. Reduced toolchain observations
are recorded in TOOLCHAIN_FINDINGS.md. The initial library is public at commit
c262104. Consumer integration then exposed indexed struct-copy corruption in
MaiaC. The owner authorized its correction and propagation: published MaiaC
f7f76bd / MaiaCpp 01a2fcc are now pinned here, and `make verify` plus generated
reproduction passed again (log /tmp/maia-glob-struct-copy-verify.log).
Library API, runtime source and generated frontend source are unchanged.

The owner repository is public and its first commit is already a MaiaOS
submodule. Publish this validated compiler-pin update before the consumer uses
it. Continue the already approved shell work to
preserve generated-shell quote provenance, add filename expansion before command
dispatch, and extend ls to multiple literal/expanded file/directory operands.
Actual VFS privacy, backpressure and LVGL/kernel tests are still consumer work.
The existing dirty MaiaOS cat feature is preserved and unpublished.
