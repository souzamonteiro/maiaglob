.PHONY: generate native test sanitize wasm test-wasm verify

generate:
	node scripts/generate.mjs
	node scripts/test-grammar.mjs
native:
	node scripts/build-native.mjs
test: generate native
	node scripts/test-matching.mjs
	node scripts/test-frontends.mjs
sanitize: generate
	node scripts/build-native.mjs --sanitize
	node scripts/test-matching.mjs
	node scripts/test-frontends.mjs
wasm:
	node scripts/build-wasm.mjs
test-wasm: test wasm
	node scripts/test-wasm.mjs
verify: sanitize wasm
	node scripts/test-wasm.mjs
