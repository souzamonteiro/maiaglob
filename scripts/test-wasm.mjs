import assert from 'node:assert/strict';
import fs from 'node:fs';
const module=new WebAssembly.Module(fs.readFileSync('build/maiaglob-test.wasm'));
let instance;
const env={
 __builtin_wasm_memory_size:i=>i===0?instance.exports.memory.buffer.byteLength/65536:-1,
 __builtin_wasm_memory_grow:(i,n)=>{if(i!==0)return -1;try{return instance.exports.memory.grow(n);}catch{return -1;}},
 __malloc:n=>instance.exports.malloc(n),__free:p=>instance.exports.free(p)
};
for(const item of WebAssembly.Module.imports(module))assert.ok(item.module==='env'&&item.name in env,`Unexpected host dependency ${item.module}.${item.name}`);
instance=new WebAssembly.Instance(module,{env});const e=instance.exports;
const corpus=JSON.parse(fs.readFileSync('build/match-corpus.json','utf8'));
const raw=h=>h==='-'?Buffer.alloc(0):Buffer.from(h,'hex');
const stack=e.__stack_ptr.value;
const execute=c=>{
 const p=raw(c.pattern),n=raw(c.name),mask=c.mask?raw(c.mask):null;
 new Uint8Array(e.memory.buffer).set(p,e.probe_pattern());new Uint8Array(e.memory.buffer).set(n,e.probe_name());
 if(mask)new Uint8Array(e.memory.buffer).set(mask,e.probe_mask());
 const compile=e.probe_compile(p.length,mask?1:0),match=compile===0?e.probe_match(n.length,c.flags,c.limit):-1;
 const code=e.probe_code_count(),ranges=e.probe_range_count();
 const result=[compile,match,e.probe_steps(),code,ranges];
 if(compile===0){for(let i=0;i<code;i++)result.push(`${e.probe_op(i)},${e.probe_first(i)},${e.probe_count(i)}`);for(let i=0;i<ranges;i++)result.push(`${e.probe_range_first(i)},${e.probe_range_last(i)}`);}
 assert.equal(e.__stack_ptr.value,stack,'Guest stack restored');return result.join(' ');
};
for(let i=0;i<corpus.cases.length;i++)assert.equal(execute(corpus.cases[i]),corpus.output[i],`Native/WASM parity case ${i}: ${JSON.stringify(corpus.cases[i])}`);
const walked=e.probe_walk_tests();
const walkOutput=Buffer.from(e.memory.buffer,e.probe_walk_last_output(),4096);
assert.equal(walked,0,`Guest traversal: executed=${e.probe_walk_case_count()}, status=${e.probe_walk_last_status()}, paths=${JSON.stringify(walkOutput.subarray(0,walkOutput.indexOf(0)).toString())}`);
assert.equal(e.probe_walk_case_count(),18,'Every callback scenario actually executed');
assert.equal(e.probe_walk_negative_control(),6,'A deliberately wrong expected path must fail');
assert.equal(e.probe_contract_tests(),0,'Guest input/storage contract checks');
const pages=e.memory.buffer.byteLength/65536;
for(let i=0;i<500;i++){
 const c=corpus.cases[corpus.cases.length-5];assert.equal(execute(c),corpus.output[corpus.cases.length-5]);
 if(i%25===0)assert.equal(e.probe_walk_tests(),0);
}
assert.equal(e.memory.buffer.byteLength/65536,pages,'Repeated compile/match/traversal must not grow guest memory');
assert.equal(e.__stack_ptr.value,stack);
console.log(`${corpus.cases.length} native/WASM cases have identical return codes, instructions, ranges and work counters. Guest walker/API checks passed; 500 repetitions stable at ${pages} pages; compile workspace ${e.probe_memory()} bytes.`);
