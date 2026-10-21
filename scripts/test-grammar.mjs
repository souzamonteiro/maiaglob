import assert from 'node:assert/strict';
import {createRequire} from 'node:module';
const require=createRequire(import.meta.url);
const Parser=require('../generated/glob-parser.cjs');
const {ParseTreeCollector}=require('../maiacpp/maiacc/parser-generator/parse-tree-collector.js');
const valid=['','a','foo','.*','*.c','a?b','[!a]','[a-]','[-a]','[]a]','[a-z]','[z-a]','/src/*/*.cpp','é?','🌍','[🌍-🌎]','\\🌍','\\*','\\[x\\]','a b','[a*b?]','[\\!a]','[a\\-b]'];
const invalid=['[','[]','[!]','abc\\','[a','[a/b]','\ud800','\udfff','\\\ud800'];
for(const source of valid){const tree=new ParseTreeCollector();tree.parse(new Parser(source,tree));assert.equal(tree.root.name,'GlobPattern');}
for(const source of invalid)assert.throws(()=>{const tree=new ParseTreeCollector();tree.parse(new Parser(source,tree));},source);
// Every non-metacharacter ASCII byte must remain a possible Literal token.
for(let c=1;c<128;++c){if('*?[\\'.includes(String.fromCharCode(c)))continue;const tree=new ParseTreeCollector();tree.parse(new Parser(String.fromCharCode(c),tree));}
console.log('Generated JavaScript grammar oracle passed: literal byte coverage, classes, ranges, escapes, separators, Unicode and malformed syntax.');
