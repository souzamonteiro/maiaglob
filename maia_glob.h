/* MaiaGlob portable runtime. Copyright 2026 Roberto Luiz Souza Monteiro.
 * SPDX-License-Identifier: Apache-2.0 */
#ifndef MAIA_GLOB_H
#define MAIA_GLOB_H
#include <stddef.h>
#define MAIA_GLOB_MATCH 0
#define MAIA_GLOB_NOMATCH 1
#define MAIA_GLOB_INVALID 2
#define MAIA_GLOB_LIMIT 3
#define MAIA_GLOB_IO 4
#define MAIA_GLOB_YIELD 5
#define MAIA_GLOB_ENTRY 6
#define MAIA_GLOB_DONE 7
#define MAIA_GLOB_SPACE 8
#define MAIA_GLOB_CHAR 1
#define MAIA_GLOB_ANY 2
#define MAIA_GLOB_STAR 3
#define MAIA_GLOB_CLASS 4
#define MAIA_GLOB_NOT_CLASS 5
#define MAIA_GLOB_END 6
#define MAIA_GLOB_PATHNAME 1u
#define MAIA_GLOB_PERIOD 2u
#define MAIA_GLOB_FILE 1u
#define MAIA_GLOB_DIRECTORY 2u
#define MAIA_GLOB_MAX_PATTERN 256u
#define MAIA_GLOB_MAX_CODE 257u
#define MAIA_GLOB_MAX_RANGES 256u
#define MAIA_GLOB_MAX_PATH 1023u
#define MAIA_GLOB_MAX_DEPTH 16u
#define MAIA_GLOB_FS_WOULD_BLOCK (-6)

typedef struct MaiaGlobInstruction { int op; unsigned int first; unsigned int count; } MaiaGlobInstruction;
typedef struct MaiaGlobRange { unsigned int first; unsigned int last; } MaiaGlobRange;
typedef struct MaiaGlobProgram {
    const MaiaGlobInstruction *code; const MaiaGlobRange *ranges;
    unsigned int code_count; unsigned int range_count; unsigned int flags;
} MaiaGlobProgram;
typedef struct MaiaGlobLimits {
    unsigned int max_pattern_bytes; unsigned int max_input_bytes;
    unsigned int max_steps; unsigned int max_depth; unsigned int max_entries;
    unsigned int max_directories; unsigned int max_matches;
    unsigned int max_result_bytes; unsigned int max_walk_steps;
} MaiaGlobLimits;
typedef struct MaiaGlobWorkspace {
    unsigned char *first; unsigned char *second; unsigned int capacity;
    unsigned int steps;
} MaiaGlobWorkspace;
typedef struct MaiaGlobStorage {
    MaiaGlobInstruction *code; unsigned int code_capacity;
    MaiaGlobRange *ranges; unsigned int range_capacity;
    void *compiler_workspace; size_t compiler_workspace_bytes;
} MaiaGlobStorage;
typedef struct MaiaGlobFs {
    void *context;
    int (*open)(void *, const unsigned char *, unsigned int);
    int (*read)(void *, int, unsigned char *, unsigned int, unsigned int *);
    int (*close)(void *, int);
} MaiaGlobFs;
typedef struct MaiaGlobFrame {
    unsigned char path[1024]; unsigned int length;
    unsigned int begin; unsigned int end; unsigned int next;
    int handle; unsigned int synthetic;
} MaiaGlobFrame;
typedef struct MaiaGlobWalker {
    const MaiaGlobProgram *program; MaiaGlobFs filesystem; MaiaGlobLimits limits;
    MaiaGlobFrame frames[16]; unsigned int depth; unsigned int active;
    unsigned int scanned; unsigned int directories; unsigned int matches;
    unsigned int result_bytes; unsigned int work; int provider_status; int status;
    unsigned char states_a[257]; unsigned char states_b[257];
    unsigned char name[256]; unsigned char pending_path[1024];
    unsigned int pending_length; unsigned int pending_type; unsigned int pending;
} MaiaGlobWalker;
#ifdef __cplusplus
extern "C" {
#endif
void maia_glob_default_limits(MaiaGlobLimits *limits);
int maia_glob_validate(const MaiaGlobProgram *program);
size_t maia_glob_compile_workspace_size(void);
int maia_glob_compile_utf8(const unsigned char *pattern, unsigned int length,
    const unsigned char *protected_bytes, MaiaGlobStorage *storage,
    const MaiaGlobLimits *limits, MaiaGlobProgram *program);
int maia_glob_match_utf8(const MaiaGlobProgram *program,
    const unsigned char *name, unsigned int length,
    MaiaGlobWorkspace *workspace, const MaiaGlobLimits *limits);
int maia_glob_walk_begin(MaiaGlobWalker *walker, const MaiaGlobProgram *program,
    const MaiaGlobFs *filesystem, const MaiaGlobLimits *limits);
int maia_glob_walk_step(MaiaGlobWalker *walker, unsigned char *path,
    unsigned int capacity, unsigned int *length, unsigned int *entry_type,
    unsigned int work_budget);
void maia_glob_walk_cancel(MaiaGlobWalker *walker);
#ifdef __cplusplus
}
#endif
#endif
