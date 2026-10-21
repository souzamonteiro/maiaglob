# MaiaGlob portable API

The public contract is in `maia_glob.h`. Core execution is C89; the separate
pattern compiler is C++98 and uses only the MaiaCC-generated lexer/parser from
`grammar/glob.ebnf`. All execution state is supplied by the caller. No recursive
wildcard matching or recursive directory traversal is used.

## Results and ownership

| Value | Name | Meaning |
| --- | --- | --- |
| 0 | MATCH | Match, valid program or successful compilation/initialization |
| 1 | NOMATCH | Valid name does not match |
| 2 | INVALID | Invalid arguments, UTF-8, pattern or program |
| 3 | LIMIT | Finite work/input/traversal quota exhausted |
| 4 | IO | Filesystem provider failure |
| 5 | YIELD | Resume the walk later |
| 6 | ENTRY | One complete path produced |
| 7 | DONE | Enumeration completed |
| 8 | SPACE | Caller storage insufficient |

Programs borrow the instruction/range buffers in `MaiaGlobStorage`. The compiler
scratch area must be aligned for pointers, at least the queried
`maia_glob_compile_workspace_size()` bytes, and separate from input/output arrays.
Use malloc for portable alignment, or an appropriately aligned caller buffer.
Buffers and the workspace must not overlap. Compilation sets the output program
only on success, but may modify instruction/range storage on failure. Any previous
program borrowing that storage must be discarded before compiling again. Compiler
scratch can be reused after compilation; compiled programs never refer to it.
The generated lexer uses guest libc allocation internally; allocation is bounded
by the fixed input/grammar limits. The C89 matcher/walker allocate nothing.

The matcher needs two distinct caller arrays, each at least `code_count` bytes.
They must not overlap each other, input or program storage. Its `steps` count
includes NFA state scans and tested class ranges. Program/input validation is
separate, bounded by their explicit sizes. Inputs are counted byte strings;
embedded NUL and invalid UTF-8 are rejected. Null input is allowed only at length
zero. No normalization, locale state or case folding is performed.

## Syntax and flags

`*` consumes zero or more Unicode scalars; `?` one scalar; `[a-z]` a scalar range;
`[!a]` a negated set. `[-a]`, `[a-]` and `[]a]` include the literal hyphen or first
closing bracket. Backslash quotes the next scalar. Reversed ranges, unclosed or
empty sets, trailing backslash and slash inside a set are invalid. Outside sets,
`]`, `!` and `-` are literals; `^` is a set member, not a negation operator.
Named POSIX classes, equivalence/collation classes, braces and extglob are outside
the initial profile. There is no special recursive `**` operator.

`PATHNAME` (1) prevents wildcards crossing slash. `PERIOD` (2) requires a literal
dot at the beginning of each matching component for a leading dot in the name;
a bracketed dot and a star before a dot do not satisfy that rule. Compilation
sets both flags. Other flag bits are invalid. Matching is whole-name only.

An optional mask has one byte for every pattern byte. Nonzero protects the
corresponding scalar from pattern syntax; all bytes of a scalar must have the
same protection. Protected scalars are transported as literal placeholders to the generated
grammar; semantic construction restores their values using a caller-workspace
side table. This prevents quote protection from accidentally combining with an
adjacent backslash. This is quote provenance, not a second shell parser. The shell
must construct this mask from its own generated parse tree before dispatch.

## Default finite limits

| Resource | Default |
| --- | ---: |
| Pattern bytes | 256 (hard maximum 256) |
| Instructions / ranges | 257 / 256 hard maxima |
| Match input bytes | 4096 |
| Match work steps | 1048576 |
| Walk depth | 16 (hard maximum 16) |
| Scanned entries / directory open attempts | 16384 / 256 |
| Emitted paths | 1024 |
| Total result bytes, including each NUL | 1048576 |
| Walker work iterations | 1048576 |
| Output path / entry name bytes | 1023 / 255 hard maxima |

All limits must be nonzero. Larger byte/work quotas are finite unsigned counts;
pattern/depth hard maxima cannot be raised through the limits structure.
Each directory entry match also has the configured per-match step quota.
Directory attempts count completed open callbacks, including rejected branches;
WOULD_BLOCK does not consume that counter. `max_walk_steps` counts traversal
iterations, including would-block retries.
Caller storage exhaustion is distinct from those resource limits.

## Filesystem callbacks and resumable walks

Providers implement `open(context,path,length)`, `read(context,handle,name,capacity,
&entry_type)` and `close(context,handle)`. A valid handle is positive. Read returns
positive UTF-8 name byte length, zero at EOF, or a negative error. Names exclude
slash and NUL. Types are FILE (1) and DIRECTORY (2); the provider owns metadata,
authorization, namespace snapshots and any symlink policy. No symlink following
or native filesystem access is supplied by this library.

WOULD_BLOCK (-6) from open/read yields immediately. Missing (-2), denied (-3) and
not-a-directory (-9) branches are skipped without exposing names. Resource (-4)
becomes LIMIT; other errors become IO. Close must complete synchronously; a
negative close result is IO during normal execution. Cancellation attempts to
close all remaining handles and has no error return. `provider_status` preserves
an actual provider error associated with a terminating failure.

Initialize with `walk_begin`, then call `walk_step` with a positive finite work
budget. ENTRY writes one NUL-terminated path and its length/type. SPACE preserves
that pending result for retry with a larger buffer and leaves output arguments
unchanged. YIELD emits no partial path. DONE, IO and LIMIT close all live handles.
Cancel before reinitializing an active walker. Calling cancel repeatedly is safe.
After explicit cancellation the walker is inactive; start a new walk before reuse.

Relative walks start at the provider's `.`; absolute walks at `/`. Empty patterns
produce no entries. Literal `.` and `..` components are resolved only through the
provider, which must enforce root clamping and authorization. Directory-only
patterns retain a final slash in results. Repeated separators may be normalized.
Provider order is preserved. Consumers must collect and sort each word's matches
by UTF-8 bytes and apply argv quotas before executing any command. The library
does not execute commands or decide no-match shell behavior.
