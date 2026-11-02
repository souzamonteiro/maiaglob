# Additive BRE toolchain propagation

2026-11-02. Published MaiaCpp16cca71 was recorded after completed git_pull and
before tests. Its recursive source pins consume MaiaC3007495, MaiaCCfe00596,
MaiaWASM8f5d02b and MaiaRegex8a1dcbf. No Glob grammar/compiler/runtime edit.

make test-wasm passed10,638 native/own-WASM cases with identical status,
instructions/ranges/work counters,18 actual callback scenarios, API contracts,
and500 stable repeated runs at18pages (94,912-byte compile workspace).
The generated JS/C++98 frontend corpus passes10,621 semantic-program comparisons.
make sanitize repeated native/frontend gates under ASan/UBSan, all pass. The
printed deliberately wrong traversal path is an existing successful negative
control, not an undisclosed failure.

The generated C++98 parser changed only for the already published MaiaCC parser
resourceError fields/reset/NOMEM/LIMIT reporting. It was regenerated through the
recorded in-repo generator, never edited by hand. A further full regeneration
reproduced every generated artifact byte for byte. The authored grammar and
old matcher sources remain unchanged.

Regex's development tools/maiacpp remains intentionally uninitialized inside
ordinary consumers (`update=none`); its historical lock stays fixed. No npm,
versions, history or MaiaOS publication. This is native/Node-WASM evidence;
MaiaOS VFS/LVGL and VM/device conformance belong to their separate integrations.
