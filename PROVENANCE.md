# Origin and audit

Audit date: 2026-10-21. Source directory: the owner's `glob-1.0`, unchanged.

- glob.c / glob.h: copyright 2015 Roberto Luiz Souza Monteiro and Hernane Borges
  de Barros Pereira; headers specify GPL version 2 or later.
- list.c / list.h: copyright 2016 Roberto Luiz Souza Monteiro; same GPL headers.
- glob.bun: a shell archive containing a separate `amatch` source attributed to
  Ozan S. Yigit, marked public domain in its own header (1994/1995). The archive
  was read as text, not executed or unpacked over the owner's files.
- MaiaRegex currently carries Apache 2.0 and a separately documented bounded C89
  program API. Its older README also contains broader legacy claims and a C99 /
  C++11 Makefile path; those are not the standards to copy into this project.

No legacy source is copied into the new implementation. MaiaGlob uses the
MaiaRegex project's Apache 2.0 convention. No public-domain matcher source was
reused either; the audit informs regression cases and design choices. The GPL wrapper
and linked-list code will not simply have their license headers replaced.
The original directory remains an independent historical reference.

## Reproduced behavior

Compiled the original glob.c/list.c with Clang strict C89 and a temporary matching
driver. No files were edited in glob-1.0 and no existing binaries were overwritten.
Observed results (1 match, 0 no match):

| String | Pattern | Legacy result |
|---|---|---|
| a | [a] |1 |
| b | [!a] |0 |
| a | [!a] |1 |
| b | [^a] |1 |
| - | [a-] |0 |
| z | [a-] |1 |
| empty | * |1 |
| .hidden | * |1 |
| a/b | * |1 |
| é | ? |0 |

A 32-byte string of `a` with pattern `*a` repeated 24 times followed by `b`
(49 pattern bytes) exceeded a 1-second subprocess timeout. This demonstrates the
need for finite work limits and a nonrecursive matcher; it is not a complete
performance characterization or evidence that every legacy pattern is wrong.
The legacy matcher has no pathname/leading-dot modes, so those rows establish
its current behavior rather than claim such flags existed.

The directory wrapper directly uses dirent, malloc and a linked list; its max
counter advances for every scanned entry, not only matches, and malloc results
are not checked. These are source observations, separate from the reproduced
matching tests. The old CLI test is only a small demonstration, not conformance.

Temporary audit artifacts are outside the repositories, under
`maia-glob-audit-*` in the platform temporary directory. No source commit,
GitHub repository creation, push, npm action or version change occurred.
