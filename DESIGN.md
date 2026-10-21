# MaiaGlob and shell expansion proposal

Status: approved by the owner on 2026-10-21; owner library implemented and native/WASM validated; shell integration pending.
The owner explicitly authorized this contract, excluding GitHub repository
creation, commit/push, npm publication and version changes. See API.md for exact
implemented signatures/status values and IMPLEMENTATION_STATUS.md for evidence.
Existing MaiaOS cat work is preserved separately. Shell consumption must wait for
separately authorized owner publication and a pinned in-repository submodule.

## Boundaries and targets

1. C89 matcher runtime, C++98-compatible public headers with extern-C guards,
   named public structs and no global mutable execution state. Separate native
   adapter/testing from the freestanding core. Never require POSIX dirent, native
   filesystem access, locale state or a JavaScript matcher in a WASM guest.
2. Compile pattern syntax once into a validated instruction/range program; match
   many filenames against that program using iterative finite-state execution
   and caller-owned scratch space. No recursive wildcard backtracking, global
   cursor, hidden unbounded allocation or silent truncation. Check every count /
   size arithmetic operation and report malformed input or exhausted limits.
3. EBNF is authoritative for a new pattern-text compiler, generated only with
   MaiaCC, followed by bounded semantic program construction. Do not handwrite a
   replacement shell parser or a pattern parser to bypass the project contract.
   The C89 program runtime can be tested independently of the generated frontend,
   as in MaiaRegex. Native full-library and MaiaCpp/C89/MaiaC/WASM tests must also
   cover the text frontend before declaring user patterns supported.
4. Optional path enumerator uses caller-supplied directory callbacks; the same
   matcher serves other programs independently of the shell/VFS. MaiaOS binds
   these callbacks to existing maia.vfs.opendir/readdir/close. The host only
   transports directory packets; matching and traversal stay in guest code.
5. New sources in principal maiaglob only. Follow deepest-owner publication and
   gitlink propagation before MaiaOS consumes the library. No sibling include,
   symlink, copied development dependency or unpublished gitlink in normal builds.
   MaiaGlob must use its own pinned compiler/generator dependencies when needed.

## Proposed portable API

Public header `maia_glob.h`, implementation `maia_glob.c`, explicit structs for
programs, ranges, limits, compile storage, matcher workspace and walker state.
The implementation defines storage sizes and ownership alongside API tests.
No promise of binary compatibility with the old Glob_Glob/List_ListItem API.

Conceptual signatures to approve (all counts are bounded unsigned integers):

```c
int maia_glob_validate(const MaiaGlobProgram *program);
int maia_glob_compile_utf8(const unsigned char *pattern, unsigned int length,
    const unsigned char *protected_bytes, MaiaGlobStorage *storage,
    const MaiaGlobLimits *limits, MaiaGlobProgram *program);
int maia_glob_match_utf8(const MaiaGlobProgram *program,
    const unsigned char *name, unsigned int length,
    MaiaGlobWorkspace *workspace, const MaiaGlobLimits *limits);
int maia_glob_walk_begin(MaiaGlobWalker *walker,
    const MaiaGlobProgram *program, const MaiaGlobFs *filesystem,
    const MaiaGlobLimits *limits);
int maia_glob_walk_step(MaiaGlobWalker *walker,
    unsigned char *path, unsigned int capacity, unsigned int *length,
    unsigned int *entry_type, unsigned int work_budget);
void maia_glob_walk_cancel(MaiaGlobWalker *walker);
```

The filesystem provider owns authorization and opaque process-local directory
handles. Callback table operations: open(context,path,length)->handle/status,
read(context,handle,name,capacity,type)->length/EOF/status, close(context,handle)
->status. No exposed host pointers. Native adapters may map handle IDs to DIR*
in their own context; the portable core does not depend on that representation.
The walker is resumable, has a finite work budget per step, retains explicit
pending state, closes handles on cancellation/error/completion and never spins
on WOULD_BLOCK. Workspaces/buffers/programs remain caller-owned.

Result families must distinguish match/no-match, invalid input/program, exhausted
limits/storage, provider error, yielded work, produced path and completed walk.
They are library results mapped by the adapter, not new MaiaOS syscall statuses.
An implementation must document exact numeric values before publishing its API.

Initial syntax: literals, `*`, `?`, bracket sets/ranges and `[!set]`, escapes,
path separators and shell-supplied protected-byte masks. Explicitly define edge
cases such as first/last hyphen, literal closing bracket, reversed ranges and
malformed brackets. `^` negation is a legacy extension, not an implicit promise.
Support strict UTF-8 scalar matching: `?` consumes one scalar, not one byte.
Use deterministic scalar/range and UTF-8 path ordering, not locale collation.
No initial extglob, brace expansion, tilde, recursive globstar, POSIX named
character classes, collation/equivalence classes or full-locale/POSIX claims.
Do not silently substitute regex syntax for glob syntax.

Path matching treats separators as component boundaries; wildcards cannot cross
slash. In filename-expansion mode, leading-dot names need an explicit leading
dot in the relevant pattern component. Literal/quoted patterns preserve exact
characters. User-visible malformed-pattern and no-match policy must be tested;
proposed no-match shell behavior is to retain the original word. Resource errors
must remain distinguishable from ordinary no-match and must not execute a partial
expanded command. No-match is not permission to expose denied namespace contents.

The runtime validates a whole-match program, with no capture/greedy-result API;
boolean glob matching does not inherit the ordered capture semantics of regex.
UTF-8 decoding rejects malformed encodings and compares scalars exactly, without
normalization or locale equivalence. A protected-byte mask supplied by the shell
must consistently protect every byte of a quoted scalar.

Limits include pattern bytes/scalars, instructions/ranges, scratch bytes, match
steps, traversal depth, scanned entries/directories, emitted paths and result
bytes. All operations require finite caller limits; zero does not silently mean
unlimited work. The implementation must publish measured concrete defaults and
storage requirements with tests before claiming a ready-to-use public API.
A walker may produce entries in provider order; the expansion stage collects and
sorts matches for each original word by UTF-8 bytes before dispatch. It must not
execute a partial argv or stream matches directly into command execution.

## Shell integration and multi-operand ls

Filename expansion belongs between generated parse-tree validation / word
evaluation and builtin dispatch. Do not implement matching only inside ls or
send raw wildcard patterns to host JavaScript for expansion. Other commands then
receive the same expanded argv; commands with stricter arity still reject excess
operands (cat remains single-file until separately extended).

Current buildCommandWords removes quotes/escapes and stores decoded strings.
Preserve per-byte quoting/escape provenance while evaluating generated tokens,
so `ls *.c`, `ls "*.c"`, `ls \\*.c` and mixed quoted words differ correctly.
Fix only relevant quote-removal semantics at this boundary, retaining source
positions and generated lexer/parser parity. No hand edits to generated sources.

Expansion uses only the caller's VFS snapshots. Respect current private-home
isolation, read-only mounts, root clamping, cwd, UTF-8 names and lifecycle. Never
probe the native filesystem or browser URL. Forward expanded names as data,
without reparsing them as shell source. Spaces/control characters in filenames
must not become extra commands; terminal listing names remain escaped.

Keep initial guest input/token/tree/word/argv limits unless separately reviewed:
255 input bytes, 32 tokens, 1024 nodes, 12 decoded/expanded words including command,
95 bytes per decoded/expanded word and 1023-byte normalized paths. Core library
limits may be caller-configurable; the shell reports expansion limits explicitly
instead of truncating matches or expanding its memory budget silently. A normal
ls directory listing still supports its current 1024-entry snapshot budget.

Extend ls separately to multiple operands and file operands, including literal
names without wildcards. Use existing authorized directory snapshots for entry
metadata; do not invent stat syscalls. Define stable ordering, file/directory
headings, missing/denied operand errors and continued processing of later operands
without leaking handles. Yield under stdout backpressure; kernel stays scheduled.
The default one-directory ls behavior and all current cat/dir regressions remain.

## Validation and publication sequence

- Preserve original glob-1.0 and maiaregex; record license/provenance accurately.
  Proposed new implementation follows Apache 2.0, without relabeling GPL sources.
- Native strict C89/C++98 tests, ASan/UBSan, exhaustive/sampled comparison against
  an independent small reference on a defined subset and system fnmatch only
  where locale/flags/semantics agree. Cover malformed syntax, Unicode, ranges,
  protected characters and adversarial star patterns with finite work limits.
- Build/generated frontend and pure runtime through repository-local pinned
  Maia Suite dependencies; compare native and actual WASM outcomes, return codes,
  consumed work, cleanup and stack/memory stability. Native-only matching is a
  partial milestone, not proof of the complete generator/toolchain path.
- Once the owner project is validated, obtain separate commit/push/GitHub
  creation authorization as needed. Publish owner before adding a pinned MaiaOS
  submodule and validating downstream. No implicit npm/version release.
- Actual shell tests: multi-file/dir ls, unquoted vs quoted/escaped/mixed globs,
  Unicode names, dotfiles, no matches, inaccessible homes, overflow, paths across
  components, malformed patterns, cleanup, repeated expansion, slow/closed peers
  and real LVGL/kernel scheduling. Keep the generated-shell parity corpus.

## Reference

The scope is informed by The Open Group Shell Command Language, pattern matching
and filename expansion, not by claims that the old matcher was fully conformant:
https://pubs.opengroup.org/onlinepubs/9799919799/utilities/V3_chap02.html
